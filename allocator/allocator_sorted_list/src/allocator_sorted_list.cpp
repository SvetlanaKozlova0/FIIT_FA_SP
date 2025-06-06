#include <not_implemented.h>
#include "../include/allocator_sorted_list.h"

allocator_sorted_list::~allocator_sorted_list()
{
    debug_with_guard("destructor called");
    if (_trusted_memory == nullptr) {
        return;
    }
    get_mutex().~mutex();
    std::pmr::memory_resource* parent_alloc = get_parent_allocator();
    parent_alloc->deallocate(_trusted_memory, get_global_size() + allocator_metadata_size);
}

allocator_sorted_list::allocator_sorted_list(
        allocator_sorted_list &&other) noexcept
{
    debug_with_guard("move constructor called");
    std::lock_guard<std::mutex> lock(other.get_mutex());
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
    debug_with_guard("move constructor completed");
}

allocator_sorted_list &allocator_sorted_list::operator=(
        allocator_sorted_list &&other) noexcept
{
    debug_with_guard("move assign constructor called");
    std::lock_guard<std::mutex> lock(get_mutex());
    std::swap(_trusted_memory, other._trusted_memory);
    debug_with_guard("move assign operator completed");
    return *this;
}

allocator_sorted_list::allocator_sorted_list(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        logger *log,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (log != nullptr) {
        log->debug("constructor started");
    }
    if (space_size == 0) {
        if (log != nullptr) {
            log->error("an  attempt to initialize the allocator's working memory of size 0");
        }
        throw std::logic_error("an attempt to initialize the allocator's working memory of size 0");
    }
    size_t total_size = space_size + allocator_metadata_size;
    if (parent_allocator != nullptr) {
        _trusted_memory = parent_allocator->allocate(total_size);
        if (_trusted_memory == nullptr) {
            if (log != nullptr)
                log->error("error while allocating memory for allocator by parent allocator");
        }
        throw std::bad_alloc();
    } else {
        try {
            _trusted_memory = std::pmr::get_default_resource()->allocate(total_size);
        }
        catch (const std::exception& ex) {
            if (log != nullptr) {
                log->error("exception while allocating memory for allocator by default pmr resource");
            }
            throw ex;
        }
    }
    auto* metadata = static_cast<std::byte*>(_trusted_memory);
    *reinterpret_cast<logger**>(metadata) = log;
    metadata += sizeof(logger*);
    *reinterpret_cast<std::pmr::memory_resource**>(metadata) = parent_allocator ? parent_allocator : std::pmr::get_default_resource();
    metadata += sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<fit_mode*>(metadata) = allocate_fit_mode;
    metadata += sizeof(fit_mode);
    *reinterpret_cast<size_t *>(metadata) = space_size;
    metadata += sizeof(size_t);
    auto* mutex_ptr = reinterpret_cast<std::mutex*>(metadata);
    new (mutex_ptr) std::mutex();
    metadata += sizeof(std::mutex);
    *reinterpret_cast<void**>(metadata) = metadata + sizeof(void*);
    metadata += sizeof(void*);
    *reinterpret_cast<void**>(metadata) = nullptr;
    metadata += sizeof(void*);
    *reinterpret_cast<size_t*>(metadata) = space_size - block_metadata_size;
    debug_with_guard("constructor completed");
}

[[nodiscard]] void *allocator_sorted_list::do_allocate_sm(
        size_t need_size)
{
    std::lock_guard<std::mutex> lock(get_mutex());
    debug_with_guard("allocation of " + std::to_string(need_size) + " bytes started");
    auto blocks = get_blocks_info_inner();
    debug_with_guard("blocks before allocating " + std::to_string(need_size) + " bytes:\n" + info_to_string(blocks));
    //information_with_guard("available memory: " + std::to_string(get_available_size(blocks)));
    if (get_global_size() < need_size) {
        throw std::bad_alloc();
    }
    debug_with_guard("fit mode: " + fit_mode_to_string(get_fit_mode()));
    void* result = allocate_with_mode(need_size);
    if (result != nullptr) {
        if (*reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(result) + sizeof(void*)) != need_size) {
            warning_with_guard("allocator changed size to " + std::to_string(*reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(result) + sizeof(void*))));
        }
    }
    blocks = get_blocks_info_inner();
    debug_with_guard("allocation completed");
    debug_with_guard("blocks after allocating:\n" + info_to_string(blocks));
    information_with_guard("available memory: " + std::to_string(get_available_size(blocks)));
    return result;
}

