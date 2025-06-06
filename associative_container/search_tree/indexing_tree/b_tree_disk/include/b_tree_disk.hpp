#ifndef B_TREE_DISK_HPP
#define B_TREE_DISK_HPP

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stack>
#include <utility>
#include <vector>

template<typename compare, typename tkey>
concept compator = requires(const compare c, const tkey& lhs, const tkey& rhs) {
    { c(lhs, rhs) } -> std::same_as<bool>;
} && std::copyable<compare> && std::default_initializable<compare>;

template<typename f_iter, typename tkey, typename tval>
concept input_iterator_for_pair = std::input_iterator<f_iter> && std::same_as<typename std::iterator_traits<f_iter>::value_type, std::pair<tkey, tval>>;

template<typename T>
concept serializable = requires(const T t, std::fstream& s) {
    { t.serialize(s) };
    { T::deserialize(s) } -> std::same_as<T>;
    { t.serialize_size() } -> std::same_as<size_t>;
} && std::copyable<T>;


template<serializable tkey, serializable tvalue, compator<tkey> compare = std::less<tkey>, std::size_t t = 2>
class B_tree_disk final : private compare {
public:
    using tree_data_type = std::pair<tkey, tvalue>;
    using tree_data_type_const = std::pair<tkey, tvalue>;

private:
    static constexpr const size_t minimum_keys_in_node = t - 1;
    static constexpr const size_t maximum_keys_in_node = 2 * t - 1;

    // region comparators declaration

    inline bool compare_keys(const tkey& lhs, const tkey& rhs) const;
    inline bool compare_pairs(const tree_data_type& lhs, const tree_data_type& rhs) const;

    // endregion comparators declaration

public:
    struct btree_disk_node {
        size_t size;
        bool _is_leaf;
        size_t position_in_disk;
        std::vector<tree_data_type> keys;
        std::vector<size_t> pointers;

        void serialize(std::fstream& stream, std::fstream& stream_for_data) const;
        static btree_disk_node deserialize(std::fstream& stream, std::fstream& stream_for_data);

        explicit btree_disk_node(bool is_leaf);
        btree_disk_node();
    };

private:
    friend btree_disk_node;

    std::fstream _file_for_tree;
    std::fstream _file_for_key_value;

    size_t _size_of_block_node;

    size_t _position_root;

    btree_disk_node _current_node;

public:
    static size_t _count_of_node;

    // region constructors declaration
    explicit B_tree_disk(const std::string& file_path, const compare& cmp = compare(), void* logger = nullptr);
    // endregion constructors declaration

    // region five declaration
    B_tree_disk(B_tree_disk&& other) noexcept;
    B_tree_disk& operator=(B_tree_disk&& other) noexcept;

    B_tree_disk(const B_tree_disk& other) = delete;
    B_tree_disk& operator=(const B_tree_disk& other) = delete;

    ~B_tree_disk() noexcept;
    // endregion five declaration

    // region iterators declaration
    class btree_disk_const_iterator {
        std::stack<std::pair<size_t, size_t>> _path;
        size_t _index;
        B_tree_disk<tkey, tvalue, compare, t>& _tree;

    public:
        using value_type = tree_data_type_const;
        using reference = value_type;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;

        using self = btree_disk_const_iterator;

        friend class B_tree_disk;

        value_type operator*() const noexcept;

        self& operator++();
        self operator++(int);

        self& operator--();
        self operator--(int);

        bool operator==(const self& other) const noexcept;
        bool operator!=(const self& other) const noexcept;

        explicit btree_disk_const_iterator(B_tree_disk<tkey, tvalue, compare, t>& tree, const std::stack<std::pair<size_t, size_t>>& path = std::stack<std::pair<size_t, size_t>>(), size_t index = 0);
    };

    friend class btree_disk_const_iterator;

    std::optional<tvalue> at(const tkey&);

    btree_disk_const_iterator begin();
    btree_disk_const_iterator end();

    std::pair<btree_disk_const_iterator, btree_disk_const_iterator> find_range(const tkey& lower, const tkey& upper, bool include_lower = true, bool include_upper = false);

    bool insert(const tree_data_type& data);

    bool update(const tree_data_type& data);

    bool erase(const tkey& key);

    bool is_valid() const noexcept;

    template<typename KeyArg, typename ValueArg>
    bool emplace(KeyArg&& key_arg, ValueArg&& value_arg) {
        return insert({tkey(std::forward<KeyArg>(key_arg)),
                       tvalue(std::forward<ValueArg>(value_arg))});
    }

