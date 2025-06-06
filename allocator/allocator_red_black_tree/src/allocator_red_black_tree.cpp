#include <not_implemented.h>

#include "../include/allocator_red_black_tree.h"

allocator_red_black_tree::allocator_red_black_tree(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        logger *logger,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    size_t total_size = space_size + allocator_metadata_size;
    if (parent_allocator == nullptr) {
        _trusted_memory = ::operator new(total_size);
    } else {
        _trusted_memory = parent_allocator->allocate(total_size, 1);
    }
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    *reinterpret_cast<class logger**>(metadata) = logger;
    metadata += sizeof(class logger*);
    *reinterpret_cast<std::pmr::memory_resource**>(metadata) = parent_allocator;
    metadata += sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<fit_mode*>(metadata) = allocate_fit_mode;
    metadata += sizeof(fit_mode);
    *reinterpret_cast<size_t*>(metadata) = space_size;
    metadata += sizeof(size_t);
    auto alloc_mutex = reinterpret_cast<std::mutex*>(metadata);
    new (alloc_mutex) std::mutex();
    metadata += sizeof(std::mutex);
    auto first_block = reinterpret_cast<void**>(metadata);
    *first_block = reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size;
    metadata += sizeof(void*);
    auto first_free_block = reinterpret_cast<void*>(metadata);
    (*reinterpret_cast<block_data*>(first_free_block)).color = block_color::BLACK;
    (*reinterpret_cast<block_data*>(first_free_block)).occupied = false;
    get_left_block(first_free_block) = nullptr;
    get_right_block(first_free_block) = nullptr;
    get_previous_block(first_free_block) = nullptr;
    get_next_block(first_free_block) = nullptr;
    get_parent_block(first_free_block) = nullptr;
    debug_with_guard("constructor completed");
}

[[nodiscard]] void *allocator_red_black_tree::do_allocate_sm(
    size_t size)
{
    std::lock_guard lock(get_mutex());
    debug_with_guard("do_allocate_sm called");
    void* result_ptr = allocate_with_mode(size);
    if (result_ptr == nullptr) {
        error_with_guard("bad allocation");
        throw std::bad_alloc();
    }
    erase_from_tree(result_ptr);
    get_parent_block(result_ptr) = _trusted_memory;
    (*reinterpret_cast<block_data*>(result_ptr)).occupied = true;
    if (get_block_size(result_ptr) < size + free_block_metadata_size) {
        warning_with_guard("allocator changed size of block");
    } else {
        split_block(result_ptr, size);
    }
    debug_with_guard("do_allocate_sm completed");
    debug_with_guard(get_info_in_string(get_blocks_info()));
    information_with_guard("memory state: " + std::to_string(get_all_free_size()));
    return reinterpret_cast<std::byte*>(result_ptr) + occupied_block_metadata_size;
}


void allocator_red_black_tree::do_deallocate_sm(
    void *at)
{
    std::lock_guard lock(get_mutex());
    debug_with_guard("do_deallocate_sm called");
    void* real_block = reinterpret_cast<std::byte*>(at) - occupied_block_metadata_size;
    if (get_parent_block(real_block) != _trusted_memory) {
        error_with_guard("trying to delete block that doesn't belong to this allocator");
        throw std::logic_error("trying to delete block that doesn't belong to this allocator");
    }
    (*reinterpret_cast<block_data*>(real_block)).occupied = false;
    merge_blocks(real_block);
    insert_in_tree(real_block);
    debug_with_guard("do_deallocate_sm completed");
    debug_with_guard(get_info_in_string(get_blocks_info()));
    information_with_guard("memory state: " + std::to_string(get_all_free_size()));
}

allocator_red_black_tree::~allocator_red_black_tree()
{
    debug_with_guard("destructor called");
    get_mutex().~mutex();
    debug_with_guard("destructor completed");
    if (get_parent_allocator() == nullptr) {
        ::operator delete(_trusted_memory);
    } else {
        get_parent_allocator()->deallocate(_trusted_memory, get_global_size() + allocator_metadata_size);
    }
}

allocator_red_black_tree::allocator_red_black_tree(
        allocator_red_black_tree &&other) noexcept
{
    debug_with_guard("move constructor called");
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
    debug_with_guard("move constructor completed");
}


