#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <memory>

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class EnableSharedFromThisBase {};

template <typename T>
class EnableSharedFromThis;

struct ControlBlock {
    size_t strong_count = 1;
    size_t weak_count = 0;
    virtual ~ControlBlock() = default;
    virtual ControlBlock& DeleterPointer() {
        return *this;
    }
    bool alive = true;
};

template <typename T>
struct ControlBlockPtr : public ControlBlock {

    explicit ControlBlockPtr(T* pointer) : pointer_(pointer) {
    }

    T* GetPointer() {
        return pointer_;
    }

    T* pointer_;
    ~ControlBlockPtr() override {
        //        delete pointer_;
    }

    ControlBlock& DeleterPointer() override {
        delete pointer_;
        //        pointer_ = nullptr;
        alive = false;
        return *this;
    }
};

template <typename T>
struct ControlBlockEmplace : public ControlBlock {
    //    template <typename... Args>
    //    ControlBlockEmplace(Args... args) {
    //        new (&storage) T(std::forward<Args&&>(args)...);
    //    }

    template <typename... Args>
    ControlBlockEmplace(Args&&... args) {
        new (&storage) T(std::forward<Args>(args)...);
    }

    T* GetPointer() {
        return reinterpret_cast<T*>(&storage);
    }

    std::aligned_storage_t<sizeof(T), alignof(T)> storage;
    ~ControlBlockEmplace() override {
        //        std::destroy_at(std::launder(reinterpret_cast<T*>(&storage)));
    }

    ControlBlock& DeleterPointer() override {
        std::destroy_at(std::launder(reinterpret_cast<T*>(&storage)));
        //        reinterpret_cast<T*>(&storage) = nullptr;
        return *this;
    }
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr(ControlBlock* block, T* pointer) : block_(block), pointer_(pointer) {
        if (block_) {
            if (!block_->alive) {
                ++block_->strong_count;
            }
        }
    }
    SharedPtr(ControlBlockEmplace<T>* block) : block_(block), pointer_(block->GetPointer()) {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(block->GetPointer());
        }
    }

    SharedPtr() : block_(nullptr), pointer_(nullptr) {
    }

    SharedPtr(std::nullptr_t) : block_(nullptr), pointer_(nullptr) {
    }

    explicit SharedPtr(T* ptr) : block_(new ControlBlockPtr<T>(ptr)), pointer_(ptr) {
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr);
        }
    }

    template <typename Son>
    explicit SharedPtr(Son* ptr) : block_(new ControlBlockPtr<Son>(ptr)), pointer_(ptr) {
        if constexpr (std::is_convertible_v<Son*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr);
        }
    }

    SharedPtr(const SharedPtr& other) : block_(other.block_), pointer_(other.pointer_) {
        if (block_) {
            ++block_->strong_count;
        }
    }

    template <typename S>
    SharedPtr(const SharedPtr<S>& other) : block_(other.block_), pointer_(other.pointer_) {
        if (block_) {
            ++block_->strong_count;
        }
    }

    SharedPtr(SharedPtr&& other) : block_(other.block_), pointer_(other.pointer_) {
        other.block_ = nullptr;
        other.pointer_ = nullptr;
    }

    template <typename Son>
    SharedPtr(SharedPtr<Son>&& other) : block_(other.block_), pointer_(other.pointer_) {
        other.block_ = nullptr;
        other.pointer_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        if (other.block_) {
            block_ = other.block_;
            pointer_ = ptr;
            ++block_->strong_count;
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) : block_(other.block_), pointer_(other.pointer_) {
        if (block_) {
            if (block_->strong_count == 0) {
                throw BadWeakPtr();
            }
            ++block_->strong_count;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }
        DeleteBlock();
        block_ = other.block_;
        pointer_ = other.pointer_;
        if (block_) {
            ++block_->strong_count;
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }
        DeleteBlock();
        block_ = other.block_;
        pointer_ = other.pointer_;
        other.block_ = nullptr;
        other.pointer_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        DeleteBlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        DeleteBlock();
        block_ = nullptr;
        pointer_ = nullptr;
    }
    void Reset(T* ptr) {
        DeleteBlock();
        block_ = new ControlBlockPtr<T>(ptr);
        pointer_ = ptr;
    }
    template <class Son>
    void Reset(Son* ptr) {
        DeleteBlock();
        block_ = new ControlBlockPtr<Son>(ptr);
        pointer_ = ptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(pointer_, other.pointer_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
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
        if (block_) {
            return block_->strong_count;
        }
        return 0;
    }
    explicit operator bool() const {
        return pointer_;
    }

private:
    void DeleteBlock() {
        if (block_) {
            --block_->strong_count;
            if (block_->strong_count == 0) {
                if (block_->alive) {
                    block_->DeleterPointer();
                }
                //                pointer_ = nullptr;
                if (block_->weak_count == 0) {
                    delete block_;
                }
            }
        }
    }

    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->weak_this_ = *this;
    }

    ControlBlock* block_;
    T* pointer_;

    template <typename Son>
    friend class SharedPtr;
    template <typename Son>
    friend class WeakPtr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto block = new ControlBlockEmplace<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(block);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        SharedPtr<T> shared_this(weak_this_);
        return shared_this;
    }

    SharedPtr<const T> SharedFromThis() const {
        SharedPtr<T> shared_this(weak_this_);
        return shared_this;
    }

    WeakPtr<T> WeakFromThis() noexcept {
        WeakPtr<T> new_weak(weak_this_);
        return new_weak;
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        WeakPtr<T> new_weak(weak_this_);
        return new_weak;
    }

private:
    WeakPtr<T> weak_this_;

    template <typename Son>
    friend class SharedPtr;
};
