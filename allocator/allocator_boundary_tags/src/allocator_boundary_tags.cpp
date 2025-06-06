#include "../include/allocator_boundary_tags.h"

allocator_boundary_tags::~allocator_boundary_tags() {
    debug_with_guard("destructor called");
    if (_trusted_memory == nullptr) {
        return;
    }
    get_mutex().~mutex();
    std::pmr::memory_resource *parent_alloc = get_parent_allocator();
    parent_alloc->deallocate(_trusted_memory, get_global_size() + allocator_metadata_size);
    _trusted_memory = nullptr;
}

allocator_boundary_tags::allocator_boundary_tags(
        allocator_boundary_tags &&other) noexcept {
    debug_with_guard("move constructor called");
    std::lock_guard<std::mutex> lock(other.get_mutex());
    _trusted_memory = std::exchange(other._trusted_memory, nullptr);
    debug_with_guard("move constructor completed");
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
        allocator_boundary_tags &&other) noexcept {
    debug_with_guard("move assign operator called");
    std::lock_guard<std::mutex> lock(get_mutex());
    std::swap(_trusted_memory, other._trusted_memory);
    debug_with_guard("move assign operator completed");
    return *this;
}

allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        logger *log,
        allocator_with_fit_mode::fit_mode allocate_fit_mode) {
    if (log != nullptr) {
        log->debug("constructor started");
    }
    if (space_size == 0) {
        if (log != nullptr) {
            log->error("an attempt to initialize the allocator's working memory of size 0");
        }
        throw std::logic_error("an attempt to initialize the allocator's working memory of size 0");
    }
    size_t total_size = space_size + allocator_metadata_size;
    if (parent_allocator != nullptr) {
        _trusted_memory = parent_allocator->allocate(total_size);
        if (_trusted_memory == nullptr) {
            if (log != nullptr) {
                log->error("error while allocating memory for allocator by parent allocator");
            }
            throw std::bad_alloc();
        }
    } else {
        try {
            _trusted_memory = std::pmr::get_default_resource()->allocate(total_size);
        }
        catch (const std::exception &ex) {
            if (log != nullptr) {
                log->error("exception while allocating memory for allocator by default pmr resource");
            }
            throw ex;
        }
    }
    auto *metadata = reinterpret_cast<std::byte *>(_trusted_memory);

    *reinterpret_cast<logger **>(metadata) = log;
    metadata += sizeof(logger *);

    *reinterpret_cast<std::pmr::memory_resource **>(metadata) = parent_allocator ? parent_allocator
                                                                                 : std::pmr::get_default_resource();
    metadata += sizeof(std::pmr::memory_resource *);

    *reinterpret_cast<fit_mode *>(metadata) = allocate_fit_mode;
    metadata += sizeof(fit_mode);

    *reinterpret_cast<size_t *>(metadata) = space_size;
    metadata += sizeof(size_t);

    auto* mutex_ptr = reinterpret_cast<std::mutex*>(metadata);
    new (mutex_ptr) std::mutex();
    metadata += sizeof(std::mutex);

    *reinterpret_cast<void **>(metadata) = nullptr;
    debug_with_guard("constructor completed");
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
        size_t need_size) {
    std::lock_guard<std::mutex> lock(get_mutex());
    debug_with_guard("allocation of " + std::to_string(need_size) + " bytes started");
    auto blocks = get_blocks_info_inner();
    information_with_guard("memory before allocation:   " + blocks_to_string(blocks));
    information_with_guard("available free memory: " + std::to_string(get_free_size(blocks)));
    void *ptr = allocate_with_mode(need_size);
    if (ptr == nullptr) {
        error_with_guard("bad alloc while allocating of" + std::to_string(need_size) + " bytes");
        throw std::bad_alloc();
    }
    if (get_size_block(ptr) != need_size) {
        warning_with_guard("allocator changed size to " + std::to_string(get_size_block(ptr)));
    }
    debug_with_guard("allocation of " + std::to_string(need_size) + " bytes completed");
    blocks = get_blocks_info_inner();
    information_with_guard("memory after allocation:    " + blocks_to_string(blocks));
    information_with_guard("available free memory: " + std::to_string(get_free_size(blocks)));
    return reinterpret_cast<std::byte *>(ptr) + occupied_block_metadata_size;
}

