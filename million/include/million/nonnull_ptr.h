#pragma once

#include <cassert>
#include <stdexcept>
#include <utility>

namespace million {

template <typename T>
class nonnull_ptr {
public:
    // Constructors
    explicit nonnull_ptr(T* ptr) {
        if (!ptr) {
            throw std::invalid_argument("nonnull_ptr cannot be initialized with a null pointer.");
        }
        ptr_ = ptr;
    }

    nonnull_ptr(std::nullptr_t) = delete;

    // Delete default constructor
    nonnull_ptr() = delete;

    // Copy constructor
    nonnull_ptr(const nonnull_ptr& other) : ptr_(other.ptr_) {}

    // Move constructor
    nonnull_ptr(nonnull_ptr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr; // No longer owns the pointer
    }

    // Copy assignment
    nonnull_ptr& operator=(const nonnull_ptr& other) {
        if (this != &other) {
            assert(other.ptr_ != nullptr);
            ptr_ = other.ptr_;
        }
        return *this;
    }

    // Move assignment
    nonnull_ptr& operator=(nonnull_ptr&& other) noexcept {
        if (this != &other) {
            ptr_ = other.ptr_;
            other.ptr_ = nullptr; // No longer owns the pointer
        }
        return *this;
    }

    // Access operators
    T& operator*() const {
        assert(ptr_ != nullptr);
        return *ptr_;
    }

    T* operator->() const {
        assert(ptr_ != nullptr);
        return ptr_;
    }

    // Get the raw pointer (non-null guaranteed)
    T* get() const {
        assert(ptr_ != nullptr);
        return ptr_;
    }

    // Reset with a new non-null pointer
    void reset(T* ptr) {
        if (!ptr) {
            throw std::invalid_argument("nonnull_ptr cannot be reset with a null pointer.");
        }
        ptr_ = ptr;
    }

    // Disable reset to null
    void reset(std::nullptr_t) = delete;

private:
    T* ptr_;
};

// Helper function to create nonnull_ptr
template <typename T>
nonnull_ptr<T> make_nonnull(T* ptr) {
    return nonnull_ptr<T>(ptr);
}

} // namespace million