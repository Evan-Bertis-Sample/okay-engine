#ifndef __OPTION_H__
#define __OPTION_H__

namespace okay {

/// @brief A class representing an optional vaLue, use for operations where you can fail, but don't
/// really need to have the overhead of a detailed error message
/// @tparam T The type of the value to return
template <typename T>
class Option {
   public:
    /// @brief Create an option with some value
    /// @param value The value to place
    /// @return An option with some value
    static Option<T> some(T value) { return Option<T>(value); }

    /// @brief Create an option with no value
    /// @return Return an option with no value
    static constexpr Option<T> none() { return Option<T>(); }

    /// @brief Ctor
    Option() : _hasValue(false), _value(T()) {}

    /// @brief Equals operator
    Option<T> operator=(const Option<T>& other) {
        _value = other._value;
        _hasValue = other._hasValue;
        return *this;
    }

    /// @brief Cast to bool
    operator bool() const { return _hasValue; }

    /// @brief Checks if a value is stored in the option
    /// @return If there is a value in the option
    bool isSome() const { return _hasValue; }

    /// @brief Checks if there is no value in the option
    /// @return If there is no value in the option
    bool isNone() const { return !_hasValue; }

    /// @brief Gets the value in the option, does not perform any checks
    /// @return The value in the option -- could be garbage if there is no value
    T value() const { return _value; }

   private:
    T _value;
    bool _hasValue;

    Option(T value) : _value(value), _hasValue(true) {}
};

}  // namespace okay

#endif  // __OPTION_H__