allocator_red_black_tree &allocator_red_black_tree::operator=(
        allocator_red_black_tree &&other) noexcept
{
    debug_with_guard("move assigment operator called");
    if (this == &other) {
        debug_with_guard("move assignment operator completed");
        return *this;
    }
    std::swap(_trusted_memory, other._trusted_memory);
    debug_with_guard("move assignment operator completed");
    return *this;
}

//region iterators
allocator_red_black_tree::rb_iterator allocator_red_black_tree::begin() const noexcept
{
    return {_trusted_memory};
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::end() const noexcept
{
    return {};
}

bool allocator_red_black_tree::rb_iterator::operator==(const allocator_red_black_tree::rb_iterator &other) const noexcept
{
    return _block_ptr == other._block_ptr;
}

bool allocator_red_black_tree::rb_iterator::operator!=(const allocator_red_black_tree::rb_iterator &other) const noexcept
{
    return !(*this == other);
}

allocator_red_black_tree::rb_iterator::rb_iterator(): _block_ptr(nullptr), _trusted(nullptr)
{
}

bool allocator_red_black_tree::rb_iterator::occupied() const noexcept
{
    return reinterpret_cast<block_data*>(_block_ptr)->occupied;
}

allocator_red_black_tree::rb_iterator::rb_iterator(void *trusted)
{
    _trusted = trusted;
    _block_ptr = reinterpret_cast<void*>(reinterpret_cast<std::byte*>(trusted) + allocator_metadata_size);
}

allocator_red_black_tree::rb_iterator allocator_red_black_tree::rb_iterator::operator++(int n)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

void *allocator_red_black_tree::rb_iterator::operator*() const noexcept
{
    return _block_ptr;
}

allocator_red_black_tree::rb_iterator &allocator_red_black_tree::rb_iterator::operator++() & noexcept
{
    _block_ptr = get_next_block(_block_ptr);
    return *this;
}

size_t allocator_red_black_tree::rb_iterator::size() const noexcept
{
    auto metadata = reinterpret_cast<std::byte*>(_trusted);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode);
    size_t global_size =  *reinterpret_cast<size_t*>(metadata);
    if (get_next_block(_block_ptr) == nullptr) {
        return reinterpret_cast<std::byte*>(_trusted) + allocator_metadata_size + global_size -
               reinterpret_cast<std::byte*>(_block_ptr) - occupied_block_metadata_size;
    }
    return reinterpret_cast<std::byte*>(get_next_block(_block_ptr)) - reinterpret_cast<std::byte*>(_block_ptr) - occupied_block_metadata_size;
}

//endregion

//region access functions

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
    return "allocator_red_black_tree";
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info_inner() const
{
    std::vector<allocator_test_utils::block_info> blocks;
    for (auto iter = begin(); iter != end(); ++iter) {
        blocks.push_back({iter.size(), iter.occupied()});
    }
    return blocks;
}

inline logger *allocator_red_black_tree::get_logger() const
{
    return *reinterpret_cast<logger**>(_trusted_memory);
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const
{
    return get_blocks_info_inner();
}

std::pmr::memory_resource *allocator_red_black_tree::get_parent_allocator() const {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*);
    return *reinterpret_cast<std::pmr::memory_resource**>(metadata);
}

std::mutex &allocator_red_black_tree::get_mutex() const noexcept {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + sizeof(size_t);
    return *reinterpret_cast<std::mutex*>(metadata);
}

void **allocator_red_black_tree::get_first_ptr(void *trusted_mem) noexcept {
    auto metadata = reinterpret_cast<std::byte*>(trusted_mem);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + sizeof(size_t) + sizeof(std::mutex);
    return reinterpret_cast<void**>(metadata);
}

size_t allocator_red_black_tree::get_global_size() const {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode);
    return *reinterpret_cast<size_t*>(metadata);
}

std::string
allocator_red_black_tree::get_info_in_string(const std::vector<allocator_test_utils::block_info> &blocks) noexcept {
    trace_with_guard("get_info_in_string called");
    std::string string_info;
    for (auto& block: blocks) {
        string_info += block.is_block_occupied ? "<occup> " : "<avail> ";
        string_info += "<" + std::to_string(block.block_size) + "> |";
    }
    trace_with_guard("get_info_in_string completed");
    return string_info;
}

allocator_with_fit_mode::fit_mode allocator_red_black_tree::get_fit_mode() const noexcept {
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*);
    return *reinterpret_cast<fit_mode*>(metadata);
}

//endregion

