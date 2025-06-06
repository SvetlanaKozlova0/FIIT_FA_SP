#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H

#include <binary_search_tree.h>

namespace __detail
{
    class RB_TAG;

    template<typename tkey, typename tvalue, typename compare>
    class bst_impl<tkey, tvalue, compare, RB_TAG>
    {
        static void print_rb(const std::string& prefix, typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node* node, bool is_left)
        {
            if (node == nullptr) {
                return;
            }
            std::cout << prefix << (is_left ? "├──" : "└──" ) << node->data.first << std::endl;
            print_rb(prefix + (is_left ? "│   " : "    "), node->left_subtree, true);
            print_rb(prefix + (is_left ? "│   " : "    "), node->right_subtree, false);
        }

        static void print_rb_full(typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node* node)
        {
            print_rb("", node, false);
        }

        friend class binary_search_tree<tkey, tvalue, compare, RB_TAG>;
        template<class ...Args>
        static binary_search_tree<tkey, tvalue, compare, RB_TAG>::node* create_node(binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, Args&& ...args);

        static void delete_node(binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, binary_search_tree<tkey, tvalue, compare, RB_TAG>::node** node);

        //Does not invalidate node*, needed for splay tree
        static void post_search(binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, binary_search_tree<tkey, tvalue, compare, RB_TAG>::node**){}

        //Does not invalidate node*
        static void post_insert(binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, binary_search_tree<tkey, tvalue, compare, RB_TAG>::node**);

        static void erase(binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, binary_search_tree<tkey, tvalue, compare, RB_TAG>::node**);

        static void swap(binary_search_tree<tkey, tvalue, compare, RB_TAG>& lhs, binary_search_tree<tkey, tvalue, compare, RB_TAG>& rhs) noexcept;
    };
}

template<typename tkey,typename tvalue, compator<tkey> compare = std::less<tkey>>
class red_black_tree final: public binary_search_tree<tkey, tvalue, compare, __detail::RB_TAG>
{

public:
    
    enum class node_color : unsigned char
    {
        RED,
        BLACK
    };

private:

    using parent = binary_search_tree<tkey, tvalue, compare, __detail::RB_TAG>;
    friend class __detail::bst_impl<tkey, tvalue, compare, __detail::RB_TAG>;

    struct node final:
        parent::node
    {
        node_color color;

        template<class ...Args>
        node(parent::node* par, Args&&... args);

        ~node() noexcept override =default;
    };


public:

    using value_type = parent::value_type;

    explicit red_black_tree(
            const compare& comp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger *logger = nullptr);

    explicit red_black_tree(
            pp_allocator<value_type> alloc,
            const compare& comp = compare(),
            logger *logger = nullptr);

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit red_black_tree(iterator begin, iterator end, const compare& cmp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger* logger = nullptr);

    template<std::ranges::input_range Range>
    explicit red_black_tree(Range&& range, const compare& cmp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger* logger = nullptr);


