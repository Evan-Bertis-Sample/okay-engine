#ifndef __RESULT_H__
#define __RESULT_H__

#include <functional>
#include <okay/core/util/type.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace okay {

template <class R>
concept ResultLike = requires {
    typename R::ValueType;
    { R::errorResult(std::string{}) } -> std::same_as<R>;
};

template <typename T>
class Result {
   public:
    using ValueType = T;

    template <typename U = T>
    static Result<T> ok(U&& value) {
        return Result<T>(std::in_place, std::forward<U>(value));
    }

    static Result<T> errorResult(std::string errorMessage) {
        return Result<T>(std::move(errorMessage));
    }

    Result() : _value(std::nullopt), _errorMessage("") {}

    operator bool() const { return _value.has_value(); }
    bool isError() const { return !_value.has_value(); }
    const std::string& error() const { return _errorMessage; }

    T value() const
        requires(std::is_copy_constructible_v<T>)
    {
        return *_value;
    }

    T value() const
        requires(!std::is_copy_constructible_v<T>)
    {
        static_assert(dependent_false_v<T>,
                      "Result<T>::value() requires T to be copyable. "
                      "For move-only T (e.g., std::unique_ptr), use valueRef() or take().");
        return T{};
    }

    T& valueRef() { return *_value; }
    const T& valueRef() const { return *_value; }

    T take() {
        T out = std::move(*_value);
        _value.reset();
        _errorMessage.clear();
        return out;
    }

    T& operator*() { return valueRef(); }
    const T& operator*() const { return valueRef(); }
    T* operator->() { return &valueRef(); }
    const T* operator->() const { return &valueRef(); }

    Result(const Result& other)
        requires(std::is_copy_constructible_v<T>)
        : _value(other._value), _errorMessage(other._errorMessage) {}

    Result& operator=(const Result& other)
        requires(std::is_copy_assignable_v<T>)
    {
        if (this == &other) return *this;
        _value = other._value;
        _errorMessage = other._errorMessage;
        return *this;
    }

    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;

    template <class F>
    auto then(F&& f) & -> std::invoke_result_t<F, T&>
        requires ResultLike<std::invoke_result_t<F, T&>>
    {
        return thenImpl<T&>(std::forward<F>(f));
    }

    template <class F>
    auto then(F&& f) const& -> std::invoke_result_t<F, const T&>
        requires ResultLike<std::invoke_result_t<F, const T&>>
    {
        return thenImpl<const T&>(std::forward<F>(f));
    }

    template <class F>
    auto then(F&& f) && -> std::invoke_result_t<F, T&>
        requires ResultLike<std::invoke_result_t<F, T&>>
    {
        return thenImpl<T&>(std::forward<F>(f));
    }

    template <class F>
    auto then(F&& f) const&& -> std::invoke_result_t<F, const T&>
        requires ResultLike<std::invoke_result_t<F, const T&>>
    {
        return thenImpl<const T&>(std::forward<F>(f));
    }

   private:
    std::optional<T> _value;
    std::string _errorMessage;

    template <typename... Args>
    explicit Result(std::in_place_t, Args&&... args)
        : _value(std::in_place, std::forward<Args>(args)...), _errorMessage("") {}

    explicit Result(std::string errorMessage)
        : _value(std::nullopt), _errorMessage(std::move(errorMessage)) {}

    template <class Ref, class F>
    auto thenImpl(F&& f) -> std::invoke_result_t<F, Ref>
        requires ResultLike<std::invoke_result_t<F, Ref>>
    {
        using R = std::invoke_result_t<F, Ref>;
        if (isError()) return R::errorResult(_errorMessage);
        if constexpr (std::is_const_v<std::remove_reference_t<Ref>>) {
            return std::invoke(std::forward<F>(f), static_cast<const T&>(*_value));
        } else {
            return std::invoke(std::forward<F>(f), static_cast<T&>(*_value));
        }
    }
};

template <typename U, typename F>
Result<U> catchResult(Result<U>&& r, F&& handler)
    requires(
        requires(F&& fn) { std::invoke(std::forward<F>(fn)); } ||
        requires(F&& fn, const std::string& e) { std::invoke(std::forward<F>(fn), e); })
{
    if (!r) {
        if constexpr (requires(F&& fn, const std::string& e) {
                          std::invoke(std::forward<F>(fn), e);
                      }) {
            std::invoke(std::forward<F>(handler), r.error());
        } else {
            std::invoke(std::forward<F>(handler));
        }
    }
    return std::move(r);
}

using Failable = Result<NoneType>;

}  // namespace okay

#endif  // __RESULT_H__
