#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H

#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <pp_allocator.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <iterator>
#include <mutex>

class allocator_boundary_tags final :
        public smart_mem_resource,
        public allocator_test_utils,
        public allocator_with_fit_mode,
        private logger_guardant,
        private typename_holder
{

private:

    static constexpr const size_t allocator_metadata_size = sizeof(logger*) + sizeof(memory_resource*) + sizeof(allocator_with_fit_mode::fit_mode) +
                                                            sizeof(size_t) + sizeof(std::mutex) + sizeof(void*);

    static constexpr const size_t occupied_block_metadata_size = sizeof(size_t) + sizeof(void*) + sizeof(void*) + sizeof(void*);

    static constexpr const size_t free_block_metadata_size = 0;

    void *_trusted_memory;

public:

    ~allocator_boundary_tags() override;

    allocator_boundary_tags(allocator_boundary_tags const &other) = delete;

    allocator_boundary_tags &operator=(allocator_boundary_tags const &other) = delete;

    allocator_boundary_tags(
            allocator_boundary_tags &&other) noexcept;

    allocator_boundary_tags &operator=(
            allocator_boundary_tags &&other) noexcept;

public:

    explicit allocator_boundary_tags(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            logger *logger = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

public:

    [[nodiscard]] void *do_allocate_sm(
            size_t bytes) override;

    void do_deallocate_sm(
            void *at) override;

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

public:

    inline void set_fit_mode(
            allocator_with_fit_mode::fit_mode mode) override;

public:

    std::vector<allocator_test_utils::block_info> get_blocks_info() const override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;


    inline logger *get_logger() const override;

    inline std::string get_typename() const noexcept override;

    [[nodiscard]] inline fit_mode get_fit_mode() const noexcept;

    static inline std::string fit_mode_to_string(fit_mode mode) noexcept;

    [[nodiscard]] inline size_t get_global_size() const noexcept;

    [[nodiscard]] inline std::byte* get_first_block() const noexcept;

    [[nodiscard]] inline void** get_first_occupied() const noexcept;

    [[nodiscard]] inline void* get_block_parent(void* ptr) const noexcept;

    [[nodiscard]] inline std::byte* get_end_of_memory() const noexcept;

    [[nodiscard]] inline size_t get_free_size(const std::vector<allocator_test_utils::block_info>& blocks) const noexcept;

    static inline void* get_next_block(void* ptr) noexcept;

    static inline void* get_previous_block(void* ptr) noexcept;

    size_t get_size_block(void* ptr) const noexcept;

    inline std::byte* get_end_of_block(void* block) const noexcept;

    [[nodiscard]] std::mutex& get_mutex() const noexcept;

    [[nodiscard]] std::pmr::memory_resource* get_parent_allocator() const noexcept;

    void* allocate_first_block(void* address, size_t size);

    void* allocate_between_blocks(void* address, size_t size, void* prev, void* next, size_t size_free);

    [[nodiscard]] static std::string blocks_to_string(const std::vector<allocator_test_utils::block_info>& blocks) ;


private:

    void* allocate_first_fit(size_t full_size);

    void* allocate_best_fit(size_t full_size);

    void* allocate_worst_fit(size_t full_size);

    void* allocate_with_mode(size_t full_size);
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BOUNDARY_TAGS_H