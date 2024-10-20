#pragma once

#include <cassert>
#include <utility>

namespace million {

template<typename T>
struct list_node {
    list_node<T>* prev;
    list_node<T>* next;
    T data;
  
    list_node() {}
    list_node(const T& value) : data(value) {}
    list_node(T&& value) : data(std::move(value)) {}
};

template<typename T>
class list;

template<typename T>
class list_iter {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    friend class list<T>;

    list_iter() : node_ptr_(nullptr) {}
    list_iter(list_node<T>* node_ptr) : node_ptr_(node_ptr) {}
    ~list_iter() = default;

    reference operator*() const {
        assert(node_ptr_ != nullptr);
        return node_ptr_->data;
    }

    pointer operator->() const {
        assert(node_ptr_ != nullptr);
        return &node_ptr_->data;
    }

    list_iter& operator++() {
        if (node_ptr_ != nullptr) {
            node_ptr_ = node_ptr_->next;
        }
        return *this;
    }

    list_iter operator++(int) const {
        list_iter temp = *this;
        ++(*this);
        return temp;
    }

    list_iter& operator--() {
        assert(node_ptr_ != nullptr); // Ensure not at the beginning
        node_ptr_ = node_ptr_->prev;
        return *this;
    }

    list_iter operator--(int) const {
        list_iter temp = *this;
        --(*this);
        return temp;
    }

    bool operator!=(const list_iter& other) const {
        return node_ptr_ != other.node_ptr_;
    }

    bool operator==(const list_iter& other) const {
        return node_ptr_ == other.node_ptr_;
    }

    bool is_valid() const {
        return node_ptr_ != nullptr;
    }

private:
    list_node<T>* node_ptr_;
};

template<typename T>
class list {
public:
    using iterator = list_iter<T>;

public:
    list() : size_(0)
    {
        node_ = new list_node<T>();
        node_->prev = node_;
        node_->next = node_;
    }

    ~list() {
        clear();
        delete node_;
    }

    void push_back(const T& value) {
        auto* new_node = new list_node<T>(value);
        push_back_node(new_node);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        auto* new_node = new list_node<T>(T(std::forward<Args>(args)...));
        push_back_node(new_node);
    }

    void pop_back() {
        assert(node_->next != node_);
        erase(end() - 1);
    }

    void erase(iterator it) {
        assert(it.is_valid());
        list_node<T>* del_node = it.node_ptr_;
        
        del_node->prev->next = del_node->next;
        del_node->next->prev = del_node->prev;

        delete del_node;
        size_--;
    }

    void clear() {
        auto node = node_->next;
        while (node != node_) {
            auto tmp = node->next;
            delete node;
            node = tmp;
        }
        node_->next = node_;
        node_->prev = node_;
        size_ = 0;
    }

    size_t size() const {
        return size_;
    }

    iterator begin() const {
        return iterator(node_->next);
    }

    iterator end() const {
        return iterator(node_);
    }

    iterator get_iter_from_data(const T* data) {
        std::uintptr_t offset = reinterpret_cast<std::uintptr_t>(&static_cast<list_node<T>*>(nullptr)->data);
        return iterator(reinterpret_cast<list_node<T>*>(reinterpret_cast<std::uintptr_t>(data) - offset));
    }

private:
    void push_back_node(list_node<T>* new_node) {
        new_node->next = node_;
        new_node->prev = node_->prev;
        node_->prev->next = new_node;
        node_->prev = new_node;
        size_++;
    }

private:
    list_node<T>* node_;
    size_t size_;
};

} // namespace million