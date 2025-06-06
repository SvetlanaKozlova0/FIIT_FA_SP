#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H

#include <pp_allocator.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <mutex>

class allocator_red_black_tree final:
    public smart_mem_resource,
    public allocator_test_utils,
    public allocator_with_fit_mode,
    private logger_guardant,
    private typename_holder
{

private:

    enum class block_color : unsigned char
    { RED, BLACK };

    struct block_data
    {
        bool occupied : 4;
        block_color color : 4;
    };

    void *_trusted_memory;

    static constexpr const size_t allocator_metadata_size = sizeof(logger*) + sizeof(std::pmr::memory_resource *) + sizeof(fit_mode) + sizeof(size_t) + sizeof(std::mutex) + sizeof(void*);
    static constexpr const size_t occupied_block_metadata_size = sizeof(block_data) + 3 * sizeof(void*);
    static constexpr const size_t free_block_metadata_size = sizeof(block_data) + 5 * sizeof(void*);

public:
    
    ~allocator_red_black_tree() override;
    
    allocator_red_black_tree(
        allocator_red_black_tree const &other) = delete;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree const &other) = delete;
    
    allocator_red_black_tree(
        allocator_red_black_tree &&other) noexcept;
    
    allocator_red_black_tree &operator=(
        allocator_red_black_tree &&other) noexcept;

public:
    
    explicit allocator_red_black_tree(
            size_t space_size,
            std::pmr::memory_resource *parent_allocator = nullptr,
            logger *logger = nullptr,
            allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

public:
    
    [[nodiscard]] void *do_allocate_sm(
        size_t size) override;
    
    void do_deallocate_sm(
        void *at) override;

    bool do_is_equal(const std::pmr::memory_resource&) const noexcept override;

    std::vector<allocator_test_utils::block_info> get_blocks_info() const override;
    
    inline void set_fit_mode(allocator_with_fit_mode::fit_mode mode) override;

    inline logger *get_logger() const override;

private:

    std::vector<allocator_test_utils::block_info> get_blocks_info_inner() const override;

    inline std::string get_typename() const noexcept override;

    class rb_iterator
    {
        void* _block_ptr;
        void* _trusted;

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = void*;
        using reference = void*&;
        using pointer = void**;
        using difference_type = ptrdiff_t;

        bool operator==(const rb_iterator&) const noexcept;

        bool operator!=(const rb_iterator&) const noexcept;

        rb_iterator& operator++() & noexcept;

        rb_iterator operator++(int n);

        size_t size() const noexcept;

        void* operator*() const noexcept;

        bool occupied()const noexcept;

        rb_iterator();

        rb_iterator(void* trusted);
    };

    friend class rb_iterator;

    rb_iterator begin() const noexcept;
    rb_iterator end() const noexcept;

private:
    inline std::mutex& get_mutex() const noexcept;
    [[nodiscard]] inline std::pmr::memory_resource* get_parent_allocator() const;
    [[nodiscard]] inline size_t get_global_size() const;
    static void*& get_next_block(void* current_block) ;
    static void*& get_previous_block(void* current_block) ;
    static void*& get_parent_block(void* current_block) ;
    static void*& get_left_block(void* current_block) ;
    static void*& get_right_block(void* current_block) ;
    static void** get_first_ptr(void* trusted_mem) noexcept;
    void small_left_rotate(void* block) noexcept;
    void small_right_rotate(void* block) noexcept;
    void big_left_rotate(void* block) noexcept;
    void big_right_rotate(void* block) noexcept;
    size_t get_block_size(void* block) const noexcept;
    void insert_in_tree(void* current_block) noexcept;
    void balance_after_erase(void* parent, void* deleted = nullptr) noexcept;
    void erase_from_tree(void* current_block) noexcept;
    size_t get_all_free_size() const noexcept;
    std::string get_info_in_string(const std::vector<allocator_test_utils::block_info>& ve) noexcept;
    void* allocate_best_fit(size_t size) const noexcept;
    void* allocate_worst_fit(size_t size) const noexcept;
    void* allocate_first_fit(size_t size) const noexcept;
    fit_mode get_fit_mode() const noexcept;
    void* allocate_with_mode(size_t need_size) const noexcept;
    void make_black(void* block) const noexcept;
    void make_red(void* block) const noexcept;
    bool is_red(void* block) const noexcept;
    bool is_black(void* block) const noexcept;
    void balance_after_insert(void* current_block, void* parent) noexcept;
    void split_block(void* block, size_t size) ;
    void merge_blocks(void* real_block);
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_RED_BLACK_TREE_H