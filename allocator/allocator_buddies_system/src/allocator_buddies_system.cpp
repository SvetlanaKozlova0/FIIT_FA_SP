#include <not_implemented.h>
#include <cstddef>
#include "../include/allocator_buddies_system.h"
#include <sstream>


allocator_buddies_system::allocator_buddies_system(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger) {
        logger->debug("CALLED:               constructor");
    }
    size_t space_size_log2 = __detail::nearest_greater_k_of_2(space_size);
    if (space_size_log2 < min_k) {
        throw std::logic_error("trying to initialize allocator with too small size (size < size of block's metadata)");
    }
    size_t need_size = (1 << space_size_log2) + allocator_metadata_size;
    if (parent_allocator) {
        _trusted_memory = parent_allocator->allocate(need_size);
    } else {
        _trusted_memory = ::operator new(need_size);
    }
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    *reinterpret_cast<class logger**>(metadata) = logger;
    metadata += sizeof(class logger*);
    *reinterpret_cast<std::pmr::memory_resource**>(metadata) = parent_allocator;
    metadata += sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<fit_mode*>(metadata) = allocate_fit_mode;
    metadata += sizeof(fit_mode);
    *reinterpret_cast<unsigned char*>(metadata) = space_size_log2;
    metadata += sizeof(unsigned char);
    auto alloc_mutex = reinterpret_cast<std::mutex*>(metadata);
    new (alloc_mutex) std::mutex();
    metadata += sizeof(std::mutex);
    auto start_block = reinterpret_cast<block_metadata*>(metadata);
    start_block->size = space_size_log2 - min_k;
    start_block->occupied = false;
    debug_complete_message("constructor\n");
}


allocator_buddies_system::~allocator_buddies_system()
{
    debug_start_message("destructor");
    get_mutex().~mutex();
    debug_complete_message("destructor\n");
    if (get_parent_allocator() == nullptr) {
        ::operator delete(_trusted_memory);
    } else {
        get_parent_allocator()->deallocate(_trusted_memory, get_global_size() + allocator_metadata_size);
    }
}

allocator_buddies_system::allocator_buddies_system(
    allocator_buddies_system &&other) noexcept
{
    debug_start_message("move constructor");
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
    debug_complete_message("move constructor");
}

allocator_buddies_system &allocator_buddies_system::operator=(
    allocator_buddies_system &&other) noexcept
{
    debug_start_message("move assignment operator");
    if (this == &other) {
        debug_complete_message("move assignment operator");
        return *this;
    }
    std::swap(_trusted_memory, other._trusted_memory);
    debug_complete_message("move assignment operator\n");
    return *this;
}


[[nodiscard]] void *allocator_buddies_system::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(get_mutex());
    debug_start_message("do_allocate_sm");
    information_with_guard("[before allocating]   " + get_string_info(get_blocks_info_inner()));
    size_t need_size = size + occupied_block_metadata_size;
    void* ptr = allocate_with_mode(need_size);
    if (ptr == nullptr) {
        error_with_guard("bad alloc() while allocating block with size " + std::to_string(size));
        throw std::bad_alloc();
    }
    split_blocks(ptr, need_size);
    fill_info_about_block(ptr);
    if (get_block_size(ptr) != need_size) {
        warning_with_guard("allocator changed size of block to: " + std::to_string(get_block_size(ptr)) + " bytes");
    }
    information_with_guard("[after allocating]    " + get_string_info(get_blocks_info_inner()));
    debug_complete_message("do_allocate_sm\n");
    return reinterpret_cast<std::byte*>(ptr) + occupied_block_metadata_size;
}

//region help allocating functions
void *allocator_buddies_system::allocate_with_mode(size_t need_size) {
    void* ptr = nullptr;
    switch (get_fit_mode()) {
        case fit_mode::first_fit:
            ptr = allocate_first_fit(need_size);
            break;
        case fit_mode::the_best_fit:
            ptr = allocate_best_first(need_size);
            break;
        case fit_mode::the_worst_fit:
            ptr = allocate_worst_fit(need_size);
            break;
    }
    return ptr;
}

void *allocator_buddies_system::allocate_first_fit(size_t need_size) const noexcept {
    auto iter_end = end();
    for (auto iter = begin(); iter != iter_end; ++iter) {
        if (!iter.occupied() && iter.size() >= need_size) {
            return *iter;
        }
    }
    return nullptr;
}