//region allocate functions
void *allocator_red_black_tree::allocate_first_fit(size_t need_size) const noexcept {
    void* result_ptr = *get_first_ptr(_trusted_memory);
    while (result_ptr != nullptr) {
        if (get_block_size(result_ptr) >= need_size) {
            return result_ptr;
        }
        result_ptr = get_right_block(result_ptr);
    }
    return result_ptr;
}

void *allocator_red_black_tree::allocate_worst_fit(size_t need_size) const noexcept {
    void* result_ptr = nullptr;
    void* worst = *get_first_ptr(_trusted_memory);
    while (worst != nullptr) {
        if (get_block_size(worst) >= need_size) {
            result_ptr = worst;
        }
        worst = get_right_block(worst);
    }
    return result_ptr;
}

void *allocator_red_black_tree::allocate_best_fit(size_t need_size) const noexcept {
    void* result_ptr = nullptr;
    void* best = *get_first_ptr(_trusted_memory);
    size_t current_size = 0;
    while (best != nullptr) {
        current_size = get_block_size(best);
        if (current_size >= need_size) {
            result_ptr = best;
        }
        if (current_size > need_size) {
            best = get_left_block(best);
        } else if (current_size < need_size) {
            best = get_right_block(best);
        } else {
            best = nullptr;
        }
    }
    return result_ptr;
}

void* allocator_red_black_tree::allocate_with_mode(size_t need_size) const noexcept {
    switch (get_fit_mode()) {
        case fit_mode::first_fit:
            return allocate_first_fit(need_size);
        case fit_mode::the_best_fit:
            return allocate_best_fit(need_size);
        case fit_mode::the_worst_fit:
            return allocate_worst_fit(need_size);
        default:
            return nullptr;
    }
}

//endregion

//region rotate functions
void allocator_red_black_tree::small_left_rotate(void *block) noexcept {
    trace_with_guard("small_left_rotate called");
    if (get_right_block(block) == nullptr) {
        trace_with_guard("small_left_rotate completed");
        return;
    }
    void* right_child = get_right_block(block);
    if (get_parent_block(block) == nullptr) {
        *get_first_ptr(_trusted_memory) = right_child;
    } else if (block == get_left_block(get_parent_block(block))) {
        get_left_block(get_parent_block(block)) = right_child;
    } else {
        get_right_block(get_parent_block(block)) = right_child;
    }
    get_parent_block(right_child) = get_parent_block(block);
    void* left_child = get_left_block(right_child);
    get_left_block(right_child) = block;
    get_parent_block(block) = right_child;
    get_right_block(block) = left_child;
    if (left_child != nullptr) {
        get_parent_block(left_child) = block;
    }
    trace_with_guard("small_left_rotate completed");
}

void allocator_red_black_tree::small_right_rotate(void *block) noexcept {
    trace_with_guard("small_right_rotate called");
    if (get_left_block(block) == nullptr) {
        trace_with_guard("small_right_rotate completed");
        return;
    }
    void* left_child = get_left_block(block);
    if (get_parent_block(block) == nullptr) {
        *get_first_ptr(_trusted_memory) = left_child;
    } else if (block == get_left_block(get_parent_block(block))) {
        get_left_block(get_parent_block(block)) = left_child;
    } else {
        get_right_block(get_parent_block(block)) = left_child;
    }
    get_parent_block(left_child) = get_parent_block(block);
    void* right_child = get_right_block(left_child);
    get_right_block(left_child) = block;
    get_parent_block(block) = left_child;
    get_left_block(block) = right_child;
    if (right_child != nullptr) {
        get_parent_block(right_child) = block;
    }
    trace_with_guard("small_left_rotate completed");
}

void allocator_red_black_tree::big_left_rotate(void *block) noexcept {
    trace_with_guard("big_left_rotate called");
    if (get_right_block(block) == nullptr || get_left_block(get_right_block(block)) == nullptr) {
        trace_with_guard("big_left_rotate completed");
        return;
    }
    void* temp = get_right_block(block);
    small_right_rotate(temp);
    small_left_rotate(block);
    trace_with_guard("big_left_rotate completed");
}

void allocator_red_black_tree::big_right_rotate(void *block) noexcept {
    trace_with_guard("big_right_rotate called");
    if (get_left_block(block) == nullptr || get_right_block(get_left_block(block)) == nullptr) {
        trace_with_guard("big_right_rotate completed");
        return;
    }
    void* temp = get_left_block(block);
    small_left_rotate(temp);
    small_right_rotate(block);
    trace_with_guard("big_right_rotate completed");
}