void allocator_sorted_list::do_deallocate_sm(void *at) {
    if (at == nullptr) {
        debug_with_guard("deallocating nullptr");
        return;
    }
    std::lock_guard<std::mutex> lock(get_mutex());
    auto blocks = get_blocks_info_inner();
    debug_with_guard("deallocating started");
    debug_with_guard("blocks before deallocating:\n" + info_to_string(blocks));
    //information_with_guard("available memory: " + std::to_string(get_available_size(blocks)));
    void* original_ptr = reinterpret_cast<std::byte*>(at) - block_metadata_size;
    if (*reinterpret_cast<void**>(original_ptr) != _trusted_memory) {
        error_with_guard("this block doesn't belong to this allocator");
        return;
    }
    size_t original_size = *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(original_ptr) + sizeof(void*));
    void **first_ptr = get_first_ptr();
    void *prev = nullptr;
    void *current = *first_ptr;
    while (current != nullptr && current < original_ptr) {
        prev = current;
        current = *reinterpret_cast<void**>(current);
    }
    if (prev == nullptr) {
        *first_ptr = original_ptr;
    } else {
        *reinterpret_cast<void**>(prev) = original_ptr;
    }
    *reinterpret_cast<void**>(original_ptr) = current;
    if (current != nullptr) {
        std::byte* original_end = reinterpret_cast<std::byte*>(original_ptr) + block_metadata_size + original_size;
        if (original_end == reinterpret_cast<std::byte*>(current)) {
            original_size += *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(current) + sizeof(void*)) + block_metadata_size;
            *reinterpret_cast<void**>(original_ptr) = *reinterpret_cast<void**>(current);
            *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(original_ptr) + sizeof(void*)) = original_size;
        }
    }
    if (prev != nullptr) {
        size_t prev_size = *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(prev) + sizeof(void*));
        std::byte* prev_end = reinterpret_cast<std::byte*>(prev) + block_metadata_size + prev_size;
        if (prev_end == reinterpret_cast<std::byte*>(original_ptr)) {
            prev_size += original_size + block_metadata_size;
            *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(prev) + sizeof(void*)) = prev_size;
            *reinterpret_cast<void**>(prev) = *reinterpret_cast<void**>(original_ptr);
        }
    }
    blocks = get_blocks_info_inner();
    debug_with_guard("blocks after deallocating:\n" + info_to_string(blocks));
    information_with_guard("available memory: " + std::to_string(get_available_size(blocks)));
    debug_with_guard("deallocating completed");
}



//region allocation helping functions

void *allocator_sorted_list::allocate_with_mode(size_t need_size) noexcept {
    trace_with_guard("allocate_with_mode called and completed");
    size_t full_size = need_size + block_metadata_size;
    switch (get_fit_mode()) {
        case fit_mode::first_fit:
            return allocate_first_fit(full_size);
        case fit_mode::the_best_fit:
            return allocate_best_fit(full_size);
        case fit_mode::the_worst_fit:
            return allocate_worst_fit(full_size);
    }
}

void* allocator_sorted_list::make_block_busy(size_t need_size, size_t block_size, void *place, void* previous) {
    trace_with_guard("make_block_busy called");
    if (place == nullptr) return nullptr;
    bool need_to_cut = false;
    void* next = *reinterpret_cast<void**>(place);
    if (block_size - need_size > block_metadata_size) {
        need_to_cut = true;
    }
    auto* block_meta = reinterpret_cast<std::byte*>(place);
    *reinterpret_cast<void**>(block_meta) = _trusted_memory;
    block_meta += sizeof(void*);
    *reinterpret_cast<size_t*>(block_meta) = need_to_cut ? need_size : block_size;
    block_meta += sizeof(size_t);
    void* ptr_to_return = block_meta;
    if (!need_to_cut) {
        if (previous != nullptr) {
            *reinterpret_cast<void**>(previous) = next;
        } else {
            *get_first_ptr() = next;
        }
        trace_with_guard("make_block_busy completed");
        return ptr_to_return;
    }
    block_meta += need_size;
    void* temp = block_meta;
    *reinterpret_cast<void**>(block_meta) = next;
    block_meta += sizeof(void*);
    *reinterpret_cast<size_t*>(block_meta) = block_size - need_size - block_metadata_size;
    if (previous != nullptr) {
        *reinterpret_cast<void**>(previous) = temp;
    } else {
        *get_first_ptr() = temp;
    }
    trace_with_guard("make_block_busy completed");
    return ptr_to_return;
}


void* allocator_sorted_list::allocate_first_fit(size_t need_size) noexcept {
    trace_with_guard("allocate_first_fit called");
    void **first_ptr = get_first_ptr();
    void *current = *first_ptr;
    void *prev = nullptr;
    while (current != nullptr) {
        size_t block_size = *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(current) + sizeof(void*));
        if (block_size >= need_size) {
            trace_with_guard("allocate_first_fit completed");
            return make_block_busy(need_size, block_size, current, prev);
        }
        prev = current;
        current = *reinterpret_cast<void**>(current);
    }
    trace_with_guard("allocate_first_fit completed");
    return nullptr;
}

