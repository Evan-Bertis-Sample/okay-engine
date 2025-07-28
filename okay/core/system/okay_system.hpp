#ifndef __OKAY_SYSTEM_H__
#define __OKAY_SYSTEM_H__

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <okay/core/util/option.hpp>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace okay {

enum OkaySystemScope : std::uint8_t { ENGINE, GAME, LEVEL, __COUNT };

struct OkaySystemConfig {
    std::uint32_t tickTime;
};

template <typename T>
class OkaySystem {
   public:
    static constexpr OkaySystemScope scope = T::scope;

    template <typename hashT>
    static constexpr std::size_t hash() {
        return std::is_same<hashT, OkaySystem>::value ? 0 : typeid(hashT).hash_code();
    }

    static constexpr std::size_t hash() { return hash<T>(); }
    static constexpr const char* name() { return typeid(T).name(); }

    virtual void initialize() {}
    virtual void postInitialize() {}

    virtual void tick() {}
    virtual void postTick() {}

    virtual void shutdown() {}
};

class OkaySystemPool {
   public:
    template <typename T>
    Option<T*> getSystem() {
        static_assert(std::is_base_of<OkaySystem<T>, T>::value,
                      "T must inherit from OkaySystem<T>.");
        auto it = _systems.find(OkaySystem<T>::hash());
        if (it != _systems.end()) {
            return Option<T*>::some(static_cast<T*>(it->second.get()));
        }
        return Option<T*>::none();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        static_assert(std::is_base_of<T, OkaySystem<T>>::value,
                      "T must inherit from OkaySystem<T>.");

        _systems.emplace(OkaySystem<T>::hash(), std::move(system));
    }

    /// @brief Checks if a system of type T exists in the pool
    /// @return True if the system exists, false otherwise
    template <typename T>
    bool hasSystem() const {
        static_assert(std::is_base_of<OkaySystem<T>, T>::value,
                      "T must inherit from OkaySystem<T>.");
        return _systems.find(OkaySystem<T>::hash()) != _systems.end();
    }

   private:
    std::map<std::size_t, std::unique_ptr<OkaySystem<void>>> _systems;
};

class OkaySystemManager {
   public:
    OkaySystemManager() {
        for (std::size_t i = 0; i < OkaySystemScope::__COUNT; ++i) {
            _pools[i] = std::make_unique<OkaySystemPool>();
        }
    }

    template <typename T>
    Option<T*> getSystem() {
        static_assert(std::is_base_of<T, OkaySystem<T>>::value,
                      "T must inherit from OkaySystem<T>.");
        return _pools[T::scope]->template getSystem<T>();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        static_assert(std::is_base_of<T, OkaySystem<T>>::value,
                      "T must inherit from OkaySystem<T>.");

        _pools[T::scope]->registerSystem(std::move(system));
    }

   private:
    static std::array<std::unique_ptr<OkaySystemPool>, OkaySystemScope::__COUNT> _pools;
};

};  // namespace okay

#endif  // __OKAY_SYSTEM_H__