//endregion

//region block access functions

void *&allocator_red_black_tree::get_left_block(void *current_block) {
    auto block = reinterpret_cast<std::byte*>(current_block);
    block += sizeof(block_data) + 3 * sizeof(void*);
    return *reinterpret_cast<void**>(block);
}

void *&allocator_red_black_tree::get_right_block(void *current_block) {
    auto block = reinterpret_cast<std::byte*>(current_block);
    block += sizeof(block_data) + 4 * sizeof(void*);
    return *reinterpret_cast<void**>(block);
}

void *&allocator_red_black_tree::get_parent_block(void *current_block) {
    auto block = reinterpret_cast<std::byte*>(current_block);
    block += sizeof(block_data) + 2 * sizeof(void*);
    return *reinterpret_cast<void**>(block);
}

void *&allocator_red_black_tree::get_next_block(void *current_block) {
    auto block = reinterpret_cast<std::byte*>(current_block);
    block += sizeof(block_data) + sizeof(void*);
    return *reinterpret_cast<void**>(block);
}

void *&allocator_red_black_tree::get_previous_block(void *current_block) {
    auto block = reinterpret_cast<std::byte*>(current_block);
    block += sizeof(block_data);
    return *reinterpret_cast<void**>(block);
}

size_t allocator_red_black_tree::get_block_size(void *block) const noexcept {
    if (get_next_block(block) == nullptr) {
        return reinterpret_cast<std::byte*>(_trusted_memory) + allocator_metadata_size + get_global_size() -
               reinterpret_cast<std::byte*>(block) - occupied_block_metadata_size;
    }
    return reinterpret_cast<std::byte*>(get_next_block(block)) - reinterpret_cast<std::byte*>(block) - occupied_block_metadata_size;
}

bool allocator_red_black_tree::is_red(void *block) const noexcept {
    return (*reinterpret_cast<block_data*>(block)).color == block_color::RED;
}

bool allocator_red_black_tree::is_black(void *block) const noexcept {
    return (*reinterpret_cast<block_data*>(block)).color == block_color::BLACK;
}

//endregion

//region help functions

bool allocator_red_black_tree::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return typeid(*this) == typeid(other);
}

void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard lock(get_mutex());
    trace_with_guard("set_fit_mode called");
    auto metadata = reinterpret_cast<std::byte*>(_trusted_memory);
    metadata += sizeof(logger*) + sizeof(std::pmr::memory_resource*);
    *reinterpret_cast<fit_mode*>(metadata) = mode;
    trace_with_guard("set_fit_mode completed");
}

size_t allocator_red_black_tree::get_all_free_size() const noexcept {
    size_t free_size = 0;
    auto end_iter = end();
    for (auto iter = begin(); iter != end_iter; ++iter) {
        if (!iter.occupied()) {
            free_size += iter.size();
        }
    }
    return free_size;
}

//endregion

//region tree functions

void allocator_red_black_tree::insert_in_tree(void *current_block) noexcept {
    trace_with_guard("insert_in_tree called");
    void* first_block = *get_first_ptr(_trusted_memory);
    void* parent = nullptr;
    while (first_block != nullptr) {
        if (get_block_size(current_block) < get_block_size(first_block)) {
            parent = first_block;
            first_block = get_left_block(first_block);
        } else {
            parent = first_block;
            first_block = get_right_block(first_block);
        }
    }
    get_parent_block(current_block) = parent;
    get_left_block(current_block) = nullptr;
    get_right_block(current_block) = nullptr;
    (*reinterpret_cast<block_data*>(current_block)).occupied = false;
    make_red(current_block);
    if (parent == nullptr) {
        *get_first_ptr(_trusted_memory) = current_block;
        make_black(current_block);
        trace_with_guard("insert_in_tree completed");
        return;
    }
    if (get_block_size(current_block) < get_block_size(parent)) {
        get_left_block(parent) = current_block;
    } else {
        get_right_block(parent) = current_block;
    }
    balance_after_insert(current_block, parent);
    trace_with_guard("insert_in_tree completed");
}