    std::pair<std::stack<std::pair<size_t, size_t>>, std::pair<size_t, bool>> find_path(const tkey& key);

public:
    btree_disk_node disk_read(size_t position);
    void check_tree(size_t pos, size_t depth);
    void disk_write(btree_disk_node& node);

private:
    std::pair<size_t, bool> find_index(const tkey& key, btree_disk_node& node) const noexcept;

    void insert_array(btree_disk_node& node, size_t right_node, const tree_data_type& data, size_t index) noexcept;

    void split_node(std::stack<std::pair<size_t, size_t>>& path);

    btree_disk_node remove_array(btree_disk_node& node, size_t index, bool remove_left_ptr = true) noexcept;

    void rebalance_node(std::stack<std::pair<size_t, size_t>>& path, btree_disk_node& node, size_t& index);

    void update_meta();
};


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::is_valid() const noexcept {
    try {
        if (_count_of_node == 0 && _position_root == 0) {
            return true;
        }
        check_tree(_position_root, 0);
        return true;
    } catch (...) {
        return false;
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node::deserialize(std::fstream& stream, std::fstream& stream_for_data) {

    if (!stream.good()) {
        throw std::runtime_error("disruptions in the stream have been detected");
    }
    if (!stream_for_data.good()) {
        throw std::runtime_error("disruptions in the stream for data have been detected");
    }

    btree_disk_node current_node;

    if (!stream.read(reinterpret_cast<char*>(&current_node.size), sizeof(current_node.size))) {
        throw std::runtime_error("error while reading size of node from file");
    }
    if (!stream.read(reinterpret_cast<char*>(&current_node._is_leaf), sizeof(current_node._is_leaf))) {
        throw std::runtime_error("error while reading leaf-flag of node from file");
    }

    if (!stream.read(reinterpret_cast<char*>(&current_node.position_in_disk), sizeof(current_node.position_in_disk))) {
        throw std::runtime_error("error while reading position in disk of node from file");
    }


    size_t amount_of_keys;
    if (!stream.read(reinterpret_cast<char*>(&amount_of_keys), sizeof(amount_of_keys))) {
        throw std::runtime_error("error while reading amount of keys of node from file");
    }

    size_t shift_of_node;

    for (size_t i = 0; i < amount_of_keys; ++i) {

        if (!stream.read(reinterpret_cast<char*>(&shift_of_node), sizeof(shift_of_node))) {
            throw std::runtime_error("error while reading shift of node from file");
        }

        stream_for_data.seekg(0, std::ios::end);

        size_t size_of_file = static_cast<size_t>(stream_for_data.tellg());

        if (size_of_file < shift_of_node) {
            throw std::runtime_error("incorrect shift of node (shift > size of file)");
        }

        stream_for_data.seekg(shift_of_node, std::ios::beg);

        if (!stream_for_data.good()) {
            throw std::runtime_error("disruptions in the stream for data have been detected");
        }

        tkey new_key = tkey::deserialize(stream_for_data);
        tvalue new_value = tvalue::deserialize(stream_for_data);

        current_node.keys.emplace_back(std::move(new_key), std::move(new_value));
    }

    size_t amount_of_pointers;

    if (!stream.read(reinterpret_cast<char*>(&amount_of_pointers), sizeof(amount_of_pointers))) {
        if (current_node._is_leaf) {
            return current_node;
        }
        throw std::runtime_error("error while reading amount of pointers");
    }

    size_t new_ptr;

    for (size_t i = 0; i < amount_of_pointers; ++i) {
        if (!stream.read(reinterpret_cast<char*>(&new_ptr), sizeof(new_ptr))) {
            throw std::runtime_error("error while reading pointers of node from file");
        }
        current_node.pointers.push_back(new_ptr);
    }

    return current_node;
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
std::pair<size_t, bool> B_tree_disk<tkey, tvalue, compare, t>::find_index(const tkey& key, btree_disk_node& node) const noexcept {

    if (node.keys.empty()) {
        return {0, false};
    }

    for (size_t i = 0; i < node.size && i < node.keys.size(); ++i) {

        const auto& current_key = node.keys[i].first;

        if (!compare_keys(key, current_key) && !compare_keys(current_key, key)) {
            return {i, true};
        }

        if (compare_keys(key, current_key)) {
            return {i, false};
        }
    }

    return {node.size, false};
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
std::optional<tvalue> B_tree_disk<tkey, tvalue, compare, t>::at(const tkey& key) {

    if (!_file_for_tree.is_open()) {
        throw std::runtime_error("file for tree isn't open");
    }
    if (!_file_for_key_value.is_open()) {
        throw std::runtime_error("file for key value isn't open");
    }

    auto [path, info] = find_path(key);
    auto [index, exist] = info;

    if (!exist || path.empty()) {
        return std::nullopt;
    }

    auto [position, top_index] = path.top();
    auto node = disk_read(position);

    if (top_index < node.keys.size()) {
        return node.keys[top_index].second;
    }

    return std::nullopt;
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::insert(const B_tree_disk::tree_data_type& data) {

    try {

        if (!(_file_for_tree.good() && _file_for_key_value.good())) {
            return false;
        }

        auto [path, info] = find_path(data.first);

        if (info.second) {
            return false;
        }

        if (path.empty() || _position_root == 0) {

            btree_disk_node root(true);

            root.keys.push_back(data);
            root.size = 1;
            _count_of_node++;
            root.position_in_disk = _count_of_node;
            _position_root = root.position_in_disk;

            disk_write(root);
            update_meta();

            return true;

        }

        auto [position, top_index] = path.top();
        auto current_node = disk_read(position);

        insert_array(current_node, 0, data, top_index);
        disk_write(current_node);

        if (current_node.size > maximum_keys_in_node) {
            auto copy_path = path;
            split_node(copy_path);
        }

        update_meta();
        return true;

    } catch (...) {
        return false;
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::update_meta() {

    if (!_file_for_tree.is_open()) {
        throw std::runtime_error("file for tree isn't open");
    }

    std::streampos save_position = _file_for_tree.tellp();
    _file_for_tree.seekp(0, std::ios::beg);

    if (!_file_for_tree.good()) {
        _file_for_tree.clear();
        throw std::runtime_error("file for tree isn't good");
    }

    _file_for_tree.write(reinterpret_cast<const char*>(&_count_of_node), sizeof(_count_of_node));
    _file_for_tree.write(reinterpret_cast<const char*>(&_position_root), sizeof(_position_root));
    _file_for_tree.write(reinterpret_cast<const char*>(&_size_of_block_node), sizeof(_size_of_block_node));

    if (!_file_for_tree.good()) {
        _file_for_tree.clear();
        throw std::runtime_error("file for tree isn't good");
    }

    _file_for_tree.flush();

    if (save_position >= 0) {
        _file_for_tree.seekp(save_position);
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
B_tree_disk<tkey, tvalue, compare, t>::B_tree_disk(const std::string& file_path,
                                                   const compare& cmp,
                                                   void* logger) : compare(cmp), _size_of_block_node(1024) {

    std::filesystem::path original_file(file_path);

    auto tree_file = original_file;
    tree_file += ".tree";

    auto data_file = original_file;
    data_file += ".data";

    bool already_exist = std::filesystem::exists(tree_file) && std::filesystem::exists(data_file);

    if (already_exist) {

        _file_for_tree.open(tree_file, std::ios::in | std::ios::out | std::ios::binary);
        _file_for_key_value.open(data_file, std::ios::in | std::ios::out | std::ios::binary);

        if (_file_for_tree.good() && _file_for_key_value.good()) {

            _file_for_tree.seekg(0, std::ios::beg);

            if (!_file_for_tree.read(reinterpret_cast<char*>(&_count_of_node), sizeof(_count_of_node))) {
                if (_file_for_tree.is_open()) {
                    _file_for_tree.close();
                }
                if (_file_for_key_value.is_open()) {
                    _file_for_key_value.close();
                }
                throw std::runtime_error("error while reading count of node from tree file");
            }

            if (!_file_for_tree.read(reinterpret_cast<char*>(&_position_root), sizeof(_position_root))) {
                if (_file_for_tree.is_open()) {
                    _file_for_tree.close();
                }
                if (_file_for_key_value.is_open()) {
                    _file_for_key_value.close();
                }
                throw std::runtime_error("error while reading root position from tree file");
            }

            if (_file_for_tree.read(reinterpret_cast<char*>(&_size_of_block_node), sizeof(_size_of_block_node))) {
                if (_count_of_node > 0 && _position_root > 0 && _size_of_block_node >= 1024) {
                    return;
                }
            }
        }

        if (_file_for_tree.is_open()) {
            _file_for_tree.close();
        }
        if (_file_for_key_value.is_open()) {
            _file_for_key_value.close();
        }
    }

    _file_for_tree.open(tree_file, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    _file_for_key_value.open(data_file, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

    if (!_file_for_tree.good() || !_file_for_key_value.good()) {
        if (_file_for_tree.is_open()) {
            _file_for_tree.close();
        }
        if (_file_for_key_value.is_open()) {
            _file_for_key_value.close();
        }
        throw std::runtime_error("file for tree and file for key aren't good");
    }

    _position_root = 0;
    _count_of_node = 0;

    btree_disk_node new_root(true);
    new_root.position_in_disk = _count_of_node + 1;
    _position_root = new_root.position_in_disk;

    update_meta();
    disk_write(new_root);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::disk_write(btree_disk_node& node) {

    if (!_file_for_tree.is_open()) {
        throw std::runtime_error("file for tree isn't open");
    }

    if (!_file_for_key_value.is_open()) {
        throw std::runtime_error("file for key value isn't open");
    }

    if (node.position_in_disk == 0) {
        _count_of_node++;
        node.position_in_disk = _count_of_node;
    }

    if (node.keys.size() != node.size) {
        node.size = node.keys.size();
    }

    size_t size_of_node = sizeof(size_t) * 3 +
                          (node.keys.size() * sizeof(size_t) * 2) +
                          (node.pointers.size() * sizeof(size_t));

    if (size_of_node > _size_of_block_node) {
        _size_of_block_node = size_of_node;
    }

    size_t position_in_file = sizeof(size_t) * 3;
    position_in_file += (node.position_in_disk - 1) * _size_of_block_node;

    _file_for_tree.seekp(position_in_file, std::ios::beg);

    if (!_file_for_tree.good()) {
        _file_for_tree.clear();
        throw std::runtime_error("error while seeking for node's position in file for tree");
    }

    try {
        node.serialize(_file_for_tree, _file_for_key_value);

        if (!_file_for_tree.good() || !_file_for_key_value.good()) {
            _file_for_tree.clear();
            _file_for_key_value.clear();
            throw std::runtime_error("error while serializing of node");
        }

        _file_for_tree.flush();
        _file_for_key_value.flush();

    } catch (const std::exception& ex) {
        _file_for_tree.clear();
        _file_for_key_value.clear();
        throw ex;
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator
B_tree_disk<tkey, tvalue, compare, t>::begin() {

    std::stack<std::pair<size_t, size_t>> path;

    if (_position_root == 0 || _count_of_node == 0) {
        return end();
    }

    size_t current = _position_root;

    while (true) {

        auto new_node = disk_read(current);
        path.push({new_node.position_in_disk, 0});

        if (new_node._is_leaf || new_node.pointers.empty()) {
            break;
        }

        current = new_node.pointers[0];
    }

    if (!path.empty()) {

        auto [position, index] = path.top();
        auto new_node = disk_read(position);

        if (new_node.size == 0) {
            return end();
        }
    }

    return btree_disk_const_iterator(*this, path);
}



template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::erase(const tkey& key) {
    try {
        auto [path, path_info] = find_path(key);
        size_t index = path_info.first;

        bool exist = path_info.second;
        if (!exist) {
            return false;
        }

        auto [top_position, top_index] = path.top();
        auto current = disk_read(top_position);

        if (current._is_leaf) {

            auto new_node = remove_array(current, index, false);
            disk_write(new_node);

        } else {
            size_t child_position = current.pointers[index];
            auto previous = disk_read(child_position);

            while (!previous._is_leaf) {
                child_position = previous.pointers[previous.size];
                previous = disk_read(child_position);
            }

            auto previous_pair = previous.keys[previous.size - 1];
            current.keys[index] = previous_pair;
            disk_write(current);

            previous = remove_array(previous, previous.size - 1, false);

            disk_write(previous);

            path.push({child_position, previous.size});
        }

        while (!path.empty()) {

            auto [temp_pos, temp_index] = path.top();
            path.pop();

            auto current_node = disk_read(temp_pos);

            rebalance_node(path, current_node, temp_index);

            disk_write(current_node);
        }

        auto root = disk_read(_position_root);

        if (!root._is_leaf && root.size == 0) {
            _position_root = root.pointers[0];
        }

        disk_write(root);
        update_meta();

        return true;

    } catch (...) {

        return false;
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::rebalance_node(std::stack<std::pair<size_t, size_t>>& path, btree_disk_node& node, size_t& index) {

    if (node._is_leaf && node.size >= minimum_keys_in_node) {
        return;
    }
    if (!node._is_leaf && node.size >= minimum_keys_in_node) {
        return;
    }
    if (path.empty()) {
        return;
    }

    auto [parent_pos, parent_index] = path.top();
    auto parent = disk_read(parent_pos);

    if (parent_index > 0) {

        size_t left_position = parent.pointers[parent_index - 1];
        auto left_brother = disk_read(left_position);

        if (left_brother.size > minimum_keys_in_node) {

            node.keys.insert(node.keys.begin(), parent.keys[parent_index - 1]);

            if (!node._is_leaf) {
                node.pointers.insert(node.pointers.begin(), left_brother.pointers.back());
            }

            parent.keys[parent_index - 1] = left_brother.keys.back();
            left_brother.keys.pop_back();

            if (!left_brother._is_leaf) {
                left_brother.pointers.pop_back();
            }

            --left_brother.size;
            ++node.size;

            disk_write(left_brother);
            disk_write(parent);
            return;
        }
    }

    if (parent_index < parent.pointers.size() - 1) {

        size_t right_position = parent.pointers[parent_index + 1];
        auto right_brother = disk_read(right_position);

        if (right_brother.size > minimum_keys_in_node) {

            node.keys.push_back(parent.keys[parent_index]);

            if (!node._is_leaf) {
                node.pointers.push_back(right_brother.pointers.front());
            }

            parent.keys[parent_index] = right_brother.keys.front();
            right_brother.keys.erase(right_brother.keys.begin());

            if (!right_brother._is_leaf) {
                right_brother.pointers.erase(right_brother.pointers.begin());
            }

            --right_brother.size;
            ++node.size;

            disk_write(right_brother);
            disk_write(parent);
            return;
        }
    }
    if (parent_index > 0) {
        size_t left_position = parent.pointers[parent_index - 1];
        auto left = disk_read(left_position);

        left.keys.push_back(parent.keys[parent_index - 1]);

        for (auto& key: node.keys) {
            left.keys.push_back(key);
        }
        if (!node._is_leaf) {
            for (auto& ptr: node.pointers) {
                left.pointers.push_back(ptr);
            }
        }

        left.size = left.keys.size();
        disk_write(left);

        parent.keys.erase(parent.keys.begin() + parent_index - 1);
        parent.pointers.erase(parent.pointers.begin() + parent_index);

    } else {
        size_t right_position = parent.pointers[parent_index + 1];
        auto right = disk_read(right_position);

        node.keys.push_back(parent.keys[parent_index]);

        for (auto& k: right.keys) {
            node.keys.push_back(k);
        }
        if (!node._is_leaf) {
            for (auto& ptr: right.pointers) {
                node.pointers.push_back(ptr);
            }
        }

        node.size = node.keys.size();
        disk_write(node);

        parent.keys.erase(parent.keys.begin() + parent_index);
        parent.pointers.erase(parent.pointers.begin() + parent_index + 1);
    }

    --parent.size;
    disk_write(parent);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node B_tree_disk<tkey, tvalue, compare, t>::remove_array(btree_disk_node& node, size_t index, bool remove_left_ptr) noexcept {

    if (index < node.keys.size()) {
        node.keys.erase(node.keys.begin() + index);
    }

    if (remove_left_ptr && index < node.pointers.size()) {
        node.pointers.erase(node.pointers.begin() + index);
    } else if (!remove_left_ptr && index + 1 < node.pointers.size()) {
        node.pointers.erase(node.pointers.begin() + index + 1);
    }

    if (node.size > 0) {
        --node.size;
    }

    return node;
}



template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::update(const B_tree_disk::tree_data_type& data) {

    auto [path, found_info] = find_path(data.first);
    size_t index = found_info.first;
    bool exist = found_info.second;

    if (!exist) {
        return false;
    }

    auto [top_position, top_index] = path.top();
    auto node = disk_read(top_position);

    if (index < node.keys.size()) {
        node.keys[index].second = data.second;
        disk_write(node);
        update_meta();
        return true;
    }

    return false;
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::split_node(std::stack<std::pair<size_t, size_t>>& path) {

    if (path.empty()) {
        throw std::runtime_error("empty path in split node");
    }

    auto [top_pos, top_index] = path.top();
    path.pop();

    auto current = disk_read(top_pos);

    if (current.keys.size() < maximum_keys_in_node) {
        throw std::runtime_error("doesn't need split this node");
    }

    btree_disk_node new_node(current._is_leaf);
    new_node.size = minimum_keys_in_node;
    auto middle_key = current.keys[minimum_keys_in_node];

    new_node.keys.clear();

    for (size_t i = t; i < current.keys.size(); ++i) {
        new_node.keys.push_back(current.keys[i]);
    }

    if (!current._is_leaf) {

        new_node.pointers.clear();

        for (size_t i = t; i <= current.keys.size() && i < current.pointers.size(); ++i) {
            new_node.pointers.push_back(current.pointers[i]);
        }
    }

    current.keys.resize(minimum_keys_in_node);
    if (!current._is_leaf) {
        current.pointers.resize(t);
    }

    current.size = minimum_keys_in_node;
    _count_of_node++;
    new_node.position_in_disk = _count_of_node;

    disk_write(current);
    disk_write(new_node);

    if (path.empty()) {
        btree_disk_node new_root(false);

        new_root.size = 1;
        new_root.keys.push_back(middle_key);

        new_root.pointers.push_back(current.position_in_disk);
        new_root.pointers.push_back(new_node.position_in_disk);

        _count_of_node++;
        new_root.position_in_disk = _count_of_node;
        _position_root = new_root.position_in_disk;

        disk_write(new_root);
        update_meta();
        return;
    }
    auto [parent_position, parent_index] = path.top();
    auto parent = disk_read(parent_position);

    if (parent_index > parent.keys.size()) {
        parent_index = parent.keys.size();
    }

    insert_array(parent, new_node.position_in_disk, middle_key, parent_index);

    disk_write(parent);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::insert_array(btree_disk_node& node, size_t right_node, const tree_data_type& data, size_t index) noexcept {

    if (index > node.keys.size()) {
        index = node.keys.size();
    }

    if (index == node.keys.size()) {
        node.keys.push_back(data);
    } else {
        node.keys.insert(node.keys.begin() + index, data);
    }

    if (!node._is_leaf && right_node != 0) {
        if (index + 1 < node.pointers.size()) {
            node.pointers.insert(node.pointers.begin() + index + 1, right_node);
        } else {
            node.pointers.push_back(right_node);
        }
    }

    node.size = node.keys.size();
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
std::pair<std::stack<std::pair<size_t, size_t>>, std::pair<size_t, bool>>
B_tree_disk<tkey, tvalue, compare, t>::find_path(const tkey& key) {

    std::stack<std::pair<size_t, size_t>> path;
    size_t position = _position_root;

    if (position == 0 || _count_of_node == 0) {
        return {path, {0, false}};
    }

    while (true) {
        auto current = disk_read(position);

        if (current.position_in_disk == 0) {
            throw std::runtime_error("incorrect position of node in disk");
        }

        auto [index, exist] = find_index(key, current);
        if (index > current.size) {
            index = current.size;
        }

        path.push({position, index});

        if (exist) {
            return {path, {index, true}};
        }

        if (current._is_leaf) {
            return {path, {index, false}};
        }

        if (current.pointers.empty()) {
            throw std::runtime_error("node isn't valid");
        }

        if (index >= current.pointers.size()) {
            index = current.pointers.size() - 1;
        }

        position = current.pointers[index];

        if (position == 0) {
            throw std::runtime_error("node isn't valid");
        }
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node::serialize(std::fstream& stream, std::fstream& stream_for_data) const {

    size_t keys_size = keys.size();

    stream.write(reinterpret_cast<const char*>(&keys_size), sizeof(keys_size));

    stream.write(reinterpret_cast<const char*>(&_is_leaf), sizeof(_is_leaf));

    stream.write(reinterpret_cast<const char*>(&position_in_disk), sizeof(position_in_disk));

    size_t amount_keys = keys_size;
    stream.write(reinterpret_cast<const char*>(&amount_keys), sizeof(amount_keys));

    for (size_t i = 0; i < amount_keys && i < keys.size(); ++i) {

        const auto& current_key = keys[i];
        stream_for_data.seekp(0, std::ios::end);
        size_t shift_of_node = static_cast<size_t>(stream_for_data.tellp());

        if (!stream_for_data.good()) {
            throw std::runtime_error("stream for data isn't good");
        }

        current_key.first.serialize(stream_for_data);
        current_key.second.serialize(stream_for_data);

        if (!stream_for_data.good()) {
            throw std::runtime_error("stream for data isn't good");
        }

        stream.write(reinterpret_cast<const char*>(&shift_of_node), sizeof(shift_of_node));
    }

    size_t amount_pointers = pointers.size();
    stream.write(reinterpret_cast<const char*>(&amount_pointers), sizeof(amount_pointers));

    for (size_t i = 0; i < amount_pointers && i < pointers.size(); ++i) {
        size_t current_ptr = pointers[i];
        stream.write(reinterpret_cast<const char*>(&current_ptr), sizeof(current_ptr));
    }

    if (!stream.good()) {
        throw std::runtime_error("stream isn't good");
    }

    if (!stream_for_data.good()) {
        throw std::runtime_error("stream for data isn't good");
    }

}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node
B_tree_disk<tkey, tvalue, compare, t>::disk_read(size_t node_position) {

    if (node_position == 0) {
        return btree_disk_node();
    }

    if (node_position > _count_of_node) {
        throw std::runtime_error("incorrect node position");
    }

    if (!_file_for_tree.is_open()) {
        throw std::runtime_error("file for tree isn't open");
    }

    if (!_file_for_key_value.is_open()) {
        throw std::runtime_error("file for key value isn't open");
    }

    size_t pos_in_file = sizeof(size_t) * 3;
    pos_in_file += (node_position - 1) * _size_of_block_node;

    _file_for_tree.seekg(0, std::ios::end);

    size_t file_size = static_cast<size_t>(_file_for_tree.tellg());

    if (pos_in_file >= file_size) {
        throw std::runtime_error("incorrect position in file for node");
    }

    _file_for_tree.seekg(pos_in_file, std::ios::beg);

    if (!_file_for_tree.good()) {
        _file_for_tree.clear();
        throw std::runtime_error("file for tree isn't good");
    }

    try {

        auto current = btree_disk_node::deserialize(_file_for_tree, _file_for_key_value);
        return current;

    } catch (const std::exception& ex) {

        _file_for_tree.clear();
        throw ex;

    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node::btree_disk_node(bool is_leaf)
: size(0), _is_leaf(is_leaf), position_in_disk(0)
{
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_node::btree_disk_node()
: size(0), _is_leaf(true), position_in_disk(0)
{
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::compare_pairs(const B_tree_disk::tree_data_type& lhs,
                                                          const B_tree_disk::tree_data_type& rhs) const {
    return compare{}(lhs.first, rhs.first);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::compare_keys(const tkey& lhs, const tkey& rhs) const {
    return compare{}(lhs, rhs);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
size_t B_tree_disk<tkey, tvalue, compare, t>::_count_of_node = 0;


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
B_tree_disk<tkey, tvalue, compare, t>::~B_tree_disk() noexcept {
    try {

        if (_file_for_tree.is_open()) {
            update_meta();
            _file_for_tree.close();
        }

        if (_file_for_key_value.is_open()) {
            _file_for_key_value.close();
        }

    } catch (...) {
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
void B_tree_disk<tkey, tvalue, compare, t>::check_tree(size_t pos, size_t depth) {

    if (pos == 0) {
        return;
    }

    auto current = disk_read(pos);

    size_t min_amount_keys = (pos == _position_root ? 1 : minimum_keys_in_node);
    size_t max_amount_keys = maximum_keys_in_node;

    if (!(current.size >= min_amount_keys && current.size <= max_amount_keys)) {
        throw std::runtime_error("incorrect amount of keys");
    }

    for (size_t i = 1; i < current.size && i < current.keys.size(); ++i) {

        if (!compare_keys(current.keys[i - 1].first, current.keys[i].first)) {
            throw std::runtime_error("incorrect comparing between keys");
        }

    }

    if (!current._is_leaf) {

        if (current.pointers.size() == current.size + 1) {
            throw std::runtime_error("incorrect number of pointers of node");
        }

        for (size_t i = 0; i < current.pointers.size(); ++i) {
            check_tree(current.pointers[i], depth + 1);
        }

    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::btree_disk_const_iterator(
        B_tree_disk<tkey, tvalue, compare, t>& tree,
        const std::stack<std::pair<size_t, size_t>>& path,
        size_t index): _tree(tree), _path(path), _index(index) {}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator
B_tree_disk<tkey, tvalue, compare, t>::end() {

    return btree_disk_const_iterator(*this);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::value_type
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator*() const noexcept {

    if (_path.empty()) {
        return tree_data_type_const();
    }

    try {
        auto [position, index] = _path.top();
        auto current = _tree.disk_read(position);

        if (index < current.keys.size()) {
            return current.keys[index];
        }
        return tree_data_type_const();

    } catch (...) {
        return tree_data_type_const();
    }
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::self&
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator++() {

    if (_path.empty()) {
        return *this;
    }

    auto [top_position, top_index] = _path.top();
    auto current = _tree.disk_read(top_position);

    if (!current._is_leaf) {

        if (top_index + 1 < current.pointers.size()) {

            size_t next_pos = current.pointers[top_index + 1];
            _path.top().second++;

            while (true) {

                auto temp_node = _tree.disk_read(next_pos);

                _path.emplace(next_pos, 0);

                if (temp_node._is_leaf || temp_node.pointers.empty()) {
                    break;
                }
                next_pos = temp_node.pointers[0];
            }

            return *this;
        }
    }

    if (top_index + 1 < current.size) {

        _path.top().second++;

        return *this;
    }

    std::stack<std::pair<size_t, size_t>> new_path;
    size_t child_pos = top_position;
    _path.pop();
    bool found_parent = false;

    while (!_path.empty()) {

        auto [parent_pos, parent_index] = _path.top();
        _path.pop();

        auto parent = _tree.disk_read(parent_pos);
        int child_index = -1;

        for (size_t i = 0; i < parent.pointers.size(); i++) {

            if (parent.pointers[i] == child_pos) {
                child_index = i;
                break;
            }

        }

        if (child_index == -1) {
            _path = std::stack<std::pair<size_t, size_t>>();
            return *this;
        }

        if (child_index < parent.size) {

            new_path.emplace(parent_pos, child_index);
            found_parent = true;
            break;

        }

        child_pos = parent_pos;
    }

    if (!found_parent) {
        return *this;
    }

    while (!new_path.empty()) {

        _path.push(new_path.top());
        new_path.pop();

    }
    return *this;

}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator++(int) {

    self temp = *this;
    ++(*this);
    return temp;

}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::self&
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator--() {

    if (_path.empty()) {
        return *this;
    }

    auto [position, index] = _path.top();
    auto current = _tree.disk_read(position);

    if (index > 0) {
        _path.top().second = index - 1;

        if (!current._is_leaf) {

            size_t pos = current.pointers[index];

            while (true) {

                auto next_node = _tree.disk_read(pos);
                size_t i = next_node.size - 1;

                _path.emplace(pos, i);

                if (next_node._is_leaf) {
                    break;
                }

                pos = next_node.pointers[i + 1];
            }
        }

    } else {

        _path.pop();

        while (!_path.empty()) {

            auto& [prev_pos, prev_index] = _path.top();

            if (prev_index > 0) {
                break;
            }

            _path.pop();
        }

        if (_path.empty()) {
            return *this;
        }

        auto& [prev_pos, prev_index] = _path.top();

        _path.top().second = prev_index - 1;

    }
    return *this;
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator
B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator--(int) {

    self temp = *this;
    --(*this);
    return temp;

}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator==(const self& other) const noexcept {

    if (_path.empty() && other._path.empty()) {
        return true;
    }
    if (_path.empty() || other._path.empty()) {
        return false;
    }

    auto this_copy = _path;
    auto other_copy = other._path;

    while (!this_copy.empty()) {

        if (this_copy.top() != other_copy.top()) {
            return false;
        }

        this_copy.pop();
        other_copy.pop();
    }

    return true;
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
bool B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator::operator!=(const self& other) const noexcept {
    return !(*this == other);
}


template<serializable tkey, serializable tvalue, compator<tkey> compare, std::size_t t>
std::pair<typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator,
        typename B_tree_disk<tkey, tvalue, compare, t>::btree_disk_const_iterator>
B_tree_disk<tkey, tvalue, compare, t>::find_range(const tkey& lower, const tkey& upper, bool include_lower, bool include_upper) {

    try {
        auto [lower_path, lower_info] = find_path(lower);
        auto [lower_index, lower_exist] = lower_info;
        auto lower_iter = btree_disk_const_iterator(*this, lower_path, lower_index);
        if (!include_lower && lower_exist) {
            ++lower_iter;
        }

        auto [upper_path, upper_info] = find_path(upper);
        auto [upper_index, upper_exist] = upper_info;
        auto upper_iter = btree_disk_const_iterator(*this, upper_path, upper_index);


        if (!include_upper && upper_exist) {

            btree_disk_const_iterator end_iter(*this);

            return std::make_pair(lower_iter, end_iter);

        } else if (upper_exist) {
            ++upper_iter;
        }

        return std::make_pair(lower_iter, upper_iter);

    } catch (...) {

        btree_disk_const_iterator end_iter(*this);

        return std::make_pair(end_iter, end_iter);
    }
}

#endif//B_TREE_DISK_HPP