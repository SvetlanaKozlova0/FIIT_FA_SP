#include <not_implemented.h>
#include "../include/allocator_global_heap.h"

void allocator_global_heap::mark_called_trace_debug(const std::string& message, logger* logg) {
    if (logg == nullptr) return;
    logg->log(message + " called", logger::severity::debug);
    logg->log(message + " called", logger::severity::trace);
}

void allocator_global_heap::mark_completed_trace_debug(const std::string& message, logger* logg) {
    if (logg == nullptr) return;
    logg->log(message + " completed", logger::severity::debug);
    logg->log(message + " completed", logger::severity::trace);
}

allocator_global_heap::allocator_global_heap(
    logger *logger): _logger(logger)
{
    mark_called_trace_debug("allocator_global_heap(logger *)", logger);
    mark_completed_trace_debug("constructor allocator_global_heap(logger *)", _logger);
}

[[nodiscard]] void *allocator_global_heap::do_allocate_sm(
    size_t size)
{
    if (size == 0) {
        return nullptr;
    }
    mark_called_trace_debug("do_allocate_sm(" + std::to_string(size) + " bytes)", _logger);
    size_t total_size = size + sizeof(size_t);
    void* raw_ptr = ::operator new(total_size);
    if (raw_ptr == nullptr) {
        if (_logger)
            _logger->log("bad alloc() when do_allocate_sm() called", logger::severity::error);
        throw std::bad_alloc();
    }
    auto meta_data = static_cast<size_t*>(raw_ptr);
    *meta_data = size;
    void* user_ptr = static_cast<void*>(meta_data + 1);
    mark_completed_trace_debug("do_allocate_sm(" + std::to_string(size) + " bytes)", _logger);
    return user_ptr;
}

void allocator_global_heap::do_deallocate_sm(
    void *at)
{
    mark_called_trace_debug("do_deallocate_sm(void *at)", _logger);
    if (at == nullptr) {
        if (_logger)
            _logger->log("nullptr passed in do_deallocate_sm()", logger::severity::error);
        mark_completed_trace_debug("do_deallocate_sm( nullptr )", _logger);
        return;
    }
    char* ptr = static_cast<char*>(at);
    ptr -= sizeof(size_t);
    size_t size_to_dealloc = *reinterpret_cast<size_t*>(ptr);
    void* orig = static_cast<void*>(ptr);
    ::operator delete(orig);
    mark_completed_trace_debug("do_deallocate_sm(" + std::to_string(size_to_dealloc) + " bytes)", _logger);
}

allocator_global_heap::~allocator_global_heap()
{
    mark_called_trace_debug("destructor ~allocator_global_heap()", _logger);
    mark_completed_trace_debug("destructor ~allocator_global_heap()", _logger);
}

allocator_global_heap::allocator_global_heap(const allocator_global_heap &other) : _logger(other.get_logger())
{
    mark_called_trace_debug("allocator_global_heap(const allocator_global_heap &other)", _logger);
    mark_completed_trace_debug("allocator_global_heap(const allocator_global_heap &other)", _logger);
}

allocator_global_heap &allocator_global_heap::operator=(const allocator_global_heap &other)
{
    mark_called_trace_debug("operator=(const allocator_global_heap &other)", _logger);
    if (&other == this) {
        if (_logger)
            _logger->log("self-sealing in operator=(const allocator_global_heap &other)", logger::severity::error);
        mark_completed_trace_debug("operator=(const allocator_global_heap &other)", _logger);
        return *this;
    }
    _logger = other._logger;
    mark_completed_trace_debug("operator=(const allocator_global_heap &other)", _logger);
    return *this;
}

bool allocator_global_heap::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return (typeid(*this) == typeid(other));
}

allocator_global_heap::allocator_global_heap(allocator_global_heap &&other) noexcept : _logger(other._logger)
{
    mark_called_trace_debug("allocator_global_heap(allocator_global_heap &&other)", _logger);
    other._logger = nullptr;
    mark_completed_trace_debug("allocator_global_heap(allocator_global_heap &&other)", _logger);
}

allocator_global_heap &allocator_global_heap::operator=(allocator_global_heap &&other) noexcept
{
    mark_called_trace_debug("operator=(allocator_global_heap &&other)", _logger);
    if (&other == this) {
        if (_logger)
            _logger->log("self-sealing in operator=(allocator_global_heap &&other)", logger::severity::error);
        mark_completed_trace_debug("operator=(allocator_global_heap &&other)", _logger);
        return *this;
    }
    _logger = other._logger;
    other._logger = nullptr;
    mark_completed_trace_debug("operator=(allocator_global_heap &&other)", _logger);
    return *this;
}
