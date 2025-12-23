#ifndef __OKAY_SINGLETON_HPP__
#define __OKAY_SINGLETON_HPP__

#include <type_traits>
#include <utility>

namespace okay {

template <class T>
class Singleton {
   public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    static T& instance() noexcept(std::is_nothrow_default_constructible_v<T>) {
        static_assert(std::is_base_of_v<Singleton<T>, T>,
                      "T must derive from Singleton<T> (CRTP).");
        static_assert(std::is_default_constructible_v<T>,
                      "T must be default-constructible (or befriend Singleton<T>).");

        static T s_instance{};
        return s_instance;
    }

   protected:
    Singleton() = default;
    ~Singleton() = default;
};

}  // namespace okay

#endif  // __OKAY_SINGLETON_HPP__