    red_black_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger* logger = nullptr);


    // region iterator definition


    class prefix_iterator : public parent::prefix_iterator
    {
    public:

        using value_type = parent::prefix_iterator::value_type;
        using difference_type = parent::prefix_iterator::difference_type;
        using pointer = parent::prefix_iterator::pointer;
        using reference = parent::prefix_iterator::reference;
        using iterator_category = parent::prefix_iterator::iterator_category;

        explicit prefix_iterator(parent::node* n = nullptr) noexcept;
        prefix_iterator(parent::prefix_iterator) noexcept;

        node_color get_color() const noexcept;

        using parent::prefix_iterator::depth;
        using parent::prefix_iterator::operator*;
        using parent::prefix_iterator::operator==;
        using parent::prefix_iterator::operator!=;
        using parent::prefix_iterator::operator++;
        using parent::prefix_iterator::operator--;
        using parent::prefix_iterator::operator->;
    };

    class prefix_const_iterator : public parent::prefix_const_iterator
    {
    public:

        using value_type = parent::prefix_const_iterator::value_type;
        using difference_type = parent::prefix_const_iterator::difference_type;
        using pointer = parent::prefix_const_iterator::pointer;
        using reference = parent::prefix_const_iterator::reference;
        using iterator_category = parent::prefix_const_iterator::iterator_category;

        explicit prefix_const_iterator(parent::node* n = nullptr) noexcept;
        prefix_const_iterator(parent::prefix_const_iterator) noexcept;

        node_color get_color() const noexcept;

        prefix_const_iterator(prefix_iterator) noexcept;

        using parent::prefix_const_iterator::depth;
        using parent::prefix_const_iterator::operator*;
        using parent::prefix_const_iterator::operator==;
        using parent::prefix_const_iterator::operator!=;
        using parent::prefix_const_iterator::operator++;
        using parent::prefix_const_iterator::operator--;
        using parent::prefix_const_iterator::operator->;
    };

    class prefix_reverse_iterator : public parent::prefix_reverse_iterator
    {
    public:

        using value_type = parent::prefix_reverse_iterator::value_type;
        using difference_type = parent::prefix_reverse_iterator::difference_type;
        using pointer = parent::prefix_reverse_iterator::pointer;
        using reference = parent::prefix_reverse_iterator::reference;
        using iterator_category = parent::prefix_reverse_iterator::iterator_category;

        explicit prefix_reverse_iterator(parent::node* n = nullptr) noexcept;
        prefix_reverse_iterator(parent::prefix_reverse_iterator) noexcept;

        node_color get_color() const noexcept;

        prefix_reverse_iterator(prefix_iterator) noexcept;
        operator prefix_iterator() const noexcept;
        prefix_iterator base() const noexcept;

        using parent::prefix_reverse_iterator::depth;
        using parent::prefix_reverse_iterator::operator*;
        using parent::prefix_reverse_iterator::operator==;
        using parent::prefix_reverse_iterator::operator!=;
        using parent::prefix_reverse_iterator::operator++;
        using parent::prefix_reverse_iterator::operator--;
        using parent::prefix_reverse_iterator::operator->;
    };

    class prefix_const_reverse_iterator : public parent::prefix_const_reverse_iterator
    {
    public:

        using value_type = parent::prefix_const_reverse_iterator::value_type;
        using difference_type = parent::prefix_const_reverse_iterator::difference_type;
        using pointer = parent::prefix_const_reverse_iterator::pointer;
        using reference = parent::prefix_const_reverse_iterator::reference;
        using iterator_category = parent::prefix_const_reverse_iterator::iterator_category;

        explicit prefix_const_reverse_iterator(parent::node* n = nullptr) noexcept;
        prefix_const_reverse_iterator(parent::prefix_const_reverse_iterator) noexcept;

        node_color get_color() const noexcept;

        prefix_const_reverse_iterator(prefix_const_iterator) noexcept;
        operator prefix_const_iterator() const noexcept;
        prefix_const_iterator base() const noexcept;

        using parent::prefix_const_reverse_iterator::depth;
        using parent::prefix_const_reverse_iterator::operator*;
        using parent::prefix_const_reverse_iterator::operator==;
        using parent::prefix_const_reverse_iterator::operator!=;
        using parent::prefix_const_reverse_iterator::operator++;
        using parent::prefix_const_reverse_iterator::operator--;
        using parent::prefix_const_reverse_iterator::operator->;
    };

    class infix_iterator : public parent::infix_iterator
    {
    public:

        using value_type = parent::infix_iterator::value_type;
        using difference_type = parent::infix_iterator::difference_type;
        using pointer = parent::infix_iterator::pointer;
        using reference = parent::infix_iterator::reference;
        using iterator_category = parent::infix_iterator::iterator_category;

        explicit infix_iterator(parent::node* n = nullptr) noexcept;
        infix_iterator(parent::infix_iterator) noexcept;

        node_color get_color() const noexcept;

        using parent::infix_iterator::depth;
        using parent::infix_iterator::operator*;
        using parent::infix_iterator::operator==;
        using parent::infix_iterator::operator!=;
        using parent::infix_iterator::operator++;
        using parent::infix_iterator::operator--;
        using parent::infix_iterator::operator->;
    };

    class infix_const_iterator : parent::infix_const_iterator
    {
    public:

        using value_type = parent::infix_const_iterator::value_type;
        using difference_type = parent::infix_const_iterator::difference_type;
        using pointer = parent::infix_const_iterator::pointer;
        using reference = parent::infix_const_iterator::reference;
        using iterator_category = parent::infix_const_iterator::iterator_category;

        explicit infix_const_iterator(parent::node* n = nullptr) noexcept;
        infix_const_iterator(parent::infix_const_iterator) noexcept;

        node_color get_color() const noexcept;

        infix_const_iterator(infix_iterator) noexcept;

        using parent::infix_const_iterator::depth;
        using parent::infix_const_iterator::operator*;
        using parent::infix_const_iterator::operator==;
        using parent::infix_const_iterator::operator!=;
        using parent::infix_const_iterator::operator++;
        using parent::infix_const_iterator::operator--;
        using parent::infix_const_iterator::operator->;
    };

    class infix_reverse_iterator : public parent::infix_reverse_iterator
    {
    public:

        using value_type = parent::infix_reverse_iterator::value_type;
        using difference_type = parent::infix_reverse_iterator::difference_type;
        using pointer = parent::infix_reverse_iterator::pointer;
        using reference = parent::infix_reverse_iterator::reference;
        using iterator_category = parent::infix_reverse_iterator::iterator_category;

        explicit infix_reverse_iterator(parent::node* n = nullptr) noexcept;
        infix_reverse_iterator(parent::infix_reverse_iterator) noexcept;

        node_color get_color() const noexcept;

        infix_reverse_iterator(infix_iterator) noexcept;
        operator infix_iterator() const noexcept;
        infix_iterator base() const noexcept;

        using parent::infix_reverse_iterator::depth;
        using parent::infix_reverse_iterator::operator*;
        using parent::infix_reverse_iterator::operator==;
        using parent::infix_reverse_iterator::operator!=;
        using parent::infix_reverse_iterator::operator++;
        using parent::infix_reverse_iterator::operator--;
        using parent::infix_reverse_iterator::operator->;
    };

    class infix_const_reverse_iterator : public parent::infix_const_reverse_iterator
    {
    public:

        using value_type = parent::infix_const_reverse_iterator::value_type;
        using difference_type = parent::infix_const_reverse_iterator::difference_type;
        using pointer = parent::infix_const_reverse_iterator::pointer;
        using reference = parent::infix_const_reverse_iterator::reference;
        using iterator_category = parent::infix_const_reverse_iterator::iterator_category;

        explicit infix_const_reverse_iterator(parent::node* n = nullptr) noexcept;
        infix_const_reverse_iterator(parent::infix_const_reverse_iterator) noexcept;

        node_color get_color() const noexcept;

        infix_const_reverse_iterator(infix_const_iterator) noexcept;
        operator infix_const_iterator() const noexcept;
        infix_const_iterator base() const noexcept;

        using parent::infix_const_reverse_iterator::depth;
        using parent::infix_const_reverse_iterator::operator*;
        using parent::infix_const_reverse_iterator::operator==;
        using parent::infix_const_reverse_iterator::operator!=;
        using parent::infix_const_reverse_iterator::operator++;
        using parent::infix_const_reverse_iterator::operator--;
        using parent::infix_const_reverse_iterator::operator->;
    };

    class postfix_iterator : public parent::postfix_iterator
    {
    public:

        using value_type = parent::postfix_iterator::value_type;
        using difference_type = parent::postfix_iterator::difference_type;
        using pointer = parent::postfix_iterator::pointer;
        using reference = parent::postfix_iterator::reference;
        using iterator_category = parent::postfix_iterator::iterator_category;

        explicit postfix_iterator(parent::node* n = nullptr) noexcept;
        postfix_iterator(parent::postfix_iterator) noexcept;

        node_color get_color() const noexcept;

        using parent::postfix_iterator::depth;
        using parent::postfix_iterator::operator*;
        using parent::postfix_iterator::operator==;
        using parent::postfix_iterator::operator!=;
        using parent::postfix_iterator::operator++;
        using parent::postfix_iterator::operator--;
        using parent::postfix_iterator::operator->;
    };

    class postfix_const_iterator : public parent::postfix_const_iterator
    {
    public:

        using value_type = parent::postfix_const_iterator::value_type;
        using difference_type = parent::postfix_const_iterator::difference_type;
        using pointer = parent::postfix_const_iterator::pointer;
        using reference = parent::postfix_const_iterator::reference;
        using iterator_category = parent::postfix_const_iterator::iterator_category;

        explicit postfix_const_iterator(parent::node* n = nullptr) noexcept;
        postfix_const_iterator(parent::postfix_const_iterator) noexcept;

        node_color get_color() const noexcept;

        postfix_const_iterator(postfix_iterator) noexcept;

        using parent::postfix_const_iterator::depth;
        using parent::postfix_const_iterator::operator*;
        using parent::postfix_const_iterator::operator==;
        using parent::postfix_const_iterator::operator!=;
        using parent::postfix_const_iterator::operator++;
        using parent::postfix_const_iterator::operator--;
        using parent::postfix_const_iterator::operator->;
    };

    class postfix_reverse_iterator : public parent::postfix_reverse_iterator
    {
    public:

        using value_type = parent::postfix_reverse_iterator::value_type;
        using difference_type = parent::postfix_reverse_iterator::difference_type;
        using pointer = parent::postfix_reverse_iterator::pointer;
        using reference = parent::postfix_reverse_iterator::reference;
        using iterator_category = parent::postfix_reverse_iterator::iterator_category;

        explicit postfix_reverse_iterator(parent::node* n = nullptr) noexcept;
        postfix_reverse_iterator(parent::postfix_reverse_iterator) noexcept;

        node_color get_color() const noexcept;

        postfix_reverse_iterator(postfix_iterator) noexcept;
        operator postfix_iterator() const noexcept;
        postfix_iterator base() const noexcept;

        using parent::postfix_reverse_iterator::depth;
        using parent::postfix_reverse_iterator::operator*;
        using parent::postfix_reverse_iterator::operator==;
        using parent::postfix_reverse_iterator::operator!=;
        using parent::postfix_reverse_iterator::operator++;
        using parent::postfix_reverse_iterator::operator--;
        using parent::postfix_reverse_iterator::operator->;
    };

    class postfix_const_reverse_iterator : public parent::postfix_const_reverse_iterator
    {
    public:

        using value_type = parent::postfix_const_reverse_iterator::value_type;
        using difference_type = parent::postfix_const_reverse_iterator::difference_type;
        using pointer = parent::postfix_const_reverse_iterator::pointer;
        using reference = parent::postfix_const_reverse_iterator::reference;
        using iterator_category = parent::postfix_const_reverse_iterator::iterator_category;

        explicit postfix_const_reverse_iterator(parent::node* n = nullptr) noexcept;
        postfix_const_reverse_iterator(parent::postfix_const_reverse_iterator) noexcept;

        node_color get_color() const noexcept;

        postfix_const_reverse_iterator(postfix_const_iterator) noexcept;
        operator postfix_const_iterator() const noexcept;
        postfix_const_iterator base() const noexcept;

        using parent::postfix_const_reverse_iterator::depth;
        using parent::postfix_const_reverse_iterator::operator*;
        using parent::postfix_const_reverse_iterator::operator==;
        using parent::postfix_const_reverse_iterator::operator!=;
        using parent::postfix_const_reverse_iterator::operator++;
        using parent::postfix_const_reverse_iterator::operator--;
        using parent::postfix_const_reverse_iterator::operator->;

    };



    // endregion iterator definition
    
    // region iterator requests declaration

    infix_iterator begin() noexcept;

    infix_iterator end() noexcept;

    infix_const_iterator begin() const noexcept;

    infix_const_iterator end() const noexcept;

    infix_const_iterator cbegin() const noexcept;

    infix_const_iterator cend() const noexcept;

    infix_reverse_iterator rbegin() noexcept;

    infix_reverse_iterator rend() noexcept;

    infix_const_reverse_iterator rbegin() const noexcept;

    infix_const_reverse_iterator rend() const noexcept;

    infix_const_reverse_iterator crbegin() const noexcept;

    infix_const_reverse_iterator crend() const noexcept;


    prefix_iterator begin_prefix() noexcept;

    prefix_iterator end_prefix() noexcept;

    prefix_const_iterator begin_prefix() const noexcept;

    prefix_const_iterator end_prefix() const noexcept;

    prefix_const_iterator cbegin_prefix() const noexcept;

    prefix_const_iterator cend_prefix() const noexcept;

    prefix_reverse_iterator rbegin_prefix() noexcept;

    prefix_reverse_iterator rend_prefix() noexcept;

    prefix_const_reverse_iterator rbegin_prefix() const noexcept;

    prefix_const_reverse_iterator rend_prefix() const noexcept;

    prefix_const_reverse_iterator crbegin_prefix() const noexcept;

    prefix_const_reverse_iterator crend_prefix() const noexcept;


    infix_iterator begin_infix() noexcept;

    infix_iterator end_infix() noexcept;

    infix_const_iterator begin_infix() const noexcept;

    infix_const_iterator end_infix() const noexcept;

    infix_const_iterator cbegin_infix() const noexcept;

    infix_const_iterator cend_infix() const noexcept;

    infix_reverse_iterator rbegin_infix() noexcept;

    infix_reverse_iterator rend_infix() noexcept;

    infix_const_reverse_iterator rbegin_infix() const noexcept;

    infix_const_reverse_iterator rend_infix() const noexcept;

    infix_const_reverse_iterator crbegin_infix() const noexcept;

    infix_const_reverse_iterator crend_infix() const noexcept;


    postfix_iterator begin_postfix() noexcept;

    postfix_iterator end_postfix() noexcept;

    postfix_const_iterator begin_postfix() const noexcept;

    postfix_const_iterator end_postfix() const noexcept;

    postfix_const_iterator cbegin_postfix() const noexcept;

    postfix_const_iterator cend_postfix() const noexcept;

    postfix_reverse_iterator rbegin_postfix() noexcept;

    postfix_reverse_iterator rend_postfix() noexcept;

    postfix_const_reverse_iterator rbegin_postfix() const noexcept;

    postfix_const_reverse_iterator rend_postfix() const noexcept;

    postfix_const_reverse_iterator crbegin_postfix() const noexcept;

    postfix_const_reverse_iterator crend_postfix() const noexcept;

    // endregion iterator requests declaration
    