void allocator_boundary_tags::do_deallocate_sm(
        void *at) {
    debug_with_guard("deallocating started");
    if (at == nullptr) {
        warning_with_guard("deallocating null-pointer");
        return;
    }
    std::lock_guard<std::mutex> lock(get_mutex());
    auto blocks = get_blocks_info_inner();
    information_with_guard("memory before deallocating: " + blocks_to_string(blocks));
    information_with_guard("available free memory: " + std::to_string(get_free_size(blocks)));
    at = reinterpret_cast<std::byte *>(at) - occupied_block_metadata_size;
    if (get_block_parent(at) != _trusted_memory) {
        warning_with_guard("this block doesn't belong to this allocator");
        return;
    }
    std::byte *first_block = get_first_block();
    void *next_block = get_next_block(at);
    void *previous_block = get_previous_block(at);
    if (previous_block) {
        *reinterpret_cast<void **>(reinterpret_cast<std::byte *>(previous_block) + sizeof(size_t)) = next_block;
    } else {
        *reinterpret_cast<void **>(first_block - sizeof(void *)) = next_block;
    }
    if (next_block != nullptr) {
        *reinterpret_cast<void **>(reinterpret_cast<std::byte *>(next_block) + sizeof(size_t) +
                                   sizeof(void *)) = previous_block;
    }
    blocks = get_blocks_info_inner();
    debug_with_guard("deallocating completed");
    information_with_guard("memory after deallocating:  " + blocks_to_string(blocks));
    information_with_guard("available free memory: " + std::to_string(get_free_size(blocks)));
}