void *allocator_buddies_system::allocate_best_first(size_t need_size) const noexcept {
    void* ptr = nullptr;
    size_t minimum = get_global_size();
    auto iter_end = end();
    for (auto iter = begin(); iter != iter_end; ++iter) {
        if (!iter.occupied() && iter.size() >= need_size && iter.size() <= minimum) {
            minimum = iter.size();
            ptr = *iter;
        }
    }
    return ptr;
}

void *allocator_buddies_system::allocate_worst_fit(size_t need_size) const noexcept {
    void* ptr = nullptr;
    size_t maximum = 0;
    auto iter_end = end();
    for (auto iter = begin(); iter != iter_end; ++iter) {
        if (!iter.occupied() && iter.size() >= need_size && iter.size() >= maximum) {
            maximum = iter.size();
            ptr = *iter;
        }
    }
    return ptr;
}

void allocator_buddies_system::split_blocks(void *ptr, size_t need_size) {
    trace_start_message("split_blocks");
    if (!can_split_block(ptr, need_size)) {
        trace_complete_message("split_blocks");
        return;
    }
    auto first_buddy = reinterpret_cast<block_metadata*>(ptr);
    first_buddy->size--;
    auto second_buddy = reinterpret_cast<block_metadata*>(get_buddy(ptr));
    second_buddy->size = first_buddy->size;
    second_buddy->occupied = false;
    split_blocks(ptr, need_size);
    trace_complete_message("split_blocks");
}

void allocator_buddies_system::fill_info_about_block(void *ptr) {
    auto block = reinterpret_cast<block_metadata*>(ptr);
    block->occupied = true;
    block++;
    *reinterpret_cast<void**>(block) = _trusted_memory;
}

bool allocator_buddies_system::can_split_block(void *ptr, size_t need_size) const {
    const size_t min_block_size = 1 << min_k;
    return (get_block_size(ptr) / 2) >= min_block_size
           && get_block_size(ptr) >= need_size * 2;
}
//endregion

void allocator_buddies_system::do_deallocate_sm(void *at)
{
    if (at == nullptr) {
        error_with_guard("trying to deallocate nullptr");
        return;
    }
    std::lock_guard lock(get_mutex());
    debug_start_message("do_deallocate_sm");
    if (get_parent_block(at) != _trusted_memory) {
        error_with_guard("trying to deallocate block that doesn't belong to this allocator");
        return;
    }
    information_with_guard("[before deallocating] " + get_string_info(get_blocks_info_inner()));
    void* occupied_block = get_real_address_block(at);
    reinterpret_cast<block_metadata*>(occupied_block)->occupied = false;
    merge_blocks(occupied_block);
    debug_complete_message("do_deallocate_sm\n");
    information_with_guard("[after deallocating]  " + get_string_info(get_blocks_info_inner()));
}

//region deallocating help functions
void allocator_buddies_system::merge_blocks(void *occupied_block) {
    trace_start_message("merge_blocks");
    void* buddy = get_buddy(occupied_block);
    if (!can_merge_block(occupied_block, buddy)) {
        trace_complete_message("merge_blocks");
        return;
    }
    void* new_block = (occupied_block < buddy) ? occupied_block : buddy;
    auto meta = reinterpret_cast<block_metadata*>(new_block);
    meta->size++;
    merge_blocks(new_block);
    trace_complete_message("merge_blocks");
}

bool allocator_buddies_system::can_merge_block(void *occupied_block, void *buddy) const {
    return get_block_size(occupied_block) != get_global_size() &&
    get_block_size(occupied_block) == get_block_size(buddy) && !is_block_occupied(buddy);
}
//endregion

bool allocator_buddies_system::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return typeid(*this) == typeid(other);
}

inline void allocator_buddies_system::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard lock(get_mutex());
    debug_start_message("set_fit_mode");
    debug_with_guard("new fit mode: " + fit_mode_to_string(mode));
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<fit_mode*>(metadata) = mode;
    debug_complete_message("set_fit_mode");
}



std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> result;
    for (auto iter = begin(), end_iter = end(); iter != end_iter; ++iter) {
        result.push_back({iter.size(), iter.occupied()});
    }
    return result;
}

std::string
allocator_buddies_system::get_string_info(const std::vector<allocator_test_utils::block_info> &info) const noexcept {
    std::ostringstream result;
    result << "| ";
    for (auto& current_block: info) {
        result << (current_block.is_block_occupied ? "<occup>" : "<avail>");
        result << " < " + std::to_string(current_block.block_size) + " > | ";
    }
    return result.str();
}