public:
    
    ~red_black_tree() noexcept final;
    
    red_black_tree(red_black_tree const &other);
    
    red_black_tree &operator=(red_black_tree const &other);
    
    red_black_tree(red_black_tree &&other) noexcept;
    
    red_black_tree &operator=(red_black_tree &&other) noexcept;


    void swap(parent& other) noexcept override;


    /** Only rebinds iterators
     */
    std::pair<infix_iterator, bool> insert(const value_type&);
    std::pair<infix_iterator, bool> insert(value_type&&);

    template<class ...Args>
    std::pair<infix_iterator, bool> emplace(Args&&...args);

    infix_iterator insert_or_assign(const value_type&);
    infix_iterator insert_or_assign(value_type&&);

    template<class ...Args>
    infix_iterator emplace_or_assign(Args&&...args);

    infix_iterator find(const tkey&);
    infix_const_iterator find(const tkey&) const;

    infix_iterator lower_bound(const tkey&);
    infix_const_iterator lower_bound(const tkey&) const;

    infix_iterator upper_bound(const tkey&);
    infix_const_iterator upper_bound(const tkey&) const;

    infix_iterator erase(infix_iterator pos);
    infix_iterator erase(infix_const_iterator pos);

    infix_iterator erase(infix_iterator first, infix_iterator last);
    infix_iterator erase(infix_const_iterator first, infix_const_iterator last);

    using parent::erase;
    using parent::insert;
    using parent::insert_or_assign;
};