void allocator_red_black_tree::balance_after_insert(void *current_block, void *parent) noexcept {
    trace_with_guard("balance_after_insert called");
    bool is_left_child = (current_block == get_left_block(parent));
    while (true) {
        if (is_black(parent)) {
            trace_with_guard("balance_after_insert completed");
            break;
        }
        if (is_red(parent)) {
            void* grandparent = get_parent_block(parent);
            bool parent_is_right = parent == get_right_block(grandparent);
            if (parent_is_right) {
                if ((get_left_block(grandparent) == nullptr || is_black(get_left_block(grandparent)))) {
                    make_red(grandparent);
                    if (is_left_child) {
                        big_left_rotate(grandparent);
                    } else {
                        small_left_rotate(grandparent);
                    }
                    make_black(get_parent_block(grandparent));
                    trace_with_guard("balance_after_insert completed");
                    break;
                } else if (is_red(get_left_block(grandparent))) {
                    make_red(grandparent);
                    make_black(get_left_block(grandparent));
                    make_black(get_right_block(grandparent));
                    current_block = grandparent;
                }
            } else {
                if ((get_right_block(grandparent) == nullptr || is_black(get_right_block(grandparent)))) {
                    make_red(grandparent);
                    if (is_left_child) {
                        small_right_rotate(grandparent);
                    } else {
                        big_right_rotate(grandparent);
                    }
                    small_right_rotate(grandparent);
                    make_black(get_parent_block(grandparent));
                    trace_with_guard("balance_after_insert completed");
                    break;
                } else if (is_red(get_right_block(grandparent))) {
                    make_red(grandparent);
                    make_black(get_right_block(grandparent));
                    make_black(get_left_block(grandparent));
                    current_block = grandparent;
                }
            }
        }
        if (get_parent_block(current_block) == nullptr) {
            make_black(current_block);
            trace_with_guard("balance_after_insert completed");
            break;
        }
        parent = get_parent_block(current_block);
        is_left_child = (get_left_block(parent) == current_block);
    }
}

void allocator_red_black_tree::erase_from_tree(void *current_block) noexcept {
    trace_with_guard("erase_from_tree called");
    void* parent = nullptr;
    bool has_right = get_right_block(current_block) == nullptr;
    bool has_left = get_left_block(current_block) == nullptr;
    if (has_left && has_right) {
        parent = get_parent_block(current_block);
        if (get_parent_block(current_block) == nullptr) {
            *get_first_ptr(_trusted_memory) = nullptr;
        } else if (current_block == get_left_block(get_parent_block(current_block))) {
            get_left_block(get_parent_block(current_block)) = nullptr;
        } else {
            get_right_block(get_parent_block(current_block)) = nullptr;
        }
        if (is_black(current_block)) {
            balance_after_erase(parent);
        }
    } else if (has_left || has_right) {
        void* replacement = get_right_block(current_block) != nullptr ? get_right_block(current_block) : get_left_block(current_block);
        make_black(replacement);
        if (get_parent_block(current_block) == nullptr) {
            *get_first_ptr(_trusted_memory) = replacement;
        } else if (current_block == get_left_block(get_parent_block(current_block))) {
            get_left_block(get_parent_block(current_block)) = replacement;
        } else {
            get_right_block(get_parent_block(current_block)) = replacement;
        }
        get_parent_block(replacement) = get_parent_block(current_block);
    } else {
        void* replacement = get_left_block(current_block);
        while (get_right_block(replacement) != nullptr) {
            replacement = get_right_block(replacement);
        }
        bool bad_case = get_left_block(replacement) == nullptr && is_black(replacement);
        parent = get_parent_block(replacement);
        if (is_black(replacement) && get_left_block(replacement) != nullptr) {
            make_black(get_left_block(replacement));
        }
        if (get_parent_block(current_block) == nullptr) {
            *get_first_ptr(_trusted_memory) = replacement;
        } else if (current_block == get_left_block(get_parent_block(current_block))) {
            get_left_block(get_parent_block(current_block)) = replacement;
        } else {
            get_right_block(get_parent_block(current_block)) = replacement;
        }
        get_right_block(replacement) = get_right_block(current_block);
        get_parent_block(get_right_block(replacement)) = replacement;
        if (get_parent_block(replacement) != current_block) {
            if (get_parent_block(replacement) == nullptr) {
                *get_first_ptr(_trusted_memory) = get_left_block(replacement);
            } else if (replacement == get_left_block(get_parent_block(replacement))) {
                get_left_block(get_parent_block(replacement)) = get_left_block(replacement);
            } else {
                get_right_block(get_parent_block(replacement)) = get_left_block(replacement);
            }
            if (get_left_block(replacement) != nullptr) {
                get_parent_block(get_left_block(replacement)) = get_parent_block(replacement);
            }
            get_left_block(replacement) = get_left_block(current_block);
            get_parent_block(get_left_block(replacement)) = replacement;
        } else {
            parent = replacement;
        }
        if (is_red(current_block)) {
            make_red(replacement);
        } else {
            make_black(replacement);
        }
        get_parent_block(replacement) = get_parent_block(current_block);
        if (bad_case) {
            balance_after_erase(parent);
        }
    }
    trace_with_guard("erase_from_tree completed");
}