inline void allocator_boundary_tags::set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) {
    debug_with_guard("method 'set_fit_mode' called");
    debug_with_guard("chosen fit mode: " + fit_mode_to_string(mode));
    auto *metadata = reinterpret_cast<std::byte *>(_trusted_memory);
    auto *fit_mode_ptr = metadata + sizeof(logger *) + sizeof(std::pmr::memory_resource *);
    *reinterpret_cast<fit_mode *>(fit_mode_ptr) = mode;
    debug_with_guard("method 'set_fit_mode' completed");
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const {
    std::lock_guard<std::mutex> lock(get_mutex());
    return get_blocks_info_inner();
}


void *allocator_boundary_tags::allocate_with_mode(size_t need_size) {
    trace_with_guard("method 'allocate_with_mode' called and completed");
    switch (get_fit_mode()) {
        case fit_mode::the_best_fit:
            return allocate_best_fit(need_size);
        case fit_mode::first_fit:
            return allocate_first_fit(need_size);
        case fit_mode::the_worst_fit:
            return allocate_worst_fit(need_size);
    }
}

void* allocator_boundary_tags::allocate_first_fit(size_t need_size) {
    trace_with_guard("method 'allocate_first_fit' called");
    size_t full_size = need_size + occupied_block_metadata_size;
    void** first_block_ptr = get_first_occupied();
    std::byte* memory_start = get_first_block();
    std::byte* memory_end = get_end_of_memory();

    if (*first_block_ptr == nullptr) {
        trace_with_guard("method 'allocate_first_fit' completed");
        return allocate_first_block(memory_start, need_size);
    }

    auto* first_block = static_cast<std::byte*>(*first_block_ptr);
    size_t front_space = first_block - memory_start;

    if (front_space >= full_size) {
        trace_with_guard("method 'allocate_first_fit' completed");
        return allocate_between_blocks(memory_start, need_size,
                                       nullptr, *first_block_ptr, front_space);
    }

    for (void* current = *first_block_ptr; current != nullptr; current = get_next_block(current)) {
        std::byte* current_end = get_end_of_block(current);
        void* next_block = get_next_block(current);
        if (next_block == nullptr) {
            size_t end_space = memory_end - current_end;
            if (end_space >= full_size) {
                trace_with_guard("method 'allocate_first_fit' completed");
                return allocate_between_blocks(current_end, need_size,
                                               current, nullptr, end_space);
            }
            trace_with_guard("method 'allocate_first_fit' completed");
            return nullptr;
        }

        size_t middle_space = static_cast<std::byte*>(next_block) - current_end;
        if (middle_space >= full_size) {
            trace_with_guard("method 'allocate_first_fit' completed");
            return allocate_between_blocks(current_end, need_size,
                                           current, next_block, middle_space);
        }
    }
    trace_with_guard("method 'allocate_first_fit' completed");
    return nullptr;
}

void* allocator_boundary_tags::allocate_best_fit(size_t need_size) {
    trace_with_guard("method 'allocate_best_fit' called");
    size_t full_size = need_size + occupied_block_metadata_size;
    void** first_block_ptr = get_first_occupied();

    if (*first_block_ptr == nullptr) {
        trace_with_guard("method 'allocate_best_fit' completed");
        return allocate_first_block(get_first_block(), need_size);
    }

    void* best_ptr = nullptr;
    void* best_prev = nullptr;
    void* best_next = nullptr;
    size_t best_fit_size = get_global_size();

    std::byte* memory_start = get_first_block();
    auto* first_block_start = static_cast<std::byte*>(*first_block_ptr);
    size_t front_space = first_block_start - memory_start;

    if (front_space >= full_size && front_space < best_fit_size) {
        best_ptr = memory_start;
        best_prev = nullptr;
        best_next = *first_block_ptr;
        best_fit_size = front_space;
    }

    for (void* current = *first_block_ptr; current != nullptr; current = get_next_block(current)) {
        std::byte* current_end = get_end_of_block(current);
        void* next_block = get_next_block(current);
        std::byte* next_block_start = (next_block != nullptr) ? static_cast<std::byte*>(next_block)
                                                              :get_end_of_memory();
        size_t available_space = next_block_start - current_end;
        if (available_space >= full_size && available_space < best_fit_size) {
            best_ptr = current_end;
            best_prev = current;
            best_next = next_block;
            best_fit_size = available_space;
            if (best_fit_size == full_size) {
                break;
            }
        }
    }
    trace_with_guard("method 'allocate_best_fit' completed");
    return (best_ptr != nullptr) ?
            allocate_between_blocks(best_ptr, need_size, best_prev, best_next, best_fit_size)
            : nullptr;
}

void* allocator_boundary_tags::allocate_worst_fit(size_t need_size) {
    trace_with_guard("method 'allocate_worst_fit' called");
    size_t full_size = need_size + occupied_block_metadata_size;
    void** first_block_ptr = get_first_occupied();

    if (*first_block_ptr == nullptr) {
        trace_with_guard("method 'allocate_worst_fit' completed");
        return allocate_first_block(get_first_block(), need_size);
    }

    void*  worst_ptr = nullptr;
    void*  worst_prev = nullptr;
    void*  worst_next = nullptr;
    size_t worst_size = 0;

    std::byte* memory_start = get_first_block();
    auto* first_block_start = static_cast<std::byte*>(*first_block_ptr);
    size_t front_space = first_block_start - memory_start;

    if (front_space >= full_size) {
        worst_ptr = memory_start;
        worst_prev = nullptr;
        worst_next = *first_block_ptr;
        worst_size = front_space;
    }

    for (void* current = *first_block_ptr; current != nullptr; current = get_next_block(current)) {
        std::byte* current_end = get_end_of_block(current);
        void* next_block = get_next_block(current);
        std::byte* next_block_start = next_block
                                                  ? static_cast<std::byte*>(next_block)
                                                  : get_end_of_memory();

        size_t available_space = next_block_start - current_end;
        if (available_space >= full_size && available_space > worst_size) {
            worst_ptr = current_end;
            worst_prev = current;
            worst_next = next_block;
            worst_size = available_space;
        }
    }
    trace_with_guard("method 'allocate_worst_fit' completed");
    return worst_ptr
           ? allocate_between_blocks(worst_ptr, need_size, worst_prev, worst_next, worst_size)
           : nullptr;
}

void *
allocator_boundary_tags::allocate_first_block(void *address, size_t size) {
    trace_with_guard("method 'allocate_first_block' called");
    if (get_end_of_memory() - get_first_block() < size + occupied_block_metadata_size) {
        trace_with_guard("method 'allocate_first_block' completed");
        return nullptr;
    }
    auto* byte_address = reinterpret_cast<std::byte *>(address);
    *reinterpret_cast<size_t *>(byte_address) = size;
    *reinterpret_cast<void **>(byte_address + sizeof(size_t)) = nullptr;
    *reinterpret_cast<void **>(byte_address + sizeof(size_t) + sizeof(void *)) = nullptr;
    *reinterpret_cast<void **>(byte_address + sizeof(size_t) + sizeof(void *) * 2) = _trusted_memory;
    *get_first_occupied() = address;
    trace_with_guard("method 'allocate_first_block' completed");
    return address;
}

void *
allocator_boundary_tags::allocate_between_blocks(void *address, size_t size, void *prev, void *next,
                                                 size_t size_free) {
    trace_with_guard("method 'allocate_between_blocks' called");
    if (size_free - size - occupied_block_metadata_size < occupied_block_metadata_size) {
        size = size_free - occupied_block_metadata_size;
    }
    auto *byte_address = static_cast<std::byte *>(address);
    *reinterpret_cast<size_t *>(byte_address) = size;
    *reinterpret_cast<void **>(byte_address + sizeof(size_t)) = next;
    *reinterpret_cast<void **>(byte_address + sizeof(size_t) + sizeof(void *)) = prev;
    *reinterpret_cast<void **>(byte_address + sizeof(size_t) + sizeof(void *) * 2) = _trusted_memory;
    if (prev != nullptr) {
        *reinterpret_cast<void **>(static_cast<std::byte *>(prev) + sizeof(size_t)) = address;
    } else {
        *get_first_occupied() = address;
    }
    if (next) {
        *reinterpret_cast<void **>(static_cast<std::byte *>(next) + sizeof(size_t) + sizeof(void *)) = address;
    }
    trace_with_guard("method 'allocate_between_blocks' completed");
    return address;
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const {
    std::vector<allocator_test_utils::block_info> result;
    if (_trusted_memory == nullptr) {
        return result;
    }
    std::byte *memory_start = get_first_block();
    std::byte *memory_end = get_end_of_memory();
    void **first_block_ptr = get_first_occupied();
    auto *current_block = reinterpret_cast<std::byte *>(*first_block_ptr);
    std::byte *prev_block_end = memory_start;

    while (current_block != nullptr && current_block < memory_end) {
        if (current_block > prev_block_end) {
            size_t hole_size = current_block - prev_block_end;
            result.push_back({.block_size = hole_size, .is_block_occupied = false});
        }
        size_t block_size = get_size_block(current_block) + occupied_block_metadata_size;
        result.push_back({.block_size = block_size, .is_block_occupied = true});
        prev_block_end = current_block + block_size;
        current_block = reinterpret_cast<std::byte *>(get_next_block(current_block));
    }
    if (prev_block_end < memory_end) {
        size_t hole_size = memory_end - prev_block_end;
        result.push_back({.block_size = hole_size, .is_block_occupied = false});
    }
    return result;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    return typeid(*this) == typeid(other);
}

std::string allocator_boundary_tags::fit_mode_to_string(allocator_with_fit_mode::fit_mode mode) noexcept {
    switch (mode) {
        case fit_mode::first_fit:
            return "first_fit";
        case fit_mode::the_best_fit:
            return "best-fit";
        case fit_mode::the_worst_fit:
            return "worst-fit";
    }
}


//region getter_functions

allocator_with_fit_mode::fit_mode allocator_boundary_tags::get_fit_mode() const noexcept {
    auto *metadata = reinterpret_cast<std::byte *>(_trusted_memory);
    metadata += sizeof(logger *) + sizeof(std::pmr::memory_resource *);
    return *reinterpret_cast<fit_mode *>(metadata);
}

std::pmr::memory_resource *allocator_boundary_tags::get_parent_allocator() const noexcept {
    return *reinterpret_cast<std::pmr::memory_resource **>(reinterpret_cast<std::byte *>(_trusted_memory) +
                                                           sizeof(logger *));
}

size_t allocator_boundary_tags::get_global_size() const noexcept {
    return reinterpret_cast<size_t>(static_cast<char *>(_trusted_memory) +
                                    sizeof(logger *) +
                                    sizeof(memory_resource * ) +
                                    sizeof(allocator_with_fit_mode::fit_mode));
}

std::byte *allocator_boundary_tags::get_first_block() const noexcept {
    return static_cast<std::byte *>(_trusted_memory) + allocator_metadata_size;
}

std::byte *allocator_boundary_tags::get_end_of_memory() const noexcept {
    size_t global_size = *reinterpret_cast<size_t *>(reinterpret_cast<std::byte *>(_trusted_memory) + sizeof(logger *) +
                                                     sizeof(std::pmr::memory_resource *) + sizeof(fit_mode));
    std::byte *end_of_memory = get_first_block() + global_size;
    return end_of_memory;
}

void **allocator_boundary_tags::get_first_occupied() const noexcept {
    void **first_block_ptr = reinterpret_cast<void **>(get_first_block() - sizeof(void *));
    return first_block_ptr;
}

void *allocator_boundary_tags::get_next_block(void *current_block) noexcept {
    return *reinterpret_cast<void **>(reinterpret_cast<std::byte *>(current_block) + sizeof(size_t));
}

void *allocator_boundary_tags::get_previous_block(void *ptr) noexcept {
    return *reinterpret_cast<void **>(reinterpret_cast<std::byte *>(ptr) + sizeof(size_t) + sizeof(void *));
}

inline logger *allocator_boundary_tags::get_logger() const {
    if (_trusted_memory == nullptr) {
        return nullptr;
    }
    return *reinterpret_cast<logger **>(_trusted_memory);
}

inline std::string allocator_boundary_tags::get_typename() const noexcept {
    return "allocator_boundary_tags";
}

std::mutex &allocator_boundary_tags::get_mutex() const noexcept {
    auto metadata = reinterpret_cast<std::byte *>(_trusted_memory);
    metadata += sizeof(logger *) + sizeof(memory_resource * ) + sizeof(fit_mode) + sizeof(size_t);
    return *reinterpret_cast<std::mutex *>(metadata);
}

size_t allocator_boundary_tags::get_size_block(void *ptr) const noexcept {
    return *reinterpret_cast<size_t *>(ptr);
}

std::byte *allocator_boundary_tags::get_end_of_block(void *block) const noexcept {
    return static_cast<std::byte *>(block) + get_size_block(block) + occupied_block_metadata_size;
}

void *allocator_boundary_tags::get_block_parent(void* ptr) const noexcept {
    return *reinterpret_cast<void**>((reinterpret_cast<std::byte*>(ptr) + sizeof(size_t) + 2 * sizeof(void*)));
}

size_t
allocator_boundary_tags::get_free_size(const std::vector<allocator_test_utils::block_info> &blocks) const noexcept {
    size_t free_size = 0;
    for (auto& block: blocks) {
        free_size += (!block.is_block_occupied) ? block.block_size : 0;
    }
    return free_size;
}
//endregion

std::string
allocator_boundary_tags::blocks_to_string(const std::vector<allocator_test_utils::block_info> &blocks) {
    std::string result = "| ";
    for (auto& block: blocks) {
        result += (block.is_block_occupied) ? "<occup>" : "<avail>";
        result += " " + std::to_string(block.is_block_occupied ? block.block_size
                - occupied_block_metadata_size : block.block_size) + " | ";
    }
    return result;
}