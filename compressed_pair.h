#pragma once

#include <type_traits>
#include <utility>

enum class OrderPair {
    first, second
};

template<typename T, OrderPair order, bool = std::is_empty_v<T> && !std::is_final_v<T>>
struct CompressedElement {
    CompressedElement() = default;

    template<typename Son>
    explicit CompressedElement(const Son &value) : value_(value) {
    }

    explicit CompressedElement(const T &value) : value_(value) {
    }

    explicit CompressedElement(T &&value) : value_(std::move(value)) {
    }

    template<typename Son>
    explicit CompressedElement(Son &&value) : value_(std::move(value)) {
    }

    CompressedElement(const CompressedElement &other) : value_(other.value_) {
    }

    CompressedElement(CompressedElement &&other) : value_(std::move(other.value_)) {
    }

    template<typename Son>
    explicit CompressedElement(CompressedElement<Son, order> &&other) : value_(std::move(other.value_)) {
    }

    const T &GetElement() const {
        return value_;
    }

    T &GetElement() {
        return value_;
    }

private:
    T value_{};
};

template<typename T, OrderPair order>
struct CompressedElement<T, order, true> : public T {
    CompressedElement() = default;

    explicit CompressedElement(const T &value) {

    };

    template<typename Son>
    explicit CompressedElement(const Son &value) {
    }

    template<typename Son>
    explicit CompressedElement(CompressedElement<Son, order> &&other) {
    }

    CompressedElement(CompressedElement &other) = default;

    CompressedElement(CompressedElement &&other)  noexcept = default;

    const T &GetElement() const {
        return *this;
    }

    T &GetElement() {
        return *this;
    }
};


template<typename F, typename S>
class CompressedPair : private CompressedElement<F, OrderPair::first>,
                       private CompressedElement<S, OrderPair::second> {

    using First = CompressedElement<F, OrderPair::first>;
    using Second = CompressedElement<S, OrderPair::second>;

public:
    CompressedPair() : First(), Second() {
    }

    CompressedPair(const F &first, const S &second) : First(first), Second(second) {
    }

    CompressedPair(F &first, S &&second) : First(first), Second(std::move(second)) {
    }

    CompressedPair(F &&first, S &second) : First(std::move(first)), Second(second) {
    }

    CompressedPair(F &&first, S &&second) : First(std::move(first)), Second(std::move(second)) {
    }

    template<typename SonF, typename SonS>
    CompressedPair(SonF &&first, SonS second) : First(std::move(first)), Second(second) {
    }

    F &GetFirst() {
        return First::GetElement();
    }

    const F &GetFirst() const {
        return First::GetElement();
    }

    S &GetSecond() {
        return Second::GetElement();
    };

    const S &GetSecond() const {
        return Second::GetElement();
    };
};
