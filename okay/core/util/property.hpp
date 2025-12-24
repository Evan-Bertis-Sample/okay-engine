#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <type_traits>
#include <utility>

namespace okay {

template <class T>
class Property {
   public:
    using CallbackFn = void (*)(void*);

    struct Delegate {
        CallbackFn fn = nullptr;
        void* ctx = nullptr;

        constexpr bool valid() const { return fn != nullptr; }
        void invoke() const {
            if (fn) fn(ctx);
        }

        static Delegate fromFunction(CallbackFn f, void* c) {
            return Delegate{f, c};
        }
    };

    Property() = default;

    explicit Property(T initial) : _value(std::move(initial)) {}

    Property(T initial, Delegate onDirty) : _value(std::move(initial)), _onDirty(onDirty) {}

    void bind(Delegate onDirty) { _onDirty = onDirty; }
    void clear() { _onDirty = {}; }

    const T& get() const { return _value; }
    T& getMutable() { return _value; }
    operator const T&() const { return _value; }

    void set(const T& value) {
        if (isSameValue(value)) return;
        _value = value;
        notifyDirty();
    }

    void set(T&& value) {
        if (isSameValue(value)) return;
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

    void markDirty() { notifyDirty(); }

   private:
    template <class U, class = void>
    struct HasEq : std::false_type {};

    template <class U>
    struct HasEq<U, std::void_t<decltype(std::declval<const U&>() == std::declval<const U&>())>>
        : std::true_type {};

    template <class V>
    bool isSameValue(const V& value) const {
        if constexpr (HasEq<T>::value) {
            return _value == value;
        } else {
            (void)value;
            return false;
        }
    }

    void notifyDirty() { _onDirty.invoke(); }

    T _value{};
    Delegate _onDirty{};
};

}  // namespace okay

#endif  // __PROPERTY_H__