#ifndef __OPTION_H__
#define __OPTION_H__

#include <optional>
#include <utility>
#include <stdexcept>

namespace okay {

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
    ~Option() = default;

    operator bool() const { return _value.has_value(); }

    bool isSome() const { return _value.has_value(); }
    bool isNone() const { return !_value.has_value(); }

    T& value() {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to access value of none Option");
        }
        return *_value;
    }

    const T& value() const {
        if (!_value.has_value()) {
            throw std::runtime_error("Tried to access value of none Option");
        }
        return *_value;
    }

   private:
    std::optional<T> _value;

    explicit Option(T value) : _value(std::move(value)) {}
};

}  // namespace okay

#endif  // __OPTION_H__