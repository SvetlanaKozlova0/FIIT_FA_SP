#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <mutex>
#include <cmath>

namespace __detail
{
    constexpr size_t nearest_greater_k_of_2(size_t size) noexcept
    {
        int ones_counter = 0, index = -1;

        constexpr const size_t o = 1;

        for (int i = sizeof(size_t) * 8 - 1; i >= 0; --i)
        {
            if (size & (o << i))
            {
                if (ones_counter == 0)
                    index = i;
                ++ones_counter;
            }
        }

        return ones_counter <= 1 ? index : index + 1;
    }
}

class allocator_buddies_system final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode,
    private logger_guardant,
    private typename_holder
{

private:


    struct block_metadata
    {
        bool occupied : 1;
        unsigned char size : 7;
    };

    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size = sizeof(logger*) + sizeof(std::pmr::memory_resource*) + sizeof(fit_mode) + sizeof(unsigned char) + sizeof(std::mutex);

    static constexpr const size_t occupied_block_metadata_size = sizeof(block_metadata) + sizeof(void*);

    static constexpr const size_t free_block_metadata_size = sizeof(block_metadata);

    static constexpr const size_t min_k = __detail::nearest_greater_k_of_2(occupied_block_metadata_size);

public:

    explicit allocator_buddies_system(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            logger *logger = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

    allocator_buddies_system(
        allocator_buddies_system const &other) = delete;
    
    allocator_buddies_system &operator=(
        allocator_buddies_system const &other) = delete;
    
    allocator_buddies_system(
        allocator_buddies_system &&other) noexcept;
    
    allocator_buddies_system &operator=(
        allocator_buddies_system &&other) noexcept;

    ~allocator_buddies_system() override;

public:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;


    [[nodiscard]] std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    [[nodiscard]] std::string get_string_info(const std::vector<allocator_test_utils::block_info>& info) const noexcept;

    [[nodiscard]] inline logger *get_logger() const override;

    inline std::mutex& get_mutex();
    
    [[nodiscard]] inline std::string get_typename() const override;

    inline static size_t get_block_size(void* ptr);

    inline static bool is_block_occupied(void* ptr);

    [[nodiscard]] inline size_t get_global_size() const;

    [[nodiscard]] inline fit_mode get_fit_mode() const;

    [[nodiscard]] inline std::pmr::memory_resource* get_parent_allocator() const;

    [[nodiscard]] void* allocate_first_fit(size_t need_size) const noexcept;

    [[nodiscard]] void* allocate_best_first(size_t need_size) const noexcept;

    [[nodiscard]] void* allocate_worst_fit(size_t need_size) const noexcept;

    [[nodiscard]] std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    [[nodiscard]] std::string fit_mode_to_string(fit_mode mode) const;

    void* allocate_with_mode(size_t size);

    void fill_info_about_block(void* ptr);

    void split_blocks(void* ptr, size_t need_size);

    void merge_blocks(void* ptr);

    void* get_buddy(void* ptr);

    [[nodiscard]] std::byte* get_first_block() const;

    bool can_split_block(void* ptr, size_t need_size) const;

    bool can_merge_block(void* occupied_block, void* buddy) const;

    void debug_start_message(const std::string& message);

    void debug_complete_message(const std::string& message);

    void trace_start_message(const std::string& message);

    void trace_complete_message(const std::string& message);

    void* get_parent_block(void* block) const;

    void* get_real_address_block(void* block) const;

    class buddy_iterator
    {
        void* _block;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const buddy_iterator&) const noexcept;

        bool operator!=(const buddy_iterator&) const noexcept;

        buddy_iterator& operator++() & noexcept;

        buddy_iterator operator++(int n);

        [[nodiscard]] size_t size() const noexcept;

        [[nodiscard]] bool occupied() const noexcept;

        void* operator*() const noexcept;

        buddy_iterator();

        [[nodiscard]] explicit buddy_iterator(void* start);
    };

    friend class buddy_iterator;

    [[nodiscard]] buddy_iterator begin() const noexcept;

    [[nodiscard]] buddy_iterator end() const noexcept;
    
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
