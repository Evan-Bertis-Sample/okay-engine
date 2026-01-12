#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <functional>
#include <type_traits>
#include <utility>

namespace okay {

// requires an asignment, and equality comparison
template <typename T>
concept PropertyValue = requires(T a, T b) {
    { a = b } -> std::same_as<T&>;
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

template <class T>
    requires PropertyValue<T>
class Property {
   public:
    Property() = default;

    explicit Property(T initial) : _value(std::move(initial)) {
    }

    Property(T initial, std::function<void(T)> onDirty)
        : _value(std::move(initial)), _onDirty(onDirty) {
    }

    void bind(std::function<void(T)> onDirty) {
        _onDirty = onDirty;
    }
    void clear() {
        _onDirty = {};
    }

    const T& get() const {
        return _value;
    }
    T& getMutable() {
        return _value;
    }
    operator const T&() const {
        return _value;
    }

    void set(const T& value) {
        if (isSameValue(value))
            return;
        _value = value;
        notifyDirty();
    }

    void set(T&& value) {
        if (isSameValue(value))
            return;
        _value = std::move(value);
        notifyDirty();
    }

    Property& operator=(const T& value) {
        set(value);
        return *this;
    }

    Property& operator=(T&& value) {
        set(std::move(value));
        return *this;
    }

    void markDirty() {
        notifyDirty();
    }

   private:
    void notifyDirty() {
        _onDirty.invoke(_value);
    }

    T _value{};
    std::function<void(T)> _onDirty;
};

}  // namespace okay

#endif  // __PROPERTY_H__