void allocator_red_black_tree::balance_after_erase(void *parent, void *current) noexcept {
    trace_with_guard("balance_after_erase called");
    if (parent == nullptr) {
        if (current != nullptr) {
            make_black(current);
        }
        trace_with_guard("balance_after_erase completed");
        return;
    }
    bool current_is_left = (get_left_block(parent) == current);
    void* brother = current_is_left ? get_right_block(parent) : get_left_block(parent);
    if (is_black(brother)) {
        void* far_nephew = current_is_left ? get_right_block(brother) : get_left_block(brother);
        void* near_nephew = current_is_left ? get_left_block(brother) : get_right_block(brother);
        if (far_nephew != nullptr && is_red(far_nephew)) {
            if (current_is_left) {
                small_left_rotate(parent);
            } else {
                small_right_rotate(parent);
            }
            if (is_red(parent)) {
                make_red(brother);
            } else {
                make_black(brother);
            }
            make_black(parent);
            make_black(far_nephew);
        } else if (near_nephew != nullptr && is_red(near_nephew)) {
            if (current_is_left) {
                big_left_rotate(parent);
            } else {
                big_right_rotate(parent);
            }
            if (is_red(parent)) {
                make_red(near_nephew);
            } else {
                make_black(near_nephew);
            }
            make_black(parent);
        } else {
            make_red(brother);
            if (is_red(parent)) {
                make_black(parent);
            } else {
                balance_after_erase(get_parent_block(parent), parent);
            }
        }
    } else if (is_red(brother)) {
        if (current_is_left) {
            small_left_rotate(parent);
        } else {
            small_right_rotate(parent);
        }
        make_red(parent);
        make_black(brother);
        balance_after_erase(parent, current);
    }
    trace_with_guard("balance_after_erase completed");
}

//endregion

//region block modifier functions

void allocator_red_black_tree::merge_blocks(void *real_block) {
    trace_with_guard("merge_blocks called");
    if (get_previous_block(real_block) != nullptr && !(*reinterpret_cast<block_data*>(get_previous_block(real_block))).occupied) {
        void* temp = real_block;
        real_block = get_previous_block(real_block);
        erase_from_tree(real_block);
        get_next_block(real_block) = get_next_block(temp);
        if (get_next_block(real_block) != nullptr) {
            get_previous_block(get_next_block(real_block)) = real_block;
        }
    }
    if (get_next_block(real_block) != nullptr && !(*reinterpret_cast<block_data*>(get_next_block(real_block))).occupied) {
        void* temp = get_next_block(real_block);
        erase_from_tree(temp);
        get_next_block(real_block) = get_next_block(temp);
        if (get_next_block(real_block) != nullptr) {
            get_previous_block(get_next_block(real_block)) = real_block;
        }
    }
    trace_with_guard("merge_blocks completed");
}

void allocator_red_black_tree::split_block(void *block, size_t size) {
    trace_with_guard("split_block called");
    void* new_block = reinterpret_cast<std::byte*>(block) + occupied_block_metadata_size + size;
    get_next_block(new_block) = get_next_block(block);
    get_previous_block(new_block) = block;
    get_next_block(block) = new_block;
    if (get_next_block(new_block) != nullptr) {
        get_previous_block(get_next_block(new_block)) = new_block;
    }
    (*reinterpret_cast<block_data*>(new_block)).occupied = false;
    get_parent_block(new_block) = nullptr;
    insert_in_tree(new_block);
    trace_with_guard("split_block completed");
}

void allocator_red_black_tree::make_black(void *block) const noexcept {
    (*reinterpret_cast<block_data*>(block)).color = block_color::BLACK;
}

void allocator_red_black_tree::make_red(void *block) const noexcept {
    (*reinterpret_cast<block_data*>(block)).color = block_color::RED;
}

//endregion