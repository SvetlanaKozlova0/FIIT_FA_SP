#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_SPLAY_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_SPLAY_TREE_H

#include <binary_search_tree.h>

namespace __detail
{
    class SPL_TAG;

    template<typename tkey, typename tvalue, typename compare>
    class bst_impl<tkey, tvalue, compare, SPL_TAG>
    {


    private:

        static void print_splay(const std::string& prefix, typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node* node, bool is_left)
        {
            if (node == nullptr) {
                return;
            }
            std::cout << prefix << (is_left ? "├──" : "└──" ) << node->data.first << std::endl;
            print_splay(prefix + (is_left ? "│   " : "    "), node->left_subtree, true);
            print_splay(prefix + (is_left ? "│   " : "    "), node->right_subtree, false);
        }

        static void print_splay_full(typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node* node)
        {
            print_splay("", node, false);
        }

        static void splay(typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node*& n,
                          binary_search_tree<tkey, tvalue, compare, SPL_TAG>& cont) {
            using binary_node = typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node;
            while (n->parent != nullptr) {
                binary_node* parent = n->parent;
                binary_node* grandparent = parent->parent;

                if (grandparent == nullptr) {
                    if (n == parent->left_subtree) {
                        cont.small_right_rotation(parent);
                    } else {
                        cont.small_left_rotation(parent);
                    }
                } else {
                    bool parent_is_left = (grandparent->left_subtree == parent);
                    bool node_is_left = (parent->left_subtree == n);

                    if (parent_is_left == node_is_left) {
                        if (node_is_left) {
                            cont.small_right_rotation(grandparent);
                            cont.small_right_rotation(parent);
                        } else {
                            cont.small_left_rotation(grandparent);
                            cont.small_left_rotation(parent);
                        }
                    } else {
                        if (node_is_left) {
                            cont.small_right_rotation(parent);
                            cont.small_left_rotation(grandparent);
                        } else {
                            cont.small_left_rotation(parent);
                            cont.small_right_rotation(grandparent);
                        }
                    }
                }
               n = parent;
            }
            cont._root = n;
            print_splay_full(cont._root);
            std::cout << "\n";
        }
    public:

        template<class ...Args>
        static binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node* create_node(binary_search_tree<tkey, tvalue, compare, SPL_TAG>& cont, Args&& ...args);

        static void delete_node(binary_search_tree<tkey, tvalue, compare, SPL_TAG>& cont, typename
                                binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node **node);

        //Does not invalidate node*, needed for splay tree
        static void post_search(binary_search_tree<tkey, tvalue, compare, SPL_TAG> &cont,
                                binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node** node){
            if (node != nullptr && *node != nullptr) {
                splay(*node, cont);
            }
        }

        //Does not invalidate node*
        static void post_insert(binary_search_tree<tkey, tvalue, compare, SPL_TAG>& cont, binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node** node) {
            if (node != nullptr && *node != nullptr) {
                splay(*node, cont);
            }
        }

        static void erase(
                binary_search_tree<tkey, tvalue, compare, SPL_TAG>& cont,
                typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node** node_ptr
        ) {
            if (node_ptr == nullptr || *node_ptr == nullptr) {
                return;
            }
            using binary_node = typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node;

            splay(*node_ptr, cont);

            binary_node* left = cont._root->left_subtree;
            binary_node* right = cont._root->right_subtree;

            delete_node(cont, &cont._root);
            cont._root = nullptr;

            if (left != nullptr) {
                left->parent = nullptr;
            }
            if (right != nullptr) {
                right->parent = nullptr;
            }
            if (left == nullptr) {
                cont._root = right;
            } else {
                binary_node* max_left = left;
                while (max_left->right_subtree) {
                    max_left = max_left->right_subtree;
                }
                splay(max_left, cont);
                max_left->right_subtree = right;
                if (right != nullptr)  {
                    right->parent = max_left;
                }
                cont._root = max_left;
                splay(cont._root, cont);
            }
            *node_ptr = nullptr;
        }

        static void swap(binary_search_tree<tkey, tvalue, compare, SPL_TAG>& lhs, binary_search_tree<tkey, tvalue, compare, SPL_TAG>& rhs) noexcept {
            std::swap(lhs._allocator, rhs._allocator);
            std::swap(lhs._logger, rhs._logger);
            std::swap(lhs._root, rhs._root);
            std::swap(lhs._size, rhs._size);
        }
    };
}