template<typename compare, typename U, typename iterator>
explicit red_black_tree(iterator begin, iterator end, const compare& cmp = compare(),
        pp_allocator<U> alloc = pp_allocator<U>(),
        logger* logger = nullptr) -> red_black_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare>;

template<typename compare, typename U, std::ranges::forward_range Range>
explicit red_black_tree(Range&& range, const compare& cmp = compare(),
        pp_allocator<U> alloc = pp_allocator<U>(),
        logger* logger = nullptr) -> red_black_tree<typename std::iterator_traits<typename std::ranges::iterator_t<Range>>::value_type::first_type, typename std::iterator_traits<typename std::ranges::iterator_t<Range>>::value_type::second_type, compare> ;

template<typename tkey, typename tvalue, typename compare, typename U>
red_black_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(),
        pp_allocator<U> alloc = pp_allocator<U>(),
        logger* logger = nullptr) -> red_black_tree<tkey, tvalue, compare>;

namespace __detail {

    class RB_TAG {};

    template<typename tkey, typename tvalue, typename compare>
    template<class ...Args>
    binary_search_tree<tkey, tvalue, compare, RB_TAG>::node* bst_impl<tkey, tvalue, compare, RB_TAG>::create_node(
            binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, Args&& ...args)
    {
        using node_name = typename red_black_tree<tkey, tvalue, compare>::node;
        auto* new_node = cont._allocator.template new_object<node_name>(std::forward<Args>(args)...);
        return new_node;
    }

    template<typename tkey, typename tvalue, typename compare>
    void bst_impl<tkey, tvalue, compare, RB_TAG>::delete_node(
            binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont, binary_search_tree<tkey, tvalue, compare, RB_TAG>::node** node)
    {
        using node_name = typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node;
        if (node != nullptr && *node != nullptr) {
            cont._allocator.template delete_object<node_name>(*node);
            *node = nullptr;
        }
    }

    template<typename tkey, typename tvalue, typename compare>
    void bst_impl<tkey, tvalue, compare, RB_TAG>::post_insert(
            binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont,
            typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node** node_ptr)
    {
        if (node_ptr == nullptr || *node_ptr == nullptr) {
            return;
        }
        using rb_node = typename red_black_tree<tkey, tvalue, compare>::node;
        using binary_node = typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node;
        using color = red_black_tree<tkey, tvalue, compare>::node_color;
        bool is_red = true;
        binary_node* current = *node_ptr;
        static_cast<rb_node*>(*node_ptr)->color = color::RED;
        while (is_red) {
            is_red = false;
            if (current->parent == nullptr) {
                static_cast<rb_node*>(current)->color = color::BLACK;
            } else if (current->parent != nullptr && current->parent->parent != nullptr &&
                        static_cast<rb_node*>(current)->color == color::RED) {
                binary_node* child = current;
                if (static_cast<rb_node*>(child->parent)->color == color::RED) {
                    binary_node* parent = child->parent;
                    bool child_is_left = (child == parent->left_subtree);
                    binary_node* grandparent = parent->parent;
                    bool parent_is_left = (parent == grandparent->left_subtree);
                    binary_node* uncle = parent_is_left ? grandparent->right_subtree : grandparent->left_subtree;
                    bool uncle_is_black = uncle == nullptr || static_cast<rb_node*>(uncle)->color == color::BLACK;
                    binary_node*& subtree = (grandparent == cont._root) ?
                                            cont._root :
                                            (grandparent == grandparent->parent->left_subtree ?
                                             grandparent->parent->left_subtree:
                                             grandparent->parent->right_subtree);
                    if (!uncle_is_black) {
                        is_red = true;
                        static_cast<rb_node*>(uncle)->color = color::BLACK;
                        static_cast<rb_node*>(parent)->color = color::BLACK;
                        static_cast<rb_node*>(grandparent)->color = color::RED;
                        current = current->parent->parent;
                    } else {
                        if (parent_is_left && child_is_left) {
                                cont.small_right_rotation(subtree);
                                static_cast<rb_node*>(child)->color = color::RED;
                                static_cast<rb_node*>(parent)->color = color::BLACK;
                                static_cast<rb_node*>(grandparent)->color = color::RED;
                        } else if (parent_is_left) {
                            cont.big_right_rotation(subtree);
                            static_cast<rb_node*>(child)->color = color::BLACK;
                            static_cast<rb_node*>(parent)->color = color::RED;
                            static_cast<rb_node*>(grandparent)->color = color::RED;
                        } else if (child_is_left) {
                            cont.big_left_rotation(subtree);
                            static_cast<rb_node*>(child)->color = color::BLACK;
                            static_cast<rb_node*>(parent)->color = color::RED;
                            static_cast<rb_node*>(grandparent)->color = color::RED;
                            } else {
                                cont.small_left_rotation(subtree);
                                static_cast<rb_node*>(child)->color = color::RED;
                                static_cast<rb_node*>(parent)->color = color::BLACK;
                                static_cast<rb_node*>(grandparent)->color = color::RED;
                            }
                        }
                    }
            }
        }
    }

