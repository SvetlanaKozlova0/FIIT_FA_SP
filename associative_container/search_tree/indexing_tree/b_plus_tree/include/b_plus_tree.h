#include <iterator>
#include <utility>
#include <vector>
#include <boost/container/static_vector.hpp>
#include <concepts>
#include <stack>
#include <pp_allocator.h>
#include <search_tree.h>
#include <initializer_list>
#include <logger_guardant.h>

#ifndef MP_OS_B_PLUS_TREE_H
#define MP_OS_B_PLUS_TREE_H

template <typename tkey, typename tvalue, compator<tkey> compare = std::less<tkey>, std::size_t t = 5>
class BP_tree final : private logger_guardant, private compare
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

    struct bptree_node_base
    {
        bool _is_terminate;

        bptree_node_base() noexcept;
        virtual ~bptree_node_base() =default;
        size_t size();
    };

    struct bptree_node_term : public bptree_node_base
    {
        bptree_node_term* _next;
        boost::container::static_vector<tree_data_type, maximum_keys_in_node + 1> _data;
        bptree_node_term() noexcept;
    };

    struct bptree_node_middle : public bptree_node_base
    {
        boost::container::static_vector<tkey, maximum_keys_in_node + 1> _keys;
        boost::container::static_vector<bptree_node_base*, maximum_keys_in_node + 2> _pointers;
        bptree_node_middle() noexcept;
    };

    pp_allocator<value_type> _allocator;
    logger* _logger;
    bptree_node_base* _root;
    size_t _size;

    logger* get_logger() const noexcept override;
    pp_allocator<value_type> get_allocator() const noexcept;

