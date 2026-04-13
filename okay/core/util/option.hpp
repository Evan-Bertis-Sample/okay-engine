#ifndef __OPTION_H__
#define __OPTION_H__

#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace okay {

template <typename T>
struct is_reference_wrapper : std::false_type {};

template <typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};

template <typename T>
struct unwrap_reference_wrapper {
    using type = T;
};

template <typename U>
struct unwrap_reference_wrapper<std::reference_wrapper<U>> {
    using type = U;
};
template <typename T>
using option_value_return_t = std::
    conditional_t<is_reference_wrapper<T>::value, typename unwrap_reference_wrapper<T>::type&, T>;

template <typename T>
using option_const_value_return_t =
    std::conditional_t<is_reference_wrapper<T>::value,
                       const typename unwrap_reference_wrapper<T>::type&,
                       T>;
template <typename T>
class Option {
   public:
    static Option<T> some(T value) { return Option<T>(std::move(value)); }
    static Option<T> none() { return Option<T>(); }

    Option() = default;
    Option(const Option&) = default;
    Option(Option&&) noexcept = default;
    Option& operator=(const Option&) = default;
    Option& operator=(Option&&) noexcept = default;

    Option(T&& value) : _value(std::forward<T>(value)) {}
    Option(const T& value) : _value(value) {}

    Option& operator=(T&& value) {
        _value = std::forward<T>(value);
        return *this;
    }

    Option& operator=(const T& value) {
        _value = value;
        return *this;
    }

    option_value_return_t<T> operator*() {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to dereference none Option");
        }
        return value();
    }

    option_const_value_return_t<T> operator*() const {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to dereference none Option");
        }
        return value();
    }

    option_value_return_t<T> operator->() {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to dereference none Option");
        }
        return value();
    }

    option_const_value_return_t<T> operator->() const {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to dereference none Option");
        }
        return value();
    }

    operator bool() const { return _value.has_value(); }

    bool isSome() const { return _value.has_value(); }
    bool isNone() const { return !_value.has_value(); }

    option_value_return_t<T> value() {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to access value of none Option");
        }

        if constexpr (is_reference_wrapper<T>::value) {
            return _value->get();
        } else {
            return *_value;
        }
    }

    option_const_value_return_t<T> value() const {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to access value of none Option");
        }

        if constexpr (is_reference_wrapper<T>::value) {
            return _value->get();
        } else {
            return *_value;
        }
    }

   private:
    option_internal_type<T> _value;
};

}  // namespace okay

#endif