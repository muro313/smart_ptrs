#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr()
            : block_(nullptr),
              pointer_(nullptr){

    };

    WeakPtr(const WeakPtr& other) : block_(other.block_), pointer_(other.pointer_) {
        AddWeak();
    }

    template <typename S>
    WeakPtr(const WeakPtr<S>& other) : block_(other.block_), pointer_(other.pointer_) {
        AddWeak();
    }

    WeakPtr(WeakPtr&& other) : block_(other.block_), pointer_(other.pointer_) {
        other.block_ = nullptr;
        other.pointer_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) : block_(other.block_), pointer_(other.pointer_) {
        AddWeak();
    }

    template <class Son>
    WeakPtr(const SharedPtr<Son>& other) : block_(other.block_), pointer_(other.pointer_) {
        AddWeak();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }
        DeleteWeakFromThis();
        block_ = other.block_;
        pointer_ = other.pointer_;
        AddWeak();
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (this == &other) {
            return *this;
        }
        DeleteWeakFromThis();
        block_ = other.block_;
        pointer_ = other.pointer_;
        other.block_ = nullptr;
        other.pointer_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        DeleteWeak();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        DeleteWeakFromThis();
        pointer_ = nullptr;
        block_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
        std::swap(pointer_, other.pointer_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_) {
            return block_->strong_count;
        }
        return 0;
    }

    bool Expired() const {
        if (!block_) {
            return true;
        }
        if (block_->strong_count == 0) {
            return true;
        }
        return false;
    }

    SharedPtr<T> Lock() const {
        if (Expired()) {
            SharedPtr<T> shared_pointer = SharedPtr<T>(block_, nullptr);
            return shared_pointer;
        }
        SharedPtr<T> shared_pointer = SharedPtr<T>(block_, pointer_);
        if (block_) {
            ++block_->strong_count;
        }
        return shared_pointer;
    }

private:
    void AddWeak() {
        if (block_) {
            ++block_->weak_count;
        }
    }

    void DeleteWeak() {
        if (block_) {
            --block_->weak_count;
            if (block_->weak_count == 0 && block_->strong_count == 0) {
                if (block_->alive) {
                    if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
                        return;
                    }
                    delete block_;
                    pointer_ = nullptr;
                    block_ = nullptr;
                } else {
                    delete block_;
                    pointer_ = nullptr;
                    block_ = nullptr;
                }
            }
        }
    }
    void DeleteWeakFromThis() {
        if (block_) {
            --block_->weak_count;
            if (block_->weak_count == 0 && block_->strong_count == 0) {
                delete block_;
                pointer_ = nullptr;
                block_ = nullptr;
            }
        }
    }
    ControlBlock* block_;
    T* pointer_;

    template <typename F>
    friend class SharedPtr;

    template <typename F>
    friend class WeakPtr;

    template <typename F>
    friend class EnableSharedFromThis;
};
