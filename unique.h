#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t

struct Slug {};

template <typename T>
class DefaultDeleter {
public:
    DefaultDeleter() = default;

    void operator()(T* ptr) const noexcept {
        delete ptr;
    }

    template <typename S>
    DefaultDeleter(DefaultDeleter<S>&& other) {
    }
};

template <typename T>
class DefaultDeleter<T[]> {
public:
    void operator()(T* ptr) const noexcept {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, const Deleter& deleter) : data_(ptr, deleter) {
    }

    UniquePtr(T* ptr, Deleter&& deleter) : data_(ptr, std::move(deleter)) {
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <typename Son, typename SonDeleter>
    UniquePtr(UniquePtr<Son, SonDeleter>&& other) noexcept
            : data_(std::move(other.data_.GetFirst()), std::move(other.data_.GetSecond())) {
        other.data_.GetFirst() = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = other.data_.GetFirst();
        data_.GetSecond() = std::move(other.data_.GetSecond());
        other.data_.GetFirst() = nullptr;
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        if (!data_.GetFirst()) {
            return *this;
        }
        data_.GetSecond()(data_.GetFirst());
        data_.GetFirst() = std::nullptr_t();
        return *this;
    }

    UniquePtr& operator=(const UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        if (data_.GetFirst()) {
            data_.GetSecond()(data_.GetFirst());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto ptr = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return ptr;
    }

    void Reset(T* ptr = nullptr) noexcept {
        auto first = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(first);
    }

    void Swap(UniquePtr& other) {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.data_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }

    explicit operator bool() const {
        return data_.GetFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    typename std::add_lvalue_reference<T>::type operator*() const {
        return *data_.GetFirst();
    }

    T* operator->() const {
        return data_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> data_;
    template <typename Son, typename SonDeleter>
    friend class UniquePtr;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    UniquePtr(T* ptr) : data_(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, const Deleter& deleter) : data_(ptr, deleter) {
    }

    ~UniquePtr() {
        data_.GetSecond()(data_.GetFirst());
    }

    void Reset(T* ptr = nullptr) noexcept {
        auto first = data_.GetFirst();
        data_.GetFirst() = ptr;
        data_.GetSecond()(first);
    }

    T& operator[](size_t position) {
        return *(data_.GetFirst() + position);
    }

    UniquePtr(T* ptr, Deleter&& deleter) : data_(ptr, std::move(deleter)) {
    }

    UniquePtr(const UniquePtr& other) = delete;

    template <typename Son, typename SonDeleter>
    UniquePtr(UniquePtr<Son[], SonDeleter>&& other) noexcept
            : data_(std::move(other.data_.GetFirst()), other.data_.GetSecond()) {
        other.data_.GetFirst() = nullptr;
    }

private:
    CompressedPair<T*, Deleter> data_;
};
