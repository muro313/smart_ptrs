#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        ++count_;
        return count_;
    }

    size_t DecRef() {
        --count_;
        return count_;
    }

    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (RefCount() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
            return;
        }
        if (counter_.DecRef() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

    //    virtual ~RefCounted() = default;

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() : pointer_(nullptr) {
        AddPointer();
    }

    IntrusivePtr(std::nullptr_t) : pointer_(nullptr) {
        AddPointer();
    }

    IntrusivePtr(T* ptr) : pointer_(ptr) {
        AddPointer();
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) : pointer_(other.pointer_) {
        AddPointer();
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) : pointer_(std::move(other.pointer_)) {
        other.pointer_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) : pointer_(other.pointer_) {
        AddPointer();
    }
    IntrusivePtr(IntrusivePtr&& other) : pointer_(std::move(other.pointer_)) {
        other.pointer_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (this == &other) {
            return *this;
        }
        DeletePointer();
        pointer_ = other.pointer_;
        AddPointer();
        return *this;
    }
    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (this == &other) {
            return *this;
        }
        DeletePointer();
        pointer_ = other.pointer_;
        other.pointer_ = nullptr;
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        DeletePointer();
    }

    // Modifiers
    void Reset() {
        DeletePointer();
        pointer_ = nullptr;
    }
    void Reset(T* ptr) {
        DeletePointer();
        pointer_ = ptr;
        AddPointer();
    }
    void Swap(IntrusivePtr& other) {
        std::swap(pointer_, other.pointer_);
    }

    // Observers
    T* Get() const {
        return pointer_;
    }

    T& operator*() const {
        return *pointer_;
    }

    T* operator->() const {
        return pointer_;
    }

    size_t UseCount() const {
        if (pointer_) {
            return pointer_->RefCount();
        }
        return 0;
    }
    explicit operator bool() const {
        return pointer_;
    }

private:
    void AddPointer() {
        if (pointer_) {
            pointer_->IncRef();
        }
    }

    void DeletePointer() {
        if (pointer_) {
            pointer_->DecRef();
        }
    }
    T* pointer_;
    template <typename Y>
    friend class IntrusivePtr;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    auto new_intrusive = new T(std::forward<Args>(args)...);
    return IntrusivePtr(new_intrusive);
}
