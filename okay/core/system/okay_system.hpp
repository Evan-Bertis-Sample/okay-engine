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

class IOkaySystem {
   public:
    static std::size_t hash() {
        // Use typeid for RTTI-based hash
        return typeid(IOkaySystem).hash_code();
    }

    virtual OkaySystemScope getScope() = 0;

    virtual void initialize() {}
    virtual void postInitialize() {}

    virtual void tick() {}
    virtual void postTick() {}

    virtual void shutdown() {}

    const OkaySystemConfig& getConfig() { return _systemConfig; }

   protected:
    OkaySystemConfig _systemConfig;
};

template <OkaySystemScope ScopeV>
class OkaySystem : public IOkaySystem {
   public:
    static_assert(std::is_enum_v<ScopeV, OkaySystemScope>, "ScopeV must be an OkaySystemScope!");

    static const OkaySystemScope scope = ScopeV;
};

class OkaySystemPool {
   public:
    template <typename T>
    Option<T*> getSystem() {
        static_assert(std::is_base_of<IOkaySystem, T>::value, "T must inherit from OkaySystem.");
        auto it = _systems.find(T::hash());
        if (it != _systems.end()) {
            return Option<T*>::some(static_cast<T*>(it->second.get()));
        }
        return Option<T*>::none();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        static_assert(std::is_base_of_v<IOkaySystem, T>, "T must inherit from OkaySystem.");
        _systems.emplace(T::hash(), std::move(system));
    }

    /// @brief Checks if a system of type T exists in the pool
    /// @return True if the system exists, false otherwise
    template <typename T>
    bool hasSystem() const {
        static_assert(std::is_base_of_v<IOkaySystem, T>, "T must inherit from OkaySystem.");
        return _systems.find(T::hash()) != _systems.end();
    }

   private:
    std::map<std::size_t, std::unique_ptr<IOkaySystem>> _systems;
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
        // static_assert(std::is_base_of_v<OkaySystem<T>, T>, "T must inherit from OkaySystem<T>.");
        return _pools[T::scope]->template getSystem<T>();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        // static_assert(std::is_base_of_v<IOkaySystem<T>, T>, "T must inherit from
        // OkaySystem<T>.");
        _pools[T::scope]->registerSystem(std::move(system));
    }

   private:
    static std::array<std::unique_ptr<OkaySystemPool>, OkaySystemScope::__COUNT> _pools;
};

};  // namespace okay

#endif  // __OKAY_SYSTEM_H__