    template<typename tkey, typename tvalue, typename compare>
    void bst_impl<tkey, tvalue, compare, RB_TAG>::erase(
            binary_search_tree<tkey, tvalue, compare, RB_TAG>& cont,
            typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node** node_ptr)
    {
        if (node_ptr == nullptr || *node_ptr == nullptr) {
            return;
        }
        using rb_node = typename red_black_tree<tkey, tvalue, compare>::node;
        using binary_node = typename binary_search_tree<tkey, tvalue, compare, RB_TAG>::node;
        using color = red_black_tree<tkey, tvalue, compare>::node_color;

        binary_node* current = *node_ptr;
        binary_node* current_to_balance;
        bool need_balance = false;
        if (current->left_subtree == nullptr && current->right_subtree == nullptr && current->parent == nullptr) {
            delete_node(cont, node_ptr);
            cont._root = nullptr;
            return;
        }
        if (current->left_subtree == nullptr && current->right_subtree == nullptr) {
            need_balance = (static_cast<rb_node*>(current)->color == color::BLACK);
            if (current->parent->left_subtree == current) {
                current->parent->left_subtree = nullptr;
            } else {
                current->parent->right_subtree = nullptr;
            }
            current_to_balance = current->parent;
            delete_node(cont, node_ptr);
        }
        else if (current->left_subtree == nullptr || current->right_subtree == nullptr) {
            binary_node* replacement = (current->left_subtree != nullptr)
                                       ? current->left_subtree
                                       : current->right_subtree;
            if (replacement != nullptr){
                replacement->parent = current->parent;
            }
            if (current->parent == nullptr){
                cont._root = replacement;
            } else {
                if (current == current->parent->left_subtree) {
                    current->parent->left_subtree = replacement;
                } else {
                    current->parent->right_subtree = replacement;
                }
            }
            color color_current = static_cast<rb_node*>(current)->color;
            if (color_current == color::BLACK) {
                static_cast<rb_node*>(replacement)->color = color::BLACK;
            }
            delete_node(cont, node_ptr);
        }
        else {
                binary_node** replacement = &current->left_subtree;
                while ((*replacement)->right_subtree != nullptr) {
                    replacement = &((*replacement)->right_subtree);
                }
                binary_node* temp = *replacement;
                bool temp_is_black = (static_cast<rb_node*>(temp)->color == color::BLACK);
                need_balance = (temp_is_black &&
                            temp->left_subtree == nullptr &&
                            temp->right_subtree == nullptr);
                std::swap(static_cast<rb_node*>(current)->color, static_cast<rb_node*>(temp)->color);
                if (*replacement == temp) {
                    binary_node* new_left = temp->left_subtree;
                    if (new_left != nullptr) {
                        new_left->parent = temp->parent;
                    }
                    if (temp->parent != nullptr) {
                        if (temp->parent->left_subtree == temp)
                            temp->parent->left_subtree = new_left;
                        else
                            temp->parent->right_subtree = new_left;
                    } else {
                        cont._root = new_left;
                    }
                }
                temp->parent = current->parent;
                if (current->left_subtree != temp) {
                    temp->left_subtree = current->left_subtree;
                    if (temp->left_subtree != nullptr) {
                        temp->left_subtree->parent = temp;
                    }
                }
                temp->right_subtree = current->right_subtree;
                if (temp->right_subtree != nullptr) {
                    temp->right_subtree->parent = temp;
                }
                if (current->parent == nullptr) {
                    cont._root = temp;
                } else if (current == current->parent->left_subtree) {
                    current->parent->left_subtree = temp;
                } else {
                    current->parent->right_subtree = temp;
                }
            delete_node(cont, node_ptr);
        }
        if (!need_balance) {
            return;
        }
        while (true) {
            if (current_to_balance == nullptr) {
                return;
            }
            if (current_to_balance->parent == nullptr) {
                static_cast<rb_node*>(current_to_balance)->color = color::BLACK;
                break;
            }
            binary_node* temp_node = current_to_balance;
            binary_node* parent = current_to_balance->parent;
            bool is_left_child = (temp_node->parent->left_subtree == temp_node);
            binary_node* brother = is_left_child ? parent->right_subtree : parent->left_subtree;
            bool brother_is_red = brother != nullptr && static_cast<rb_node*>(brother)->color == color::RED;
            if (brother_is_red && is_left_child) {
                binary_node*& subtree = (current_to_balance->parent == cont._root) ?
                            cont._root :
                                            (current_to_balance->parent->parent->left_subtree == current->parent) ?
                                            current_to_balance->parent->parent->left_subtree:
                                            current_to_balance->parent->parent->right_subtree;
                cont.small_left_rotation(subtree);
                static_cast<rb_node*>(parent)->color = color::RED;
                static_cast<rb_node*>(brother)->color = color::BLACK;
                current_to_balance = current_to_balance->left_subtree;
                continue;
            } else if (brother_is_red) {
                binary_node*& subtree = (current_to_balance->parent == cont._root) ?
                                            cont._root :
                                            (current_to_balance->parent->parent->left_subtree == current->parent) ?
                                            current_to_balance->parent->parent->left_subtree:
                                            current_to_balance->parent->parent->right_subtree;
                cont.small_right_rotation(subtree);
                static_cast<rb_node*>(parent)->color = color::RED;
                static_cast<rb_node*>(brother)->color = color::BLACK;
                current_to_balance = current_to_balance->right_subtree;
                continue;
            } else if (brother != nullptr) {
                binary_node* f_nephew = is_left_child ? brother->right_subtree : brother->left_subtree;
                binary_node* nephew = is_left_child ? brother->left_subtree : brother->right_subtree;
                bool f_nephew_is_red = f_nephew != nullptr && static_cast<rb_node*>(f_nephew)->color == color::RED;
                bool nephew_is_red = nephew != nullptr && static_cast<rb_node*>(nephew)->color == color::RED;
                binary_node*& subtree = (current_to_balance->parent == cont._root) ?
                                        cont._root :
                                        (current_to_balance->parent->parent->left_subtree == current->parent) ?
                                        current_to_balance->parent->parent->left_subtree:
                                        current_to_balance->parent->parent->right_subtree;
                if (f_nephew_is_red && is_left_child) {
                    cont.small_left_rotation(subtree);
                    static_cast<rb_node *>(brother)->color = static_cast<rb_node *>(parent)->color;
                    static_cast<rb_node *>(parent)->color = color::BLACK;
                    static_cast<rb_node *>(f_nephew)->color = color::BLACK;
                    break;
                } else if (f_nephew_is_red) {
                    cont.small_right_rotation(subtree);
                    static_cast<rb_node*>(brother)->color = static_cast<rb_node*>(parent)->color;
                    static_cast<rb_node*>(parent)->color = color::BLACK;
                    static_cast<rb_node*>(f_nephew)->color = color::BLACK;
                    break;
                } else if (nephew_is_red && is_left_child) {
                    cont.big_left_rotation(subtree);
                    static_cast<rb_node *>(nephew)->color = static_cast<rb_node *>(parent)->color;
                    static_cast<rb_node *>(parent)->color = color::BLACK;
                    break;
                } else if (nephew_is_red) {
                    cont.big_right_rotation(subtree);
                    static_cast<rb_node*>(nephew)->color = static_cast<rb_node*>(parent)->color;
                    static_cast<rb_node*>(parent)->color = color::BLACK;
                    break;
                } else {
                    static_cast<rb_node*>(brother)->color = color::RED;
                    bool parent_is_red = static_cast<rb_node*>(parent)->color == color::RED;
                    if (parent_is_red) {
                        static_cast<rb_node*>(parent)->color = color::BLACK;
                        break;
                    } else {
                        if (current_to_balance != nullptr)
                            current_to_balance = current_to_balance->parent;
                        continue;
                    }
                }
            } else {
                static_cast<rb_node*>(parent)->color = color::BLACK;
                current_to_balance = current_to_balance->parent;
                continue;
            }
        }
    }