//region buddy iterator
allocator_buddies_system::buddy_iterator allocator_buddies_system::begin() const noexcept
{
    return buddy_iterator(get_first_block());
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::end() const noexcept
{
    return buddy_iterator(get_first_block() + get_global_size());
}

bool allocator_buddies_system::buddy_iterator::operator==(const allocator_buddies_system::buddy_iterator &other) const noexcept
{
    return _block == other._block;
}

bool allocator_buddies_system::buddy_iterator::operator!=(const allocator_buddies_system::buddy_iterator &other) const noexcept
{
    return _block != other._block;
}

allocator_buddies_system::buddy_iterator &allocator_buddies_system::buddy_iterator::operator++() & noexcept
{
    _block = reinterpret_cast<std::byte*>(_block) + get_block_size(_block);
    return *this;
}

allocator_buddies_system::buddy_iterator allocator_buddies_system::buddy_iterator::operator++(int n)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

size_t allocator_buddies_system::buddy_iterator::size() const noexcept
{
    return get_block_size(_block);
}

bool allocator_buddies_system::buddy_iterator::occupied() const noexcept
{
    return is_block_occupied(_block);
}

void *allocator_buddies_system::buddy_iterator::operator*() const noexcept
{
    return _block;
}

allocator_buddies_system::buddy_iterator::buddy_iterator(void *start): _block(start)
{
}

allocator_buddies_system::buddy_iterator::buddy_iterator(): _block(nullptr)
{
}
//endregion buddy iterator

//region helper functions

void *allocator_buddies_system::get_buddy(void *ptr) {
    debug_start_message("get_buddy");
    size_t offset = reinterpret_cast<std::byte*>(ptr) - get_first_block();
    size_t size_degree2 = reinterpret_cast<block_metadata*>(ptr)->size + min_k;
    size_t k = 1ULL << size_degree2;
    size_t buddy_offset = offset ^ k;
    debug_complete_message("get_buddy");
    return get_first_block() + buddy_offset;
}

void *allocator_buddies_system::get_real_address_block(void *block) const {
    return reinterpret_cast<std::byte*>(block) - occupied_block_metadata_size;
}

void *allocator_buddies_system::get_parent_block(void *block) const {
    return *reinterpret_cast<void**>(reinterpret_cast<std::byte*>(block) - sizeof(void*));
}

inline logger *allocator_buddies_system::get_logger() const
{
    return *reinterpret_cast<logger**>(_trusted_memory);
}

std::pmr::memory_resource *allocator_buddies_system::get_parent_allocator() const {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*);
    return *reinterpret_cast<std::pmr::memory_resource**>(metadata);
}

allocator_with_fit_mode::fit_mode allocator_buddies_system::get_fit_mode() const {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*);
    return *reinterpret_cast<fit_mode*>(metadata);
}

size_t allocator_buddies_system::get_global_size() const {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode);
    unsigned char k = *reinterpret_cast<unsigned char*>(metadata);
    return 1 << k;
}

std::mutex &allocator_buddies_system::get_mutex() {
    trace_start_message("get_mutex");
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + sizeof(unsigned char);
    trace_complete_message("get_mutex");
    return *reinterpret_cast<std::mutex*>(metadata);
}

size_t allocator_buddies_system::get_block_size(void *ptr) {
    auto metadata = reinterpret_cast<block_metadata*>(ptr);
    unsigned char k = metadata->size;
    return 1 << (min_k + k);
}

bool allocator_buddies_system::is_block_occupied(void *ptr) {
    return reinterpret_cast<block_metadata*>(ptr)->occupied;
}

inline std::string allocator_buddies_system::get_typename() const
{
    return "allocator_buddies_system";
}


std::string allocator_buddies_system::fit_mode_to_string(allocator_with_fit_mode::fit_mode mode) const {
    switch (mode) {
        case fit_mode::first_fit:
            return "first fit";
        case fit_mode::the_best_fit:
            return "best fit";
        case fit_mode::the_worst_fit:
            return "worst fit";
    }
}

std::byte *allocator_buddies_system::get_first_block() const {
    return reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size;
}
//endregion

//region logger functions

void allocator_buddies_system::debug_start_message(const std::string &message){
    debug_with_guard("CALLED:               " + message);
}

void allocator_buddies_system::trace_start_message(const std::string &message){
    trace_with_guard("CALLED:               " + message);
}

void allocator_buddies_system::debug_complete_message(const std::string &message) {
    debug_with_guard("COMPLETED:            " + message);
}

void allocator_buddies_system::trace_complete_message(const std::string &message) {
    trace_with_guard("COMPLETED:            " + message);
}
//endregion