void* allocator_sorted_list::allocate_best_fit(size_t need_size) noexcept {
    trace_with_guard("allocate_best_fit called");
    void **first_ptr = get_first_ptr();
    void *current = *first_ptr;
    void *prev = nullptr;
    void *best_block = nullptr;
    void *best_prev = nullptr;
    size_t best_size = get_global_size();
    while (current != nullptr) {
        size_t block_size = *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(current) + sizeof(void*));
        if (block_size >= need_size && block_size < best_size) {
            best_size = block_size;
            best_block = current;
            best_prev = prev;
        }
        prev = current;
        current = *reinterpret_cast<void**>(current);
    }
    trace_with_guard("allocate_best_fit completed");
    return best_block ? make_block_busy(need_size, best_size, best_block, best_prev) : nullptr;
}


void* allocator_sorted_list::allocate_worst_fit(size_t need_size) noexcept {
    trace_with_guard("allocate_worst_fit called");
    void **first_ptr = get_first_ptr();
    void *current = *first_ptr;
    void *prev = nullptr;
    void *worst_block = nullptr;
    void *worst_prev = nullptr;
    size_t worst_size = 0;
    while (current != nullptr) {
        size_t block_size = *reinterpret_cast<size_t*>(reinterpret_cast<std::byte*>(current) + sizeof(void*));
        if (block_size >= need_size && block_size > worst_size) {
            worst_size = block_size;
            worst_block = current;
            worst_prev = prev;
        }
        prev = current;
        current = *reinterpret_cast<void**>(current);
    }
    trace_with_guard("allocate_worst_fit completed");
    return worst_block ? make_block_busy(need_size, worst_size, worst_block, worst_prev) : nullptr;
}


//endregion

//region getter functions

inline logger *allocator_sorted_list::get_logger() const
{
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    return *reinterpret_cast<logger**>(metadata);
}

inline std::string allocator_sorted_list::get_typename() const
{
    return "allocator_sorted_list";
}

allocator_with_fit_mode::fit_mode allocator_sorted_list::get_fit_mode() const noexcept {
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource *);
    return *reinterpret_cast<fit_mode*>(metadata);
}

std::mutex &allocator_sorted_list::get_mutex() const {
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource *) + sizeof(fit_mode) + sizeof(size_t);
    return *reinterpret_cast<std::mutex*>(metadata);
}

std::pmr::memory_resource *allocator_sorted_list::get_parent_allocator() const noexcept {
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*);
    return *reinterpret_cast<std::pmr::memory_resource**>(metadata);
}

size_t allocator_sorted_list::get_global_size() const noexcept {
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource *) + sizeof(fit_mode);
    return *reinterpret_cast<size_t*>(metadata);
}

void **allocator_sorted_list::get_first_ptr() const noexcept {
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += allocator_metadata_size - sizeof(void*);
    return reinterpret_cast<void**>(metadata);
}

//endregion

//region help functions

inline void allocator_sorted_list::set_fit_mode(
        allocator_with_fit_mode::fit_mode mode)
{
    trace_with_guard("set_fit_mode called");
    auto* metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource *);
    trace_with_guard("set_fit_mode completed");
    *reinterpret_cast<fit_mode*>(metadata) = mode;
}

bool allocator_sorted_list::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return typeid(*this) == typeid(other);
}

std::string allocator_sorted_list::info_to_string(const std::vector<allocator_test_utils::block_info> &blocks) {
    trace_with_guard("info_to_string called");
    std::string info = "| ";
    for (auto& block: blocks) {
        info += block.is_block_occupied ? "<occup> " : "<avail> ";
        info += std::to_string(block.block_size) + " | ";
    }
    trace_with_guard("info_to_string completed");
    return info;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    return get_blocks_info_inner();
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info_inner() const {
    std::vector<allocator_test_utils::block_info> blocks;
    std::byte* current_block = reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size;
    const std::byte* end_ptr = reinterpret_cast<std::byte*>(_trusted_memory)
                               + allocator_metadata_size
                               + get_global_size();
    while (current_block < end_ptr) {
        void* block_ptr = *reinterpret_cast<void**>(current_block);
        size_t block_size = *reinterpret_cast<size_t*>(current_block + sizeof(void*));
        blocks.push_back({.block_size = block_size,
                                 .is_block_occupied = (block_ptr == _trusted_memory)});
        current_block += block_metadata_size + block_size;
    }
    return blocks;
}

std::string allocator_sorted_list::fit_mode_to_string(allocator_with_fit_mode::fit_mode mode) {
    trace_with_guard("fit_mode_to_string called and completed");
    switch (mode) {
        case fit_mode::first_fit:
            return "first_fit";
        case fit_mode::the_best_fit:
            return "best_fit";
        case fit_mode::the_worst_fit:
            return "worst_fit";
    }
}

size_t allocator_sorted_list::get_available_size(const std::vector<allocator_test_utils::block_info> &blocks) {
    trace_with_guard("get_available_size called");
    size_t avail_memory = 0;
    for (auto& block: blocks) {
        if (!block.is_block_occupied) {
            avail_memory += block.block_size;
        }
    }
    trace_with_guard("get_available_size completed");
    return avail_memory;
}
//endregion