template<typename tkey, typename tvalue, compator<tkey> compare = std::less<tkey>>
class splay_tree final: public binary_search_tree<tkey, tvalue, compare, __detail::SPL_TAG>
{

    using parent = binary_search_tree<tkey, tvalue, compare, __detail::SPL_TAG>;
public:

    using value_type = parent::value_type;

    explicit splay_tree(
            const compare& comp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger *logger = nullptr);

    explicit splay_tree(
            pp_allocator<value_type> alloc,
            const compare& comp = compare(),
            logger *logger = nullptr);

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit splay_tree(iterator begin, iterator end, const compare& cmp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger* logger = nullptr);

    template<std::ranges::input_range Range>
    explicit splay_tree(Range&& range, const compare& cmp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger* logger = nullptr);


    splay_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(),
            pp_allocator<value_type> alloc = pp_allocator<value_type>(),
            logger* logger = nullptr);

public:
    
    ~splay_tree() noexcept final;
    
    splay_tree(splay_tree const &other);
    
    splay_tree &operator=(splay_tree const &other);
    
    splay_tree(splay_tree &&other) noexcept;
    
    splay_tree &operator=(splay_tree &&other) noexcept;
};

template<typename compare, typename U, typename iterator>
explicit splay_tree(iterator begin, iterator end, const compare& cmp = compare(),
        pp_allocator<U> alloc = pp_allocator<U>(),
        logger* logger = nullptr) -> splay_tree<const typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare>;

template<typename compare, typename U, std::ranges::forward_range Range>
explicit splay_tree(Range&& range, const compare& cmp = compare(),
        pp_allocator<U> alloc = pp_allocator<U>(),
        logger* logger = nullptr) -> splay_tree<const typename std::iterator_traits<typename std::ranges::iterator_t<Range>>::value_type::first_type, typename std::iterator_traits<typename std::ranges::iterator_t<Range>>::value_type::second_type, compare> ;

template<typename tkey, typename tvalue, typename compare, typename U>
splay_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(),
        pp_allocator<U> alloc = pp_allocator<U>(),
        logger* logger = nullptr) -> splay_tree<tkey, tvalue, compare>;

// region implementation

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare>::splay_tree(
        const compare& comp,
        pp_allocator<value_type> alloc,
        logger *logger): parent(comp, alloc, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare>::splay_tree(
        pp_allocator<value_type> alloc,
        const compare& comp,
        logger *logger): parent(comp, alloc, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
template<input_iterator_for_pair<tkey, tvalue> iterator>
splay_tree<tkey, tvalue, compare>::splay_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): parent(begin, end, cmp, alloc, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
template<std::ranges::input_range Range>
splay_tree<tkey, tvalue, compare>::splay_tree(
        Range&& range,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): parent(range, cmp, alloc, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare>::splay_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): parent(data, cmp, alloc, logger)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare>::~splay_tree() noexcept
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare>::splay_tree(splay_tree const &other): parent(other)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare> &splay_tree<tkey, tvalue, compare>::operator=(splay_tree const &other)
{
    if (*this != other) {
        parent::operator=(other);
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare>::splay_tree(splay_tree &&other) noexcept: parent(std::move(other))
{
}

template<typename tkey, typename tvalue, compator<tkey> compare>
splay_tree<tkey, tvalue, compare> &splay_tree<tkey, tvalue, compare>::operator=(splay_tree &&other) noexcept
{
    if (*this != other) {
        parent::operator=(std::move(other));
    }
    return *this;
}

// endregion implementation

namespace __detail {

    template<typename tkey, typename tvalue, typename compare>
    template<class ...Args>
    binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node *bst_impl<tkey, tvalue, compare, SPL_TAG>::create_node(
            binary_search_tree <tkey, tvalue, compare, SPL_TAG> &cont, Args &&...args){
        using node_name = typename splay_tree<tkey, tvalue, compare>::node;
        auto *new_node = cont._allocator.template new_object<node_name>(std::forward<Args>(args)...);
        return new_node;
    }

    template<typename tkey, typename tvalue, typename compare>
    void bst_impl<tkey, tvalue, compare, SPL_TAG>::delete_node(binary_search_tree<tkey, tvalue, compare, SPL_TAG>& cont, typename
                            binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node **node) {
        using node_name = typename binary_search_tree<tkey, tvalue, compare, SPL_TAG>::node;
        if (node != nullptr && *node != nullptr) {
            cont._allocator.template delete_object<node_name>(*node);
            *node = nullptr;
        }
    }

}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_SPLAY_TREE_H