    template<typename tkey, typename tvalue, typename compare>
    void bst_impl<tkey, tvalue, compare, RB_TAG>::swap(binary_search_tree<tkey, tvalue, compare, RB_TAG> &lhs,
                                                                            binary_search_tree<tkey, tvalue, compare, RB_TAG> &rhs) noexcept
    {
        std::swap(lhs._allocator, rhs._allocator);
        std::swap(lhs._logger, rhs._logger);
        std::swap(lhs._root, rhs._root);
        std::swap(lhs._size, rhs._size);
    }
}


template<typename tkey, typename tvalue, compator<tkey> compare>
template<class ...Args>
red_black_tree<tkey, tvalue, compare>::node::node(parent::node* par, Args&&... args):
parent::node(par, std::forward<Args>(args)...), color(node_color::RED)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::red_black_tree(
        const compare& comp,
        pp_allocator<value_type> alloc,
        logger *logger): parent(comp, alloc, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::red_black_tree(
        pp_allocator<value_type> alloc,
        const compare& comp,
        logger *logger): parent(alloc, comp, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
template<input_iterator_for_pair<tkey, tvalue> iterator>
red_black_tree<tkey, tvalue, compare>::red_black_tree(
        iterator begin, iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): parent(cmp, alloc, logger)
{
    for (auto iter = begin; iter != end; ++iter) {
        this->emplace(*iter);
    }
}

template<typename tkey, typename tvalue, compator<tkey> compare>
template<std::ranges::input_range Range>
red_black_tree<tkey, tvalue, compare>::red_black_tree(
        Range&& range,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): parent(cmp, alloc, logger)
{
    for (auto&& elem: range) {
        emplace(std::forward<decltype(elem)>(elem));
    }
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::red_black_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): parent(cmp, alloc, logger)
{
    for (const auto& elem: data) {
        emplace(elem.first, elem.second);
    }
}

// region iterator implementation

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_iterator::prefix_iterator(parent::node* n) noexcept: parent::prefix_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_iterator::prefix_iterator(parent::prefix_iterator it) noexcept: parent::prefix_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::prefix_iterator::get_color() const noexcept
{
    return static_cast<node*>(this->_data)->color;
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_iterator::prefix_const_iterator(parent::node* n) noexcept: parent::prefix_const_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_iterator::prefix_const_iterator(parent::prefix_const_iterator it) noexcept: parent::prefix_const_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::prefix_const_iterator::get_color() const noexcept
{
    return prefix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_iterator::prefix_const_iterator(prefix_iterator it) noexcept: parent::prefix_const_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator::prefix_reverse_iterator(parent::node* n) noexcept: parent::prefix_reverse_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator::prefix_reverse_iterator(parent::prefix_reverse_iterator it) noexcept: parent::prefix_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator::get_color() const noexcept
{
    return prefix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator::prefix_reverse_iterator(prefix_iterator it) noexcept: parent::prefix_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator::operator red_black_tree<tkey, tvalue, compare>::prefix_iterator() const noexcept
{
    return parent::prefix_reverse_iterator::operator prefix_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_iterator
red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator::base() const noexcept
{
    return parent::prefix_reverse_iterator::base();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator::prefix_const_reverse_iterator(parent::node* n) noexcept:
parent::prefix_const_reverse_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator::prefix_const_reverse_iterator(parent::prefix_const_reverse_iterator it) noexcept:
parent::prefix_const_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator::get_color() const noexcept
{
    return prefix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator::prefix_const_reverse_iterator(prefix_const_iterator it) noexcept:
parent::prefix_const_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator::operator red_black_tree<tkey, tvalue, compare>::prefix_const_iterator() const noexcept
{
    return parent::prefix_const_reverse_iterator::operator prefix_const_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_iterator
red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator::base() const noexcept
{
    return parent::prefix_const_reverse_iterator::base();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_iterator::infix_iterator(parent::node* n) noexcept:
parent::infix_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_iterator::infix_iterator(parent::infix_iterator it) noexcept:
parent::infix_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::infix_iterator::get_color() const noexcept
{
    return static_cast<node*>(this->_data)->color;
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_iterator::infix_const_iterator(parent::node* n) noexcept:
parent::infix_const_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_iterator::infix_const_iterator(parent::infix_const_iterator it) noexcept:
parent::infix_const_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::infix_const_iterator::get_color() const noexcept
{
    return infix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_iterator::infix_const_iterator(infix_iterator it) noexcept:
parent::infix_const_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator::infix_reverse_iterator(parent::node* n) noexcept:
parent::infix_reverse_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator::infix_reverse_iterator(parent::infix_reverse_iterator it) noexcept:
parent::infix_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator::get_color() const noexcept
{
    return infix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator::infix_reverse_iterator(infix_iterator it) noexcept:
parent::infix_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator::operator red_black_tree<tkey, tvalue, compare>::infix_iterator() const noexcept
{
    return parent::infix_reverse_iterator::operator infix_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator::base() const noexcept
{
    return parent::infix_reverse_iterator::base();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator::infix_const_reverse_iterator(parent::node* n) noexcept:
parent::infix_const_reverse_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator::infix_const_reverse_iterator(parent::infix_const_reverse_iterator it) noexcept:
parent::infix_const_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator::get_color() const noexcept
{
    return infix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator::infix_const_reverse_iterator(infix_const_iterator it) noexcept:
parent::infix_const_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator::operator red_black_tree<tkey, tvalue, compare>::infix_const_iterator() const noexcept
{
    return parent::infix_const_reverse_iterator::operator infix_const_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator::base() const noexcept
{
    return parent::infix_const_reverse_iterator::base();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_iterator::postfix_iterator(parent::node* n) noexcept:
parent::postfix_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_iterator::postfix_iterator(parent::postfix_iterator it) noexcept:
parent::postfix_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::postfix_iterator::get_color() const noexcept
{
    return static_cast<node*>(this->_data)->color;
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_iterator::postfix_const_iterator(parent::node* n) noexcept:
parent::postfix_const_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_iterator::postfix_const_iterator(parent::postfix_const_iterator it) noexcept:
parent::postfix_const_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::postfix_const_iterator::get_color() const noexcept
{
    return postfix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_iterator::postfix_const_iterator(postfix_iterator it) noexcept:
parent::postfix_const_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator::postfix_reverse_iterator(parent::node* n) noexcept:
parent::postfix_reverse_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator::postfix_reverse_iterator(parent::postfix_reverse_iterator it) noexcept:
parent::postfix_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator::get_color() const noexcept
{
    return postfix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator::postfix_reverse_iterator(postfix_iterator it) noexcept:
parent::postfix_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator::operator red_black_tree<tkey, tvalue, compare>::postfix_iterator() const noexcept
{
    return parent::postfix_reverse_iterator::operator postfix_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_iterator
red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator::base() const noexcept
{
    return parent::postfix_reverse_iterator::base();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator::postfix_const_reverse_iterator(parent::node* n) noexcept:
parent::postfix_const_reverse_iterator(n)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator::postfix_const_reverse_iterator(parent::postfix_const_reverse_iterator it) noexcept:
parent::postfix_const_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::node_color
red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator::get_color() const noexcept
{
    return postfix_iterator(this->_base).get_color();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator::postfix_const_reverse_iterator(postfix_const_iterator it) noexcept:
parent::postfix_const_reverse_iterator(it)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator::operator red_black_tree<tkey, tvalue, compare>::postfix_const_iterator() const noexcept
{
    return parent::postfix_const_reverse_iterator::operator postfix_const_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_iterator
red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator::base() const noexcept
{
    return parent::postfix_const_reverse_iterator::base();
}

// endregion iterator implementation

// region iterator requests implementation

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::begin() noexcept
{
    return parent::begin();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::end() noexcept
{
    return parent::end();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::begin() const noexcept
{
    return parent::begin();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::end() const noexcept
{
    return parent::end();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::cbegin() const noexcept
{
    return parent::cbegin();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::cend() const noexcept
{
    return parent::cend();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin() noexcept
{
    return parent::rbegin();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend() noexcept
{
    return parent::rend();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin() const noexcept
{
    return parent::rbegin();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend() const noexcept
{
    return parent::rend();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crbegin() const noexcept
{
    return parent::crbegin();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crend() const noexcept
{
    return parent::crend();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_iterator
red_black_tree<tkey, tvalue, compare>::begin_prefix() noexcept
{
    return parent::begin_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_iterator
red_black_tree<tkey, tvalue, compare>::end_prefix() noexcept
{
    return parent::end_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_iterator
red_black_tree<tkey, tvalue, compare>::begin_prefix() const noexcept
{
    return parent::begin_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_iterator
red_black_tree<tkey, tvalue, compare>::end_prefix() const noexcept
{
    return parent::end_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_iterator
red_black_tree<tkey, tvalue, compare>::cbegin_prefix() const noexcept
{
    return parent::cbegin_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_iterator
red_black_tree<tkey, tvalue, compare>::cend_prefix() const noexcept
{
    return parent::cend_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin_prefix() noexcept
{
    return parent::rbegin_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend_prefix() noexcept
{
    return parent::rend_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin_prefix() const noexcept
{
    return parent::rbegin_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend_prefix() const noexcept
{
    return parent::rend_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crbegin_prefix() const noexcept
{
    return parent::crbegin_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::prefix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crend_prefix() const noexcept
{
    return parent::crend_prefix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::begin_infix() noexcept
{
    return parent::begin_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::end_infix() noexcept
{
    return parent::end_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::begin_infix() const noexcept
{
    return parent::begin_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::end_infix() const noexcept
{
    return parent::end_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::cbegin_infix() const noexcept
{
    return parent::cbegin_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::cend_infix() const noexcept
{
    return parent::cend_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin_infix() noexcept
{
    return parent::rbegin_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend_infix() noexcept
{
    return parent::rend_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin_infix() const noexcept
{
    return parent::rbegin_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend_infix() const noexcept
{
    return parent::rend_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crbegin_infix() const noexcept
{
    return parent::crbegin_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crend_infix() const noexcept
{
    return parent::crend_infix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_iterator
red_black_tree<tkey, tvalue, compare>::begin_postfix() noexcept
{
    return parent::begin_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_iterator
red_black_tree<tkey, tvalue, compare>::end_postfix() noexcept
{
    return parent::end_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_iterator
red_black_tree<tkey, tvalue, compare>::begin_postfix() const noexcept
{
    return parent::begin_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_iterator
red_black_tree<tkey, tvalue, compare>::end_postfix() const noexcept
{
    return parent::end_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_iterator
red_black_tree<tkey, tvalue, compare>::cbegin_postfix() const noexcept
{
    return parent::cbegin_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_iterator
red_black_tree<tkey, tvalue, compare>::cend_postfix() const noexcept
{
    return parent::cend_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin_postfix() noexcept
{
    return parent::rbegin_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend_postfix() noexcept
{
    return parent::rend_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rbegin_postfix() const noexcept
{
    return parent::rbegin_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::rend_postfix() const noexcept
{
    return parent::rend_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crbegin_postfix() const noexcept
{
    return parent::crbegin_postfix();
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::postfix_const_reverse_iterator
red_black_tree<tkey, tvalue, compare>::crend_postfix() const noexcept
{
    return parent::crend_postfix();
}

// endregion iterator requests implementation

// region rb_tree implementation

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::~red_black_tree() noexcept
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::red_black_tree(red_black_tree const &other): parent(other)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare> &
red_black_tree<tkey, tvalue, compare>::operator=(red_black_tree const &other)
{
    if (this != &other) {
        parent::operator=(other);
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare>::red_black_tree(red_black_tree &&other) noexcept: parent(std::move(other))
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
red_black_tree<tkey, tvalue, compare> &
red_black_tree<tkey, tvalue, compare>::operator=(red_black_tree &&other) noexcept
{
    if (this != &other) {
        parent::operator=(std::move(other));
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare>
void red_black_tree<tkey, tvalue, compare>::swap(parent& other) noexcept
{
    if (this != &other) {
        parent::swap(other);
    }
}

// endregion rb_tree implementation

template<typename tkey, typename tvalue, compator<tkey> compare>
std::pair<typename red_black_tree<tkey, tvalue, compare>::infix_iterator, bool>
red_black_tree<tkey, tvalue, compare>::insert(const value_type& value)
{
    return parent::insert(value);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
std::pair<typename red_black_tree<tkey, tvalue, compare>::infix_iterator, bool>
red_black_tree<tkey, tvalue, compare>::insert(value_type&& value)
{
    return parent::insert(std::move(value));
}

template<typename tkey, typename tvalue, compator<tkey> compare>
template<class ...Args>
std::pair<typename red_black_tree<tkey, tvalue, compare>::infix_iterator, bool>
red_black_tree<tkey, tvalue, compare>::emplace(Args&&... args)
{
    return parent::emplace(std::forward<Args>(args)...);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::insert_or_assign(const value_type& value)
{
    return parent::insert_or_assign(value);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::insert_or_assign(value_type&& value)
{
    return parent::insert_or_assign(std::move(value));
}

template<typename tkey, typename tvalue, compator<tkey> compare>
template<class ...Args>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::emplace_or_assign(Args&&... args)
{
    return binary_search_tree<tkey, tvalue, compare, __detail::RB_TAG>::emplace_or_assign(std::forward<Args>(args)...);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::find(const tkey& key)
{
    return parent::find(key);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::find(const tkey& key) const
{
    return parent::find(key);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::lower_bound(const tkey& key)
{
    return parent::lower_bound(key);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::lower_bound(const tkey& key) const
{
    return parent::lower_bound(key);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::upper_bound(const tkey& key)
{
    return parent::upper_bound(key);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_const_iterator
red_black_tree<tkey, tvalue, compare>::upper_bound(const tkey& key) const
{
    return parent::upper_bound(key);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::erase(infix_iterator pos)
{
    return parent::erase(pos);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::erase(infix_const_iterator pos)
{
    return parent::erase(pos);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::erase(infix_iterator first, infix_iterator last)
{
    return parent::erase(first, last);
}

template<typename tkey, typename tvalue, compator<tkey> compare>
typename red_black_tree<tkey, tvalue, compare>::infix_iterator
red_black_tree<tkey, tvalue, compare>::erase(infix_const_iterator first, infix_const_iterator last)
{
    return parent::erase(first, last);
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