public:

    // region constructors declaration

    explicit BP_tree(const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>(), logger* logger = nullptr);

    explicit BP_tree(pp_allocator<value_type> alloc, const compare& comp = compare(), logger *logger = nullptr);

    template<input_iterator_for_pair<tkey, tvalue> iterator>
    explicit BP_tree(iterator begin, iterator end, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>(), logger* logger = nullptr);

    BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp = compare(), pp_allocator<value_type> = pp_allocator<value_type>(), logger* logger = nullptr);

    // endregion constructors declaration

    // region five declaration

    BP_tree(const BP_tree& other);

    BP_tree(BP_tree&& other) noexcept;

    BP_tree& operator=(const BP_tree& other);

    BP_tree& operator=(BP_tree&& other) noexcept;

    ~BP_tree() noexcept override;

    // endregion five declaration

    // region iterators declaration

    class bptree_iterator;
    class bptree_const_iterator;

    class bptree_iterator final
    {
        bptree_node_term* _node;
        size_t _index;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_iterator;

        friend class BP_tree;
        friend class bptree_const_iterator;

        reference operator*() const;
        pointer operator->() const;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_iterator(bptree_node_term* node = nullptr, size_t index = 0);

    };

    class bptree_const_iterator final
    {
        const bptree_node_term* _node;
        size_t _index;

    public:

        using value_type = tree_data_type_const;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using self = bptree_const_iterator;

        friend class BP_tree;
        friend class bptree_iterator;

        bptree_const_iterator(const bptree_iterator& it) noexcept;

        reference operator*() const;
        pointer operator->() const;

        self& operator++();
        self operator++(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        size_t current_node_keys_count() const noexcept;
        size_t index() const noexcept;

        explicit bptree_const_iterator(const bptree_node_term* node = nullptr, size_t index = 0);
    };

    friend class btree_iterator;
    friend class btree_const_iterator;

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

    bptree_iterator begin();
    bptree_iterator end();

    bptree_const_iterator begin() const;
    bptree_const_iterator end() const;

    bptree_const_iterator cbegin() const;
    bptree_const_iterator cend() const;

    // endregion iterator begins declaration

    // region lookup declaration

    size_t size() const noexcept;
    bool empty() const noexcept;

    /*
     * Returns end() if not exist
     */

    bptree_iterator find(const tkey& key);
    bptree_const_iterator find(const tkey& key) const;

    bptree_iterator lower_bound(const tkey& key);
    bptree_const_iterator lower_bound(const tkey& key) const;

    bptree_iterator upper_bound(const tkey& key);
    bptree_const_iterator upper_bound(const tkey& key) const;

    bool contains(const tkey& key) const;

    // endregion lookup declaration

    // region modifiers declaration

    void clear() noexcept;

    /*
     * Does nothing if key exists, delegates to emplace.
     * Second return value is true, when inserted
     */
    std::pair<bptree_iterator, bool> insert(const tree_data_type& data);
    std::pair<bptree_iterator, bool> insert(tree_data_type&& data);

    template <typename ...Args>
    std::pair<bptree_iterator, bool> emplace(Args&&... args);

    /*
     * Updates value if key exists, delegates to emplace.
     */
    bptree_iterator insert_or_assign(const tree_data_type& data);
    bptree_iterator insert_or_assign(tree_data_type&& data);

    template <typename ...Args>
    bptree_iterator emplace_or_assign(Args&&... args);

    /*
     * Return iterator to node next ro removed or end() if key not exists
     */
    bptree_iterator erase(bptree_iterator pos);
    bptree_iterator erase(bptree_const_iterator pos);

    bptree_iterator erase(bptree_iterator beg, bptree_iterator en);
    bptree_iterator erase(bptree_const_iterator beg, bptree_const_iterator en);

    bptree_iterator erase(const tkey& key);

    void delete_from_term(size_t, bptree_node_term*, std::stack<std::pair<bptree_node_base*, size_t>>);

    void merge(bptree_node_base*, bptree_node_base*, bptree_node_middle*, size_t);

    void merge_term_nodes(bptree_node_term*, bptree_node_term*, bptree_node_middle*, size_t);

    void merge_middle_nodes(bptree_node_middle*, bptree_node_middle*, bptree_node_middle*, size_t);

    bool not_term(bptree_node_base* current) const noexcept;

    size_t find_index(bptree_node_base* current, const tkey& key) const noexcept;

    void split(bptree_node_base*, bptree_node_middle*);

    // endregion modifiers declaration
};

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t
BP_tree<tkey, tvalue, compare, t>::find_index(BP_tree::bptree_node_base *current, const tkey &key) const noexcept {
    size_t index = 0;
    auto middle_current = static_cast<bptree_node_middle*>(current);
    while (index < middle_current->_keys.size() && compare_keys(middle_current->_keys[index], key)){
        index++;
    }
    return index;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::not_term(BP_tree::bptree_node_base *current) const noexcept {
    return !current->_is_terminate && static_cast<bptree_node_middle*>(current)->_pointers.size() > 0
    && static_cast<bptree_node_middle*>(current)->_pointers[0] != nullptr;

}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_node_base::size() {
    if (_is_terminate){
        return static_cast<bptree_node_term*>(this)->_data.size();
    } else {
        return static_cast<bptree_node_middle*>(this)->_keys.size();
    }
}




template<std::input_iterator iterator, compator<typename std::iterator_traits<iterator>::value_type::first_type> compare = std::less<typename std::iterator_traits<iterator>::value_type::first_type>,
        std::size_t t = 5, typename U>
BP_tree(iterator begin, iterator end, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>(),
        logger *logger = nullptr) -> BP_tree<typename std::iterator_traits<iterator>::value_type::first_type, typename std::iterator_traits<iterator>::value_type::second_type, compare, t>;


template<typename tkey, typename tvalue, compator<tkey> compare = std::less<tkey>, std::size_t t = 5, typename U>
BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare &cmp = compare(), pp_allocator<U> = pp_allocator<U>(),
        logger *logger = nullptr) -> BP_tree<tkey, tvalue, compare, t>;


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_pairs(const BP_tree::tree_data_type &lhs,
                                                      const BP_tree::tree_data_type &rhs) const
{
    return compare_keys(lhs.first, rhs.first);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_base::bptree_node_base() noexcept: _is_terminate(false)
{
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_term::bptree_node_term() noexcept : _next(nullptr), _data()
{
    bptree_node_base::_is_terminate = true;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_node_middle::bptree_node_middle() noexcept : _keys(), _pointers()
{
    bptree_node_base::_is_terminate = false;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
logger * BP_tree<tkey, tvalue, compare, t>::get_logger() const noexcept
{
    return _logger;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
pp_allocator<typename BP_tree<tkey, tvalue, compare, t>::value_type> BP_tree<tkey, tvalue, compare, t>::
get_allocator() const noexcept
{
    return _allocator;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::reference BP_tree<tkey, tvalue, compare, t>::
bptree_iterator::operator*() const
{
    if (_node == nullptr){
        throw std::runtime_error("dereference nullptr");
    }
    return reinterpret_cast<reference>(_node->_data[_index]);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::pointer BP_tree<tkey, tvalue, compare, t>::bptree_iterator
::operator->() const
{
    if (_node == nullptr){
        throw std::runtime_error("dereference nullptr");
    }
    return reinterpret_cast<pointer>(&(_node->_data[_index]));
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self & BP_tree<tkey, tvalue, compare, t>::bptree_iterator::
operator++()
{
    if (_index + 1 == _node->_data.size() && _node->_next != nullptr) {
        _index = 0;
        _node = _node->_next;
    } else {
        ++_index;
    }
    return *this;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator::self BP_tree<tkey, tvalue, compare, t>::bptree_iterator::
operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator==(const self &other) const noexcept
{
    return _index == other._index && _node == other._node;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_iterator::operator!=(const self &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::current_node_keys_count() const noexcept
{
    return _node->_data.size();
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_iterator::index() const noexcept
{
    return _index;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_iterator::bptree_iterator(bptree_node_term *node, size_t index) : _index(index), _node(node)
{
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_iterator &it) noexcept: _node(it._node), _index(it._index)
{
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::reference BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator*() const
{
    if (_node == nullptr){
        throw std::runtime_error("dereference nullptr");
    }
    return reinterpret_cast<reference>(_node->_data[_index]);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::pointer BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator->() const
{
    if (_node == nullptr){
        throw std::runtime_error("dereference nullptr");
    }
    return reinterpret_cast<pointer>(&(_node->_data[_index]));
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self & BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator++()
{
    if (_index + 1 == _node->_data.size() && _node->_next != nullptr) {
        _index = 0;
        _node = _node->_next;
    } else {
        ++_index;
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::self BP_tree<tkey, tvalue, compare, t>::
bptree_const_iterator::operator++(int)
{
    auto temp = *this;
    ++(*this);
    return temp;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator==(const self &other) const noexcept
{
    return _index == other._index && _node == other._node;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::operator!=(const self &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::current_node_keys_count() const noexcept
{
    return _node->_data.size();
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::index() const noexcept {
    return _index;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator::bptree_const_iterator(const bptree_node_term *node, size_t index)
: _index(index), _node(node)
{
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
tvalue & BP_tree<tkey, tvalue, compare, t>::at(const tkey & key)
{
    auto iter = find(key);
    if (iter == end()){
        throw std::out_of_range("this key doesn't exist");
    }
    return iter->second;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
const tvalue & BP_tree<tkey, tvalue, compare, t>::at(const tkey & key) const
{
    auto iter = find(key);
    if (iter == end()){
        throw std::out_of_range("this key doesn't exist");
    }
    return iter->second;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
tvalue & BP_tree<tkey, tvalue, compare, t>::operator[](const tkey &key)
{
    return find(key)->second;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
tvalue & BP_tree<tkey, tvalue, compare, t>::operator[](tkey &&key)
{
    return find(std::forward<tkey&&>(key))->second;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::insert(
        const tree_data_type &data)
{
    if (_root == nullptr) {
        _root = _allocator.template new_object<bptree_node_term>();
        auto new_root = static_cast<bptree_node_term *>(_root);
        new_root->_next = nullptr;
        new_root->_data.push_back(data);
        _size++;
        return std::pair(bptree_iterator(), true);
    }
    size_t index_in_parent = 0;
    bptree_node_base *parent = nullptr;
    auto current = _root;
    std::stack<std::pair<bptree_node_base*, size_t>> path = std::stack<std::pair<bptree_node_base*, size_t>>();
    size_t i = 0;
    while (!current->_is_terminate) {
        i = 0;
        index_in_parent = 0;
        auto current_middle = static_cast<bptree_node_middle*>(current);
        while (i < current_middle->_keys.size() && compare_keys(current_middle->_keys[i], data.first)) {
            index_in_parent = i;
            i++;
        }
        auto current_key = current_middle->_keys[index_in_parent];
        if (current_key == data.first) {
            return std::pair<bptree_iterator, bool>(end(), false);
        }
        parent = current;
        if (i < current_middle->_pointers.size()) {
            current = current_middle->_pointers[i];
        } else {
            current = nullptr;
        }
        path.push(std::pair(current, index_in_parent));
    }
    auto term_node = static_cast<bptree_node_term*>(current);
    i = 0;
    while (i < term_node->_data.size() && compare_keys(term_node->_data[i].first, data.first)) {
        i++;
    }
    auto inserted_node = term_node;
    if (inserted_node->_data.size() == maximum_keys_in_node){
        inserted_node->_data.insert(inserted_node->_data.begin() + i, data);
        split(current, static_cast<bptree_node_middle*>(parent));
    } else {
        inserted_node->_data.insert(inserted_node->_data.begin() + i, data);
    }
    _size++;
    return std::pair(bptree_iterator(inserted_node, i), true);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::split(BP_tree::bptree_node_base * node, BP_tree::bptree_node_middle* parent) {
    size_t middle_index = (maximum_keys_in_node + 1) / 2;
    auto term_node = static_cast<bptree_node_term*>(node);
    tkey middle_key = term_node->_data[middle_index].first;
    size_t index_in_parent;
    bool is_root = parent == nullptr;
    if (is_root) {
        _root = _allocator.template new_object<bptree_node_middle>();
        parent = static_cast<bptree_node_middle*>(_root);
        _root->_is_terminate = false;
        index_in_parent = 0;
    } else {
        size_t index = 0;
        for (index = 0; index < parent->_keys.size(); index++){
            if (!compare_keys(parent->_keys[index], middle_key)){
                break;
            }
        }
        index_in_parent = index;
    }
    bptree_node_term* left_son = _allocator.template new_object<bptree_node_term>();
    bptree_node_term* right_son = _allocator.template new_object<bptree_node_term>();
    if (left_son == nullptr || right_son == nullptr){
        if (left_son != nullptr){
            _allocator.template delete_object<bptree_node_term >(left_son);
        }
        if (right_son != nullptr){
            _allocator.template delete_object<bptree_node_term >(right_son);
        }
    }
    const auto begin_iter = term_node->_data.begin();
    const auto end_iter = term_node->_data.end();
    right_son->_data.insert(right_son->_data.begin(), begin_iter + middle_index, end_iter);
    auto temp = term_node->_next;
    term_node->_next = right_son;
    right_son->_next = temp;
    parent->_keys.insert(parent->_keys.begin() + index_in_parent, term_node->_data[middle_index].first);
    if (is_root) {
        parent->_pointers.insert(parent->_pointers.begin() + index_in_parent, term_node);
    }
    parent->_pointers.insert(parent->_pointers.begin() + index_in_parent + 1, right_son);
    for (size_t i = parent->_keys.size() + 1; i < parent->_pointers.size(); i++){
        parent->_pointers[i] = nullptr;
    }
    term_node->_data.assign(begin_iter, begin_iter + middle_index);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::compare_keys(const tkey &lhs, const tkey &rhs) const
{
    return compare::operator()(lhs, rhs);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const compare& cmp, pp_allocator<value_type> alloc, logger* logger):
_allocator(alloc), _logger(logger), _root(nullptr), _size(0)
{
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(pp_allocator<value_type> alloc, const compare& cmp, logger* logger):
_allocator(alloc), _logger(logger), _root(nullptr), _size(0)
{
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
template<input_iterator_for_pair<tkey, tvalue> iterator>
BP_tree<tkey, tvalue, compare, t>::BP_tree(iterator begin, iterator end, const compare& cmp, pp_allocator<value_type> alloc, logger* logger):
_logger(logger), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto iter = begin; iter != end; iter++){
        insert(*iter);
    }
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(std::initializer_list<std::pair<tkey, tvalue>> data, const compare& cmp, pp_allocator<value_type> alloc, logger* logger):
_logger(logger), _allocator(alloc), _root(nullptr), _size(0)
{
    for (auto element: data) {
        insert(element);
    }
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(const BP_tree& other): _logger(other._logger), _allocator(other._allocator), _root(nullptr), _size(0)
{
    for (auto iter = other.begin(); iter != other.end(); iter++){
        insert(iter);
    }
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::BP_tree(BP_tree&& other) noexcept:
_logger(other._logger), _allocator(other._allocator), _root(other._root), _size(other._size)
{
    other._size = 0;
    other._root = nullptr;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(const BP_tree& other)
{
    if (this == &other) {
        return *this;
    }
    clear();
    _logger = other._logger;
    _allocator = other._allocator;
    for (auto iter = other.begin(); iter != other.end(); iter++){
        insert(iter);
    }
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>& BP_tree<tkey, tvalue, compare, t>::operator=(BP_tree&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    _logger = other._logger;
    _allocator = other._allocator;
    _root = other._root;
    _size = std::exchange(other._size, 0);
    _root = std::exchange(other._root, nullptr);
    return *this;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
BP_tree<tkey, tvalue, compare, t>::~BP_tree() noexcept
{
    clear();
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::begin()
{
    if (_root == nullptr) {
        return bptree_iterator(nullptr, 0);
    }
    bptree_node_base *current = _root;
    while (not_term(current)) {
        current = static_cast<bptree_node_middle*>(current)->_pointers[0];
    }
    return bptree_iterator(static_cast<bptree_node_term*>(current), 0);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::end()
{
    if (_root == nullptr){
        return bptree_iterator(nullptr,  0);
    }
    bptree_node_base* current = _root;
    while (!current->_is_terminate && static_cast<bptree_node_middle*>(current)->_pointers.size() > 0) {
        auto middle_node = static_cast<bptree_node_middle*>(current);
        size_t last_index = middle_node->_pointers.size() - 1;
        if (middle_node->_pointers[last_index] == nullptr) {
            break;
        }
        current = middle_node->_pointers[last_index];
    }
    auto end_node = static_cast<bptree_node_term*>(current);
    return bptree_iterator(end_node, end_node->_data.size());
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::begin() const
{
    if (_root == nullptr) {
        return bptree_iterator(nullptr, 0);
    }
    bptree_node_base *current = _root;
    while (not_term(current)) {
        current = current->_pointers[0];
    }
    return bptree_iterator(static_cast<bptree_node_term*>(current), 0);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::end() const
{
    if (_root == nullptr){
        return bptree_iterator(nullptr,  0);
    }
    bptree_node_base* current = _root;
    while (!current->_is_terminate && static_cast<bptree_node_middle*>(current)->_pointers.size() > 0) {
        size_t last_index = static_cast<bptree_node_middle*>(current)->_pointers.size() - 1;
        if (current->_pointers[last_index] == nullptr) {
            break;
        }
        current = current->_pointers[last_index];
    }
    auto end_node = static_cast<bptree_node_term*>(current);
    return bptree_iterator(end_node, end_node->_data.size());
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cbegin() const
{
    if (_root == nullptr) {
        return bptree_const_iterator(nullptr, 0);
    }
    bptree_node_base *current = _root;
    while (not_term(current)) {
        current = static_cast<bptree_node_middle*>(current)->_pointers[0];
    }
    auto begin_node = bptree_const_iterator(static_cast<bptree_node_term*>(current), 0);
    return begin_node;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::cend() const
{
    if (_root == nullptr){
        return bptree_const_iterator(nullptr,  0);
    }
    bptree_node_base * current = _root;
    while (!current->_is_terminate
           && static_cast<bptree_node_middle*>(current)->_pointers.size() > 0) {
        auto middle_node = static_cast<bptree_node_middle*>(current);
        size_t last_index = middle_node->_pointers.size() - 1;
        if (middle_node->_pointers[last_index] == nullptr) {
            break;
        }
        current = middle_node->_pointers[last_index];
    }
    auto end_node = static_cast<bptree_node_term*>(current);
    return bptree_const_iterator(end_node, end_node->_data.size());
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
size_t BP_tree<tkey, tvalue, compare, t>::size() const noexcept
{
    return _size;
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::empty() const noexcept
{
    return _size == 0;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key)
{
    size_t index = 0;
    auto current = _root;
    while (current != nullptr && !current->_is_terminate){
        index = find_index(current, key);
        auto middle_current = static_cast<bptree_node_middle*>(current);
        if (index == middle_current->_keys.size() - 1 && key == middle_current->_keys[index]){
            current = middle_current->_pointers[index + 1];
        } else {
            current = middle_current->_pointers[index];
        }
    }
    if (current == nullptr){
        return end();
    }
    auto current_term = static_cast<bptree_node_term*>(current);
    index = 0;
    while (index < current_term->_data.size() && current_term->_data[index].first != key) {
        index++;
    }
    if (index != current_term->_data.size()){
        return bptree_iterator(current_term, index);
    }
    return end();
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::find(const tkey& key) const
{
    size_t index = 0;
    auto current = _root;
    while (current != nullptr && !current->_is_terminate){
        index = find_index(current, key);
        auto middle_current = static_cast<bptree_node_middle*>(current);
        if (index == middle_current->_keys.size() - 1 && key == middle_current->_keys[index]){
            current = middle_current->_pointers[index + 1];
        } else {
            current = middle_current->_pointers[index];
        }
    }
    if (current == nullptr) {
        return cend();
    }
    auto term_current = static_cast<bptree_node_term*>(current);
    index = 0;
    while (index < term_current->_data.size() && term_current->_data[index].first != key){
        index++;
    }
    if (index != term_current->_data.size()){
        return bptree_const_iterator(term_current, index);
    }
    return cend();
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key)
{
    size_t index = 0;
    size_t prev_index = 0;
    auto current = _root;
    std::stack<std::pair<bptree_node_base*, size_t>> path = std::stack<std::pair<bptree_node_base*, size_t>>();
    path.push(std::pair<bptree_node_base*, size_t>(_root, 0));
    while(current != nullptr){
        index = find_index(current, key);
        path.push(std::pair<bptree_node_base*, size_t>(current, prev_index));
        current = current->_pointers[index];
        prev_index = index;
    }
    if (path.size() == 1 && index == path.top().first->_keys.size()){
        return end();
    }
    path.pop();
    return bptree_const_iterator(path.top().first, prev_index);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::lower_bound(const tkey& key) const
{
    size_t index = 0;
    size_t prev_index = 0;
    auto current = _root;
    std::stack<std::pair<bptree_node_base*, size_t>> path = std::stack<std::pair<bptree_node_base*, size_t>>();
    path.push(std::pair<bptree_node_base*, size_t>(_root, 0));
    while(current != nullptr){
        index = find_index(current, key);
        path.push(std::pair<bptree_node_base*, size_t>(current, prev_index));
        current = current->_pointers[index];
        prev_index = index;
    }
    if (path.size() == 1 && index == path.top().first->_keys.size()){
        return end();
    }
    path.pop();
    return bptree_const_iterator(path.top().first, prev_index);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key)
{
    size_t index = 0;
    size_t prev_index = 0;
    auto current = _root;
    std::stack<std::pair<bptree_node_base*, size_t>> path = std::stack<std::pair<bptree_node_base*, size_t>>();
    path.push(std::pair<bptree_node_base*, size_t>(_root, 0));
    while (current != nullptr){
        index = 0;
        while (index < current->_keys.size() && !compare_keys(key, current->_keys[index].first)) {
            index++;
        }
        path.push(std::pair<bptree_node_base*, size_t>(current, prev_index));
        current = current->_pointers[index];
        prev_index = index;
    }
    if (path.size() == 1 && index == path.top().first->_keys.size()) {
        return end();
    }
    path.pop();
    return bptree_const_iterator(path.top().first, prev_index);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_const_iterator BP_tree<tkey, tvalue, compare, t>::upper_bound(const tkey& key) const
{
    size_t index = 0;
    size_t prev_index = 0;
    auto current = _root;
    std::stack<std::pair<bptree_node_base*, size_t>> path = std::stack<std::pair<bptree_node_base*, size_t>>();
    path.push(std::pair<bptree_node_base*, size_t>(_root, 0));
    while (current != nullptr){
        index = 0;
        while (index < current->_keys.size() && !compare_keys(key, current->_keys[index].first)) {
            index++;
        }
        path.push(std::pair<bptree_node_base*, size_t>(current, prev_index));
        current = current->_pointers[index];
        prev_index = index;
    }
    if (path.size() == 1 && index == path.top().first->_keys.size()) {
        return end();
    }
    path.pop();
    return bptree_const_iterator(path.top().first, prev_index);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
bool BP_tree<tkey, tvalue, compare, t>::contains(const tkey& key) const
{
    auto iter = find(key);
    return iter != end();
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::clear() noexcept
{
    while (!empty()) {
        erase(begin());
    }
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::insert(tree_data_type&& data)
{
    return insert(data);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
template <typename ...Args>
std::pair<typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator, bool> BP_tree<tkey, tvalue, compare, t>::emplace(Args&&... args)
{
    return this->insert(tree_data_type(args ...));
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(const tree_data_type& data)
{
    auto iter = find(data.first);
    if (iter != end()){
        iter->second = data.second;
    } else {
        insert(data);
    }
}

template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::insert_or_assign(tree_data_type&& data)
{
    return insert(data);
}



template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
template <typename ...Args>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::emplace_or_assign(Args&&... args)
{
    tree_data_type value(args ...);
    insert_or_assign(std::move(value));
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator pos)
{
    return erase(pos->first);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator pos)
{
    return erase(pos->first);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_iterator beg, bptree_iterator en)
{
    auto iter = beg;
    while(iter != en){
        iter = erase(iter);
    }
    return iter;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(bptree_const_iterator beg, bptree_const_iterator en)
{
    auto iter = beg;
    while (iter != en){
        iter = erase(iter);
    }
    return iter;
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
typename BP_tree<tkey, tvalue, compare, t>::bptree_iterator BP_tree<tkey, tvalue, compare, t>::erase(const tkey& key)
{
    if (_root == nullptr){
        throw std::runtime_error("tree is empty");
    }
    size_t index = 0;
    size_t prev_index = 0;
    auto current = _root;
    std::stack<std::pair<bptree_node_base *, size_t>> path = std::stack<std::pair<bptree_node_base *, size_t>>();
    while (!current->_is_terminate){
        auto current_middle = static_cast<bptree_node_middle*>(current);
        while (index < current_middle->_keys.size() && compare_keys(current_middle->_keys[index], key)){
            ++index;
        }
        path.push(std::pair(current, prev_index));
        if (index < current_middle->_keys.size() && current_middle->_keys[index] == key){
            break;
        }
        if (index >= current_middle->_pointers.size()){
            return end();
        }
        current = current_middle->_pointers[index];
        prev_index = index;
    }
    path.push(std::pair(current, prev_index));
    auto current_term = static_cast<bptree_node_term*>(current);
    index = 0;
    while (index < current_term->_data.size() && current_term->_data[index].first != key){
        ++index;
    }
    if (index == current_term->_data.size()){
        return end();
    }
    bptree_iterator next = bptree_iterator(current_term, index);
    next++;
    tkey next_key;
    if (next != end()) {
        next_key = next->first;
    }
    delete_from_term(index, static_cast<bptree_node_term*>(current), path);
    _size--;
    if (next == end()) {
        return end();
    }
    return find(next_key);
}



template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::delete_from_term(size_t index_of_key, BP_tree::bptree_node_term * node,
                                                             std::stack<std::pair<bptree_node_base*, size_t>> path)  {
    if (node->_data.size() > minimum_keys_in_node){
        node->_data.erase(node->_data.begin() + index_of_key);
        return;
    }
    bptree_node_base *current = path.top().first;
    size_t index = path.top().second;
    path.pop();
    bptree_node_middle *parent = nullptr;
    if (!path.empty()) {
        parent = static_cast<bptree_node_middle*>(path.top().first);
    } else {
        if (node->_data.size() > 1) {
            node->_data.erase(node->_data.begin() + index_of_key);
        } else {
            _root = nullptr;
            _allocator.delete_object(node);
        }
        return;
    }
    size_t size_left_bro = 0;
    size_t size_right_bro = 0;
    if (index > 0 && parent->_pointers[index - 1] != nullptr) {
        size_left_bro = parent->_pointers[index - 1]->size();
    }
    if (index + 1 < parent->_pointers.size() && parent->_pointers[index + 1] != nullptr) {
        size_right_bro = parent->_pointers[index + 1]->size();
    }
    bptree_node_term* left_brother = index > 0 ? static_cast<bptree_node_term*>(parent->_pointers[index - 1]) : nullptr;
    bptree_node_term* right_brother = index + 1 < parent->_pointers.size() ? static_cast<bptree_node_term*>(parent->_pointers[index + 1]) : nullptr;
    if (size_left_bro > minimum_keys_in_node) {
        node->_data.erase(node->_data.begin() + index_of_key);
        node->_data.insert(node->_data.begin(), left_brother->_data[size_left_bro - 1]);
        parent->_keys[index - 1] = left_brother->_data[size_left_bro - 1].first;
        left_brother->_data.erase(left_brother->_data.begin() + size_left_bro - 1);
    } else if (size_right_bro > minimum_keys_in_node) {
        node->_data.erase(node->_data.begin() + index_of_key);
        node->_data.insert(node->_data.end(), right_brother->_data[0]);
        parent->_keys[index] = right_brother->_data[1].first;
        right_brother->_data.erase(right_brother->_data.begin());
    } else {
        node->_data.erase(node->_data.begin() + index_of_key);
        if (size_left_bro > 0) {
            merge(left_brother, current, parent, index - 1);
        } else if (size_right_bro > 0) {
            merge(current, right_brother, parent, index);
        }
        path.pop();
        while (!path.empty() && parent->_keys.size() < minimum_keys_in_node){
            current = parent;
            parent = static_cast<bptree_node_middle*>(path.top().first);
            auto middle_left_bro = static_cast<bptree_node_middle*>(parent->_pointers[index - 1]);
            auto middle_right_bro = static_cast<bptree_node_middle*>(parent->_pointers[index + 1]);
            index = path.top().second;
            path.pop();
            if (index > 0 && middle_left_bro != nullptr) {
                size_left_bro = middle_left_bro->_keys.size();
            }
            if (index + 1 < parent->_pointers.size() && middle_right_bro != nullptr) {
                size_right_bro = middle_right_bro->_keys.size();
            }
            if (size_left_bro > 0) {
                merge(middle_left_bro, current, parent, index - 1);
            } else if (size_right_bro > 0) {
                merge(current, middle_right_bro, parent, index);
            } else {
                throw std::logic_error("error while merging");
            }
        }
        if (parent->_keys.size() != 0) {
            return;
        }
        if (parent->_pointers.size() <= 0) {
            _root = nullptr;
        } else {
            if (parent->_pointers[0] != nullptr) {
                _root = parent->_pointers[0];
            } else {
                if (parent->_pointers.size() > 1) {
                    _root = parent->_pointers[1];
                } else {
                    _root = nullptr;
                }
            }
        }
        _allocator.delete_object(parent);
    }
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::merge(BP_tree::bptree_node_base* left_node, BP_tree::bptree_node_base* right_node,
                                              bptree_node_middle* parent, size_t index) {
    if (left_node->_is_terminate){
        merge_term_nodes(static_cast<bptree_node_term*>(left_node), static_cast<bptree_node_term*>(right_node), parent, index);
    } else {
        merge_middle_nodes(static_cast<bptree_node_middle*>(left_node), static_cast<bptree_node_middle*>(right_node), parent, index);
    }
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void
BP_tree<tkey, tvalue, compare, t>::merge_term_nodes(BP_tree::bptree_node_term *left_node, BP_tree::bptree_node_term *right_node,
                                                   BP_tree::bptree_node_middle *parent, size_t index) {
    bptree_node_term* new_node = _allocator.template new_object<bptree_node_term>();
    new_node->_data = left_node->_data;
    new_node->_data.insert(new_node->_data.end(), right_node->_data.begin(), right_node->_data.end());
    parent->_keys.erase(parent->_keys.begin() + index);
    parent->_pointers.erase(parent->_pointers.begin() + index);
    if (parent->_pointers.size() > index) {
        parent->_pointers[index] = new_node;
    } else {
        parent->_pointers.push_back(new_node);
    }
    _allocator.delete_object(left_node);
    _allocator.delete_object(right_node);
}


template<typename tkey, typename tvalue, compator<tkey> compare, std::size_t t>
void BP_tree<tkey, tvalue, compare, t>::merge_middle_nodes(BP_tree::bptree_node_middle* left_node, BP_tree::bptree_node_middle* right_node,
                                                     bptree_node_middle* parent, size_t index) {
    bptree_node_middle* new_node = _allocator.template new_object<bptree_node_middle>();
    new_node->_keys = left_node->_keys;
    new_node->_pointers = left_node->_pointers;
    new_node->_keys.push_back(parent->_keys[index]);
    new_node->_pointers.push_back(nullptr);
    new_node->_keys.insert(new_node->_keys.end(), right_node->_keys.begin(), right_node->_keys.end());
    new_node->_pointers.insert(new_node->_pointers.end(), right_node->_pointers.begin(), right_node->_pointers.end());
    parent->_keys.erase(parent->_keys.begin() + index);
    parent->_pointers.erase(parent->_pointers.begin() + index);
    if (parent->_pointers.size() > index) {
        parent->_pointers[index] = new_node;
    } else {
        parent->_pointers.push_back(new_node);
    }
    _allocator.delete_object(left_node);
    _allocator.delete_object(right_node);
}

#endif