#ifndef MP_OS_B_TREE_H
#define MP_OS_B_TREE_H

#include <iterator>
#include <utility>
#include <boost/container/static_vector.hpp>
#include <stack>
#include <pp_allocator.h>
#include <search_tree.h>
#include <initializer_list>
#include <logger_guardant.h>

template <typename tkey, typename tvalue, compator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class B_tree final : private logger_guardant, private compare
{
public:

    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<const tkey, tvalue>;
    using value_type = tree_data_type_const;

private:

    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration


    struct btree_node
    {
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<btree_node*, maximum_keys_in_node + 2> _pointers;
        btree_node() noexcept;
    };

    pp_allocator<value_type> _allocator;
    logger* _logger;
    btree_node* _root;
    size_t _size;

    logger* get_logger() const noexcept override;
    pp_allocator<value_type> get_allocator() const noexcept;

public:

    // region constructors declaration

    explicit B_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>(), logger* logger = nullptr);

    explicit B_tree(pp_allocator<value_type> alloc, const compare& comp = compare(), logger *logger = nullptr);

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit B_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>(), logger* logger = nullptr);

    B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>(), logger* logger = nullptr);

    // endregion constructors declaration

    // region five declaration

    B_tree(const B_tree& other);

    B_tree(B_tree&& other) noexcept;

    B_tree& operator=(const B_tree& other);

    B_tree& operator=(B_tree&& other) noexcept;

    ~B_tree() noexcept override;

    // endregion five declaration

    // region iterators declaration

    class btree_iterator;
    class btree_reverse_iterator;
    class btree_const_iterator;
    class btree_const_reverse_iterator;

    class btree_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:
        using value_type = tree_data_type;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);

    };

    class btree_const_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_iterator;
        friend class btree_const_reverse_iterator;

        btree_const_iterator(const btree_iterator& it) noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    class btree_reverse_iterator final
    {
        std::stack<std::pair<btree_node**, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_reverse_iterator;

        friend class B_tree;
        friend class btree_iterator;
        friend class btree_const_iterator;
        friend class btree_const_reverse_iterator;

        btree_reverse_iterator(const btree_iterator& it) noexcept;
        operator btree_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_reverse_iterator(const std::stack<std::pair<btree_node**, size_t>>& path = std::stack<std::pair<btree_node**, size_t>>(), size_t index = 0);
    };

    class btree_const_reverse_iterator final
    {
        std::stack<std::pair<btree_node* const*, size_t>> _path;
        size_t _index;

    public:

        using value_type = tree_data_type;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = btree_const_reverse_iterator;

        friend class B_tree;
        friend class btree_reverse_iterator;
        friend class btree_const_iterator;
        friend class btree_iterator;

        btree_const_reverse_iterator(const btree_reverse_iterator& it) noexcept;
        operator btree_const_iterator() const noexcept;

        reference operator*() const noexcept;
        pointer operator->() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t depth() const noexcept;
        size_t current_node_keys_count() const noexcept;
        bool is_terminate_node() const noexcept;
        size_t index() const noexcept;

        explicit btree_const_reverse_iterator(const std::stack<std::pair<btree_node* const*, size_t>>& path = std::stack<std::pair<btree_node* const*, size_t>>(), size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;
    friend class btree_reverse_iterator;
    friend class btree_const_reverse_iterator;

    // endregion iterators declaration

    // region element access declaration

    /*
     * Returns a reference to the mapped value of the element with specified key. If no such element exists, an exception of type std::out_of_range is thrown.
     */
    tvalue& at(const tkey&);
    const tvalue& at(const tkey&) const;

    /*
     * If key not exists, makes default initialization of value
     */
    tvalue& operator[](const tkey& key);
    tvalue& operator[](tkey&& key);

    // endregion element access declaration
    // region iterator begins declaration

    btree_iterator begin();
    btree_iterator end();

    btree_const_iterator begin() const;
    btree_const_iterator end() const;

    btree_const_iterator cbegin() const;
    btree_const_iterator cend() const;

    btree_reverse_iterator rbegin();
    btree_reverse_iterator rend();

    btree_const_reverse_iterator rbegin() const;
    btree_const_reverse_iterator rend() const;

    btree_const_reverse_iterator crbegin() const;
    btree_const_reverse_iterator crend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    btree_iterator find(const tkey& key);
    btree_const_iterator find(const tkey& key) const;

    btree_iterator lower_bound(const tkey& key);
    btree_const_iterator lower_bound(const tkey& key) const;

    btree_iterator upper_bound(const tkey& key);
    btree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<btree_iterator, bool> insert(const tree_data_type& data);
    std::pair<btree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<btree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    btree_iterator insert_or_assign(const tree_data_type& data);
    btree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    btree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    btree_iterator erase(btree_iterator pos);
    btree_iterator erase(btree_const_iterator pos);

    btree_iterator erase(btree_iterator beg, btree_iterator en);
    btree_iterator erase(btree_const_iterator beg, btree_const_iterator en);


    btree_iterator erase(const tkey& key);

    // endregion modifiers declaration

private:

    std::pair<size_t, bool> find_key_position(const tkey& key, btree_node* node) const noexcept {
        if (node == nullptr) {
            return {0, false};
        }
        size_t begin = 0;
        size_t middle = 0;
        size_t end = node->_keys.size();
        while (begin < end) {
            middle = begin + (end - begin) / 2;
            if (compare_keys(node->_keys[middle].first, key)) {
                begin = middle + 1;
            } else if (compare_keys(key, node->_keys[middle].first)) {
                end = middle;
            } else {
                return {middle, true};
            }
        }
        return {begin, false};
    }


    void split_and_promote_node(std::stack<std::pair<btree_node**, size_t>>& path, btree_node*& node, size_t& index) {
        btree_node* current = *path.top().first;
        size_t middle_index = current->_keys.size() / 2;
        auto middle_pair = std::move(current->_keys[middle_index]);
        auto* new_node = _allocator.template new_object<btree_node>();
        new_node->_keys.assign(std::make_move_iterator(current->_keys.begin() + middle_index + 1),
                               std::make_move_iterator(current->_keys.end()));
        current->_keys.erase(current->_keys.begin() + middle_index, current->_keys.end());
        if (!is_leaf_node(current)) {
            new_node->_pointers.assign(current->_pointers.begin() + middle_index + 1,
                                       current->_pointers.end());
            current->_pointers.erase(current->_pointers.begin() + middle_index + 1, current->_pointers.end());
        }
        if (path.size() == 1) {
            auto* new_root = _allocator.template new_object<btree_node>();
            new_root->_keys.push_back(middle_pair);
            new_root->_pointers.push_back(current);
            new_root->_pointers.push_back(new_node);
            _root = new_root;
            path.pop();
            path.push(std::make_pair(&_root, 0));
            return;
        }
        path.pop();
        btree_node* parent = *path.top().first;
        auto [index_in_parent, exist] = find_key_position(middle_pair.first, parent);
        parent->_keys.insert(parent->_keys.begin() + index_in_parent, middle_pair);
        parent->_pointers.insert(parent->_pointers.begin() + index_in_parent + 1, new_node);
        if (node == current) {
            if (index == middle_index) {
                node = parent;
                index = index_in_parent;
            } else if (index > middle_index) {
                node = new_node;
                index -= (middle_index + 1);
            }
        }
        path.top().second = index_in_parent;
    }

    void merge_child_nodes(std::stack<std::pair<btree_node**, size_t>>& path, btree_node*& parent, size_t& index) {
        btree_node* left = parent->_pointers[index];
        btree_node* right = parent->_pointers[index + 1];
        left->_keys.push_back(std::move(parent->_keys[index]));
        for (auto& key: right->_keys) {
            left->_keys.push_back(std::move(key));
        }
        for (auto& child_ptr: right->_pointers) {
            left->_pointers.push_back(child_ptr);
        }
        parent->_keys.erase(parent->_keys.begin() + index);
        parent->_pointers.erase(parent->_pointers.begin() + index + 1);
        _allocator.template delete_object<btree_node>(right);
        if (parent == _root && parent->_keys.empty()) {
            _allocator.template delete_object<btree_node>(_root);
            _root = left;
            path = std::stack<std::pair<btree_node**, size_t>>();
        }
    }

    bool is_node_full(const btree_node* node) const {
        return node->_keys.size() > maximum_keys_in_node;
    }

    std::pair<std::stack<std::pair<btree_node**, size_t>>, size_t>
    insert_inner(tree_data_type&& data, std::stack<std::pair<btree_node**, size_t>>& path) {
        if (_root == nullptr) {
            _root = _allocator.template new_object<btree_node>();
            _root->_keys.push_back(std::move(data));
            path.push({&_root, 0});
            _size = 1;
            return {std::move(path), 0};
        }
        btree_node* current = *path.top().first;
        auto [index, is_exist] = find_key_position(data.first, current);
        if (is_exist) {
            current->_keys[index].second = std::move(data.second);
            return {std::move(path), index};
        }
        current->_keys.insert(current->_keys.begin() + index, std::move(data));
        ++_size;
        while (!path.empty()) {
            btree_node*& node = *path.top().first;
            if (!is_node_full(node)) {
                break;
            }
            split_and_promote_node(path, node, index);
        }
        return {std::move(path), index};
    }

    void transfer_from_left(btree_node* parent, size_t index) {
        btree_node* current = parent->_pointers[index];
        btree_node* brother = parent->_pointers[index - 1];
        current->_keys.insert(current->_keys.begin(), std::move(parent->_keys[index - 1]));
        parent->_keys[index - 1] = std::move(brother->_keys.back());
        brother->_keys.pop_back();
        if (is_leaf_node(current)) {
            return;
        }
        current->_pointers.insert(current->_pointers.begin(), brother->_pointers.back());
        brother->_pointers.pop_back();
    }

    void transfer_from_right(btree_node* parent, size_t index) {
        btree_node* current = parent->_pointers[index];
        btree_node* brother = parent->_pointers[index + 1];
        current->_keys.push_back(std::move(parent->_keys[index]));
        parent->_keys[index] = std::move(brother->_keys.front());
        brother->_keys.erase(brother->_keys.begin());
        if (is_leaf_node(current)) {
            return;
        }
        current->_pointers.push_back(brother->_pointers.front());
        brother->_pointers.erase(brother->_pointers.begin());
    }

    bool is_node_correct(btree_node* current) {
        return current->_keys.size() >= minimum_keys_in_node;
    }

    std::pair<std::stack<std::pair<btree_node**, size_t>>, size_t>
    erase_inner(std::stack<std::pair<btree_node**, size_t>>& path, size_t& index) noexcept {
        btree_node* node = *path.top().first;
        node->_keys.erase(node->_keys.begin() + index);
        --_size;
        while (!path.empty()) {
            btree_node* current = *path.top().first;
            if (is_node_correct(current) || current == _root) {
                break;
            }
            auto [parent_ptr, parent_index] = path.top();
            if (path.size() < 2) {
                break;
            }
            path.pop();
            auto [grandparent, grandparent_index] = path.top();
            btree_node* parent = *grandparent;
            bool can_transfer_left = parent_index > 0 && parent->_pointers[parent_index - 1]->_keys.size() > minimum_keys_in_node;
            bool can_transfer_right = parent_index + 1 < parent->_pointers.size() &&
                                      parent->_pointers[parent_index + 1]->_keys.size() > minimum_keys_in_node;
            if (can_transfer_left) {
                transfer_from_left(parent, parent_index);
            } else if (can_transfer_right) {
                transfer_from_right(parent, parent_index);
            } else {
                merge_child_nodes(path, parent, parent_index);
            }
            index = parent_index;
        }
        return {std::move(path), index};
    }

    bool is_leaf_node(const btree_node *node) const noexcept {
        return node == nullptr || node->_pointers.empty();
    }

    std::pair<std::stack<std::pair<btree_node**, size_t>>, std::pair<size_t,bool>> trace_key_path(const tkey& key) const noexcept {
        std::stack<std::pair<btree_node**, size_t>> path;
        if (_root == nullptr) {
            return {std::move(path), {0, false}};
        }
        auto** node_ptr = const_cast<btree_node**>(&_root);
        size_t index = 0;
        bool found = false;
        while (!found) {
            btree_node* current_node = *node_ptr;
            auto [temp_index, exist] = find_key_position(key, current_node);
            index = temp_index;
            found = exist;
            path.push({node_ptr, index});
            if (is_leaf_node(current_node)) {
                break;
            }
            node_ptr = &current_node->_pointers[index];
        }
        return {std::move(path), {index, found}};
    }


    btree_node* copy_tree(const btree_node* node)
    {
        auto* new_node = _allocator.template new_object<btree_node>();
        new_node->_keys = node->_keys;
        for (btree_node* temp : node->_pointers) {
            if (temp != nullptr) {
                new_node->_pointers.push_back(copy_tree(temp));
            } else {
                new_node->_pointers.push_back(nullptr);
            }
        }
        return new_node;
    }
};


template<std::input_iterator iterator, compator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
B_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>(),
       logger *logger = nullptr) -> B_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;

template<typename tkey, typename tvalue, compator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
B_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>(),
       logger *logger = nullptr) -> B_tree<tkey, tvalue, compare, t>;

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_pairs(const B_tree::tree_data_type &lhs,
                                                     const B_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_node::btree_node() noexcept: _keys(), _pointers()
{
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
logger* B_tree<tkey, tvalue, compare, t>::get_logger() const noexcept
{
    return _logger;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
pp_allocator<typename B_tree<tkey, tvalue, compare, t>::value_type> B_tree<tkey, tvalue, compare, t>::get_allocator() const noexcept
{
    return _allocator;
}

// region constructors implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): _root(nullptr), _size(0), _allocator(alloc), _logger(logger), compare(cmp)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        pp_allocator<value_type> alloc,\
        const compare& comp,
        logger* logger): _root(nullptr), _size(0), _allocator(alloc), _logger(logger), compare(comp)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
B_tree<tkey, tvalue, compare, t>::B_tree(
        iterator begin,
        iterator end,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): _root(nullptr), _size(0), _allocator(alloc), _logger(logger), compare(cmp)
{
    iterator temp = begin;
    while (temp != end) {
        insert(*temp);
        ++temp;
    }
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(
        std::initializer_list<std::pair<tkey, tvalue>> data,
        const compare& cmp,
        pp_allocator<value_type> alloc,
        logger* logger): _root(nullptr), _size(0), _allocator(alloc), _logger(logger), compare(cmp)
{
    for (const auto& temp: data) {
        emplace(temp.first, temp.second);
    }
}

// endregion constructors implementation

// region five implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::~B_tree() noexcept
{
    clear();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(const B_tree& other):
        _allocator(other._allocator),
        _logger(other._logger),
        _root(nullptr),
        _size(0)
{
            if (other._root == nullptr) {
                return;
            }
            _root = copy_tree(other._root);
            _size = other._size;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(const B_tree& other)
{
    if (this == &other) {
        return *this;
    }
    clear();
    _allocator = other._allocator;
    _logger = other._logger;
    if (other._root != nullptr) {
        _root = copy_tree(other._root);
        _size = other._size;
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::B_tree(B_tree&& other) noexcept:
        _root(other._root),
        _allocator(std::move(other._allocator)),
        _size(other._size),
        _logger(other._logger)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>& B_tree<tkey, tvalue, compare, t>::operator=(B_tree&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    clear();
    _root = other._root;
    _allocator = std::move(other._allocator);
    _size = other._size;
    _logger = other._logger;
    return *this;
}

// endregion five implementation

// region iterators implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_iterator::btree_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path,
        size_t index
): _path(path), _index(index)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator*() const noexcept
{
    return (*(_path.top()))->_keys[_index];
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator->() const noexcept
{
    if (_path.empty()) {
        return nullptr;
    }
    return &((*(_path.top().first))->_keys[_index]);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }
    if (is_terminate_node()) {
        ++_index;
        while (!_path.empty() && _index == ((*(_path.top().first))->_keys).size()) {
            _index = _path.top().second;
            _path.pop();
        }
        return *this;
    }
    _path.push({&(*(_path.top().first))->_pointers[_index + 1], 0});
    _index = 0;
    while (!is_terminate_node()) {
        _path.push({&(*(_path.top().first))->_pointers[0], 0});
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator&
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }
    if (is_terminate_node()) {
        if (_index != 0) {
            --_index;
        } else {
            while (!_path.empty() && _index == 0) {
                _index = _path.top().second;
                _path.pop();
            }
        }
        return *this;
    }
    _path.push({&(*_path.top().first)->_pointers[_index], _index});
    while (!is_terminate_node()) {
        _index = (*(_path.top().first))->_keys.size();
        _path.push({&((*(_path.top().first))->_pointers[_index]), _index - 1});
    }
    _index = (*_path.top().first)->_keys.size() - 1;
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::btree_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    return (!_path.empty() && !other._path.empty() && _path.size() == other._path.size() &&
            *(_path.top().first) == *(other._path.top().first) && _index == other._index);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::depth() const noexcept
{
    return _path.size();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    btree_node* current_node = *_path.top().first;
    if (current_node != nullptr) {
        return current_node->_keys.size();
    }
    return 0;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) {
        return true;
    }
    btree_node* current_node = *_path.top().first;
    return current_node == nullptr || current_node->_pointers[0] == nullptr;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const std::stack<std::pair<btree_node* const*,
                size_t>>& path, size_t index):
        _path(path), _index(index)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::btree_const_iterator(
        const btree_iterator& it) noexcept: _index(it._index)
{
    std::stack<std::pair<btree_node**, size_t>> temp = it._path;
    std::stack<std::pair<btree_node* const*, size_t>> new_stack;
    std::vector<std::pair<btree_node** , size_t>> temp_stack;
    while (!temp.empty()) {
        temp_stack.push_back(temp.top());
        temp.pop();
    }
    for (auto iter = temp_stack.rbegin(); iter != temp_stack.rend(); ++iter) {
        auto* const* temp_ptr = const_cast<btree_node* const*>((*iter).first);
        new_stack.emplace(temp_ptr, iter->second);
    }
    _path = std::move(new_stack);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator*() const noexcept
{
    return (*(_path.top()).first)->_keys[_index];
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator->() const noexcept
{
    if (_path.empty()) {
        return nullptr;
    }
    return &static_cast<const value_type&>((*(_path.top().first))->_keys[_index]);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }
    if (is_terminate_node()) {
        ++_index;
        while (!_path.empty() && _index == (*(_path.top().first))->_keys.size()) {
            _index = _path.top().second;
            _path.pop();
        }
        return *this;
    }
    _path.push({&(*(_path.top().first))->_pointers[_index + 1], _index + 1});
    _index = 0;
    while (!is_terminate_node()) {
        _path.push({&(*(_path.top().first))->_pointers[0], 0});
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }
    if (is_terminate_node()) {
        if (_index == 0) {
            while (!_path.empty() && _index == 0) {
                _index = _path.top().second;
                _path.pop();
            }
        } else {
            --_index;
        }
        return *this;
    }
    _path.push({&(*_path.top().first)->_pointers[_index], _index});
    while (!is_terminate_node()) {
        _index = (*(_path.top().first))->_keys.size();
        _path.push({&((*(_path.top().first))->_pointers[_index]), _index - 1});
    }
    _index = (*_path.top().first)->_keys.size() - 1;
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    return (!_path.empty() && !other._path.empty() && _path.size() == other._path.size() &&
            *(_path.top().first) == *(other._path.top().first) && _index == other._index);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::depth() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    return _path.size() - 1;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    btree_node* current_node = *_path.top().first;
    if (current_node != nullptr) {
        return current_node->_keys.size();
    }
    return 0;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) {
        return true;
    }
    const btree_node* current_node = *_path.top().first;
    return current_node == nullptr || current_node->_pointers.size() == 0;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const std::stack<std::pair<btree_node**, size_t>>& path, size_t index): _path(path), _index(index)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::btree_reverse_iterator(
        const btree_iterator& it) noexcept
{
    btree_iterator temp = it;
    --temp;
    _path = temp._path;
    _index = temp._index;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_iterator() const noexcept
{
    btree_iterator temp(_path, _index);
    ++temp;
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator*() const noexcept
{
    return (*(_path.top()).first)->_keys[_index];
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator->() const noexcept
{
    if (_path.empty()) {
        return nullptr;
    }
    return &(*(_path.top().first))->_keys[_index];
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }
    auto temp = static_cast<btree_iterator>(*this);
    --temp;
    *this = btree_reverse_iterator(temp);
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }
    auto temp = static_cast<btree_iterator>(*this);
    ++temp;
    *this = btree_reverse_iterator(temp);
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    return (!_path.empty() && !other._path.empty() && _path.size() == other._path.size() &&
            *(_path.top().first) == *(other._path.top().first) && _index == other._index);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::depth() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    return _path.size();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    btree_node* current_node = *_path.top().first;
    if (current_node != nullptr) {
        return current_node->_keys.size();
    }
    return 0;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) {
        return true;
    }
    const btree_node* current_node = *_path.top().first;
    return current_node == nullptr || current_node->_pointers[0] == nullptr;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator::index() const noexcept
{
    return _index;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const std::stack<std::pair<btree_node* const*, size_t>>& path, size_t index): _path(path), _index(index)
{}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::btree_const_reverse_iterator(
        const btree_reverse_iterator& it) noexcept: _index(it._index)
{
    std::stack<std::pair<btree_node* const*, size_t>> new_stack;
    std::stack<std::pair<btree_node**, size_t>> temp = it._path;
    std::vector<std::pair<btree_node**, size_t>> temp_stack;
    while (!temp.empty()) {
        temp_stack.push_back(temp.top());
        temp.pop();
    }
    for (auto iter = temp_stack.rbegin(); iter != temp_stack.rend(); ++iter) {
        auto* const* ptr = const_cast<btree_node* const*>(iter->first);
        new_stack.emplace(ptr, iter->second);
    }
    _path = std::move(new_stack);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator B_tree<tkey, tvalue, compare, t>::btree_const_iterator() const noexcept
{
    btree_const_iterator temp(_path, _index);
    ++temp;
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::reference
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator*() const noexcept
{
    return (*(_path.top()).first)->_keys[_index];
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::pointer
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator->() const noexcept
{
    if (_path.empty()) {
        return nullptr;
    }
    return &(*(_path.top().first))->_keys[_index];
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++()
{
    if (_path.empty()) {
        return *this;
    }
    auto temp = static_cast<btree_const_iterator>(*this);
    --temp;
    *this = btree_reverse_iterator(temp);
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator&
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--()
{
    if (_path.empty()) {
        return *this;
    }
    auto temp = static_cast<btree_const_iterator>(*this);
    ++temp;
    *this = btree_reverse_iterator(temp);
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator
B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator--(int)
{
    auto temp = *this;
    --(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator==(const self& other) const noexcept
{
    if (_path.empty() && other._path.empty()) {
        return true;
    }
    return (!_path.empty() && !other._path.empty() && _path.size() == other._path.size() &&
            *(_path.top().first) == *(other._path.top().first) && _index == other._index);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::operator!=(const self& other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::depth() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    return _path.size();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::current_node_keys_count() const noexcept
{
    if (_path.empty()) {
        return 0;
    }
    btree_node* current_node = *_path.top().first;
    if (current_node != nullptr) {
        return current_node->_keys.size();
    }
    return 0;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::is_terminate_node() const noexcept
{
    if (_path.empty()) {
        return true;
    }
    const btree_node* current_node = *_path.top().first;
    return current_node == nullptr || current_node->_pointers[0] == nullptr;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator::index() const noexcept
{
    return _index;
}

// endregion iterators implementation

// region element access implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key)
{
    auto key_path = trace_key_path(key);
    if (key_path.second.second == false) {
        throw std::logic_error("this key doesn't exist in tree");
    }
    return ((*(key_path.first.top().first))->_keys[key_path.second.first]).second;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
const tvalue& B_tree<tkey, tvalue, compare, t>::at(const tkey& key) const
{
    auto key_path = trace_key_path(key);
    if (key_path.second.second == false) {
        throw std::logic_error("this key doesn't exist in tree");
    }
    return ((*(key_path.first.top().first))->_keys[key_path.second.first]).second;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](const tkey& key)
{
    auto key_path = trace_key_path(key);
    if (key_path.second.second == false) {
        throw std::logic_error("this key doesn't exist in tree");
    }
    return ((*(key_path.first.top().first))->_keys[key_path.second.first]).second;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
tvalue& B_tree<tkey, tvalue, compare, t>::operator[](tkey&& key)
{
    auto key_path = trace_key_path(key);
    if (key_path.second.second == false) {
        throw std::logic_error("this key doesn't exist in tree");
    }
    return ((*(key_path.first.top().first))->_keys[key_path.second.first]).second;
}

// endregion element access implementation

// region iterator begins implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) {
        return end();
    }
    std::stack<std::pair<btree_node**, size_t>> begin_stack;
    begin_stack.push({&_root, 0});
    btree_node* current = _root;
    while (!is_leaf_node(current)) {
        current = current->_pointers[0];
        begin_stack.push({&current, 0});
    }
    return btree_iterator(std::move(begin_stack), 0);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::end()
{
    return btree_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::begin() const
{
    if (_root == nullptr) {
        return end();
    }
    std::stack<std::pair<btree_node* const*, size_t>> begin_stack;
    begin_stack.push({ &_root, 0 });
    auto** current = const_cast<btree_node**>(&_root);
    while (!is_leaf_node(*current)) {
        current = &((*current)->_pointers[0]);
        begin_stack.push({current, 0});
    }
    return btree_const_iterator(std::move(begin_stack), 0);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::end() const
{
    return btree_const_iterator();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cbegin() const
{
    return begin();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::cend() const
{
    return end();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin()
{
    return btree_reverse_iterator(end());
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend()
{
    return btree_reverse_iterator(begin());
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rbegin() const
{
    return btree_const_reverse_iterator(btree_reverse_iterator(end()));
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::rend() const
{
    return btree_const_reverse_iterator(btree_reverse_iterator(begin()));
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crbegin() const
{
    return rbegin();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_reverse_iterator B_tree<tkey, tvalue, compare, t>::crend() const
{
    return rend();
}

// endregion iterator begins implementation

// region lookup implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _root == nullptr;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{

    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (!exist) {
        return end();
    }
    return btree_iterator(key_path, index);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (!exist) {
        return end();
    }
    return btree_iterator(key_path, index);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (exist) {
        return btree_iterator(std::move(key_path), index);
    }
    if (key_path.empty()) {
        return end();
    }
    if (index < (*key_path.top().first)->_keys.size()) {
        return btree_iterator(std::move(key_path), index);
    }
    btree_iterator iter(std::move(key_path), index);
    ++iter;
    return iter;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (exist) {
        return btree_const_iterator(std::move(key_path), index);
    }
    if (key_path.empty()) {
        return end();
    }
    if (index < (*key_path.top().first)->_keys.size()) {
        return btree_const_iterator(std::move(key_path), index);
    }
    btree_const_iterator iter(std::move(key_path), index);
    ++iter;
    return iter;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (key_path.empty()) {
        return end();
    }
    if (exist) {
        auto iter = btree_iterator(std::move(key_path), index);
        ++iter;
        return iter;
    }
    if (index < (*key_path.top().first)->_keys.size()) {
        return btree_iterator(std::move(key_path), index);
    }
    btree_iterator iter(std::move(key_path), index);
    ++iter;
    return iter;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_const_iterator B_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (key_path.empty()) {
        return end();
    }
    if (exist) {
        auto iter = btree_const_iterator(std::move(key_path), index);
        ++iter;
        return iter;
    }
    if (index < (*key_path.top().first)->_keys.size()) {
        return btree_const_iterator(std::move(key_path), index);
    }
    btree_const_iterator iter(std::move(key_path), index);
    ++iter;
    return iter;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool B_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    auto key_path = trace_key_path(key);
    return key_path.second.second;
}

// endregion lookup implementation

// region modifiers implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void B_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    if (_root == nullptr) {
        return;
    }
    std::stack<btree_node*> nodes;
    nodes.push(_root);
    while (!nodes.empty()) {
        btree_node* current = nodes.top();
        nodes.pop();
        if (!is_leaf_node(current)) {
            for (auto* temp : current->_pointers) {
                if (temp != nullptr) {
                    nodes.push(temp);
                }
            }
        }
        _allocator.template delete_object<btree_node>(current);
    }
    _root = nullptr;
    _size = 0;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(const tree_data_type& data)
{
    return insert(tree_data_type(data));
}
template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    auto [key_path, index_found] = trace_key_path(data.first);
    auto [index, exist] = index_found;
    if (exist) {
        return {btree_iterator(std::move(key_path), index), false};
    }
    auto [new_path, new_index] = insert_inner(std::move(data), key_path);
    return {btree_iterator(std::move(new_path), new_index), true};
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
template<typename... Args>
std::pair<typename B_tree<tkey, tvalue, compare, t>::btree_iterator, bool>
B_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    tree_data_type value(std::forward<Args>(args)...);
    return insert(value);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto [iter, is_exist] = insert(data);
    if (!is_exist) {
        (*iter).second = data.second;
    }
    return iter;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    auto [iter, is_exist] = insert(std::move(data));
    if (!is_exist) {
        (*iter).second = std::move(data.second);
    }
    return iter;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
template<typename... Args>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type new_value(std::forward<Args>(args)...);
    return insert_or_assign(std::move(new_value));
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator pos)
{
    return erase(*pos);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator pos)
{
    return erase(*pos);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_iterator beg, btree_iterator en)
{
    btree_iterator iter = beg;
    btree_iterator next = end();
    while (iter != en) {
        next = erase((*iter).first);
        ++iter;
    }
    return next;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(btree_const_iterator beg, btree_const_iterator en)
{
    btree_const_iterator iter = beg;
    btree_const_iterator next = end();
    while (iter != en) {
        next = erase((*iter).first);
        ++iter;
    }
    return next;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename B_tree<tkey, tvalue, compare, t>::btree_iterator
B_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    auto [key_path, index_found] = trace_key_path(key);
    auto [index, exist] = index_found;
    if (!exist) {
        return end();
    }
    auto [new_path, new_index] = erase_inner(key_path, index);
    return btree_iterator(std::move(new_path), new_index);
}

// endregion modifiers implementation

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool compare_pairs(const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &lhs,
                   const typename B_tree<tkey, tvalue, compare, t>::tree_data_type &rhs)
{
    return compare::operator()(lhs.first, rhs.first);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool compare_keys(const tkey &lhs, const tkey &rhs)
{
    return compare::operator()(lhs, rhs);
}



#endif