#ifndef __RESULT_H__
#define __RESULT_H__

#include <sstream>
#include <vector>
#include <string>
#include <initializer_list>
#include <functional>

namespace okay {

/// @brief A class reprsenting the result of a function, used for operations where you can fail, and
/// care about an error message
/// @tparam T The type of the the value
template <typename T>
class Result {
   public:
    /// @brief Create a result with some value -- no error
    /// @param value The value
    /// @return The result with a value
    static Result<T> ok(T value) { return Result<T>(value); }

    /// @brief Create a result with no value, but with an error message
    /// @param errorMessage The error message
    /// @return The result with an error message
    static Result<T> errorResult(std::string errorMessage) { return Result<T>(true, errorMessage); }

    /// @brief Ctor
    Result() : _error(true) {}

    /// @brief Cast to bool
    operator bool() const { return !_error; }

    /// @brief Equals operator
    Result<T> operator=(const Result<T>& other) {
        _value = other._value;
        _error = other._error;
        _errorMessage = other._errorMessage;
        return *this;
    }

    /// @brief Get the value in the result
    /// @return The value in the result -- could be garbage if it is an error
    T value() const { return _value; }

    /// @brief Checks if the result is an error
    /// @return If the result is an error
    bool isError() const { return _error; }

    /// @brief Get the error message
    /// @return The error message
    std::string error() const { return _errorMessage; }

   private:
    T _value;
    bool _error;
    std::string _errorMessage;

    Result(T value) : _value(value), _error(false), _errorMessage("") {}
    Result(bool error, std::string errorMessage) : _error(error), _errorMessage(errorMessage) {}
};

}  // namespace okay

#endif  // __RESULT_H__