#ifndef __RESULT_H__
#define __RESULT_H__

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace okay {

template <typename>
inline constexpr bool dependent_false_v = false;

/// @brief A class representing the result of a function: either a value (ok) or an error message.
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

    /// @brief Copy the value out (only available when T is copyable).
    T value() const requires(std::is_copy_constructible_v<T>) {
        return *_value;
    }

    /// @brief If you call value() on a move-only T, you get a clear compile-time error.
    T value() const requires(!std::is_copy_constructible_v<T>) {
        static_assert(dependent_false_v<T>,
                      "Result<T>::value() requires T to be copyable. "
                      "For move-only T (e.g., std::unique_ptr), use valueRef() or take().");
        return T{};
    }

    T& valueRef() { return *_value; }
    const T& valueRef() const { return *_value; }

    /// @brief Move the value out. After take(), Result becomes an error (empty message).
    T take() {
        T out = std::move(*_value);
        _value.reset();
        _errorMessage.clear();
        return out;
    }

    // Nice ergonomics
    T& operator*() { return valueRef(); }
    const T& operator*() const { return valueRef(); }
    T* operator->() { return &valueRef(); }
    const T* operator->() const { return &valueRef(); }

    // Copy only if T is copyable
    Result(const Result& other) requires(std::is_copy_constructible_v<T>)
        : _value(other._value), _errorMessage(other._errorMessage) {}

    Result& operator=(const Result& other) requires(std::is_copy_assignable_v<T>) {
        if (this == &other) return *this;
        _value = other._value;
        _errorMessage = other._errorMessage;
        return *this;
    }

    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;

private:
    std::optional<T> _value;
    std::string _errorMessage;

    template <typename... Args>
    explicit Result(std::in_place_t, Args&&... args)
        : _value(std::in_place, std::forward<Args>(args)...), _errorMessage("") {}

    explicit Result(std::string errorMessage)
        : _value(std::nullopt), _errorMessage(std::move(errorMessage)) {}
};

struct NoneType {};
using Failable = Result<NoneType>;

}  // namespace okay

#endif  // __RESULT_H__
