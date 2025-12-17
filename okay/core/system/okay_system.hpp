#ifndef __OKAY_SYSTEM_H__
#define __OKAY_SYSTEM_H__

#include <array>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <okay/core/util/option.hpp>
#include <stdexcept>
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
    template <typename T>
    static const std::size_t sysid() {
        return typeid(T).hash_code();
    }

    template <typename T>
    static const char* sysName() {
        return typeid(T).name();
    }

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
    static_assert(ScopeV >= OkaySystemScope::ENGINE && ScopeV < OkaySystemScope::__COUNT,
                  "ScopeV must be a valid OkaySystemScope value.");

    static const OkaySystemScope scope = ScopeV;
};

struct OkaySystemDescriptor {
    std::size_t SysId;
    const char* SystemName;

    template <typename T>
    static OkaySystemDescriptor create() {
        static_assert(std::is_base_of<IOkaySystem, T>::value, "T must inherit from OkaySystem");

        return OkaySystemDescriptor(IOkaySystem::sysid<T>(), IOkaySystem::sysName<T>());
    }

   private:
    OkaySystemDescriptor(std::size_t sysid, const char* systemName)
        : SysId(sysid), SystemName(systemName) {}
};

class OkaySystemPool {
   public:
    template <typename T>
    Option<T*> getSystem() {
        static_assert(std::is_base_of<IOkaySystem, T>::value, "T must inherit from OkaySystem.");
        auto it = _systems.find(IOkaySystem::sysid<T>());
        if (it != _systems.end()) {
            return Option<T*>::some(static_cast<T*>(it->second.get()));
        }
        return Option<T*>::none();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        static_assert(std::is_base_of_v<IOkaySystem, T>, "T must inherit from OkaySystem.");
        _systems.emplace(IOkaySystem::sysid<T>(), std::move(system));
    }

    /// @brief Checks if a system of type T exists in the pool
    /// @return True if the system exists, false otherwise
    template <typename T>
    bool hasSystem() const {
        static_assert(std::is_base_of_v<IOkaySystem, T>, "T must inherit from OkaySystem.");
        return _systems.find(IOkaySystem::sysid<T>) != _systems.end();
    }

    bool hasSystem(std::size_t hash) const { return _systems.find(hash) != _systems.end(); }

    class iterator {
       private:
        using base_it = std::map<std::size_t, std::unique_ptr<IOkaySystem>>::iterator;

       public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = IOkaySystem*;
        using difference_type = std::ptrdiff_t;
        using pointer = IOkaySystem**;
        using reference = IOkaySystem*&;

        iterator() = default;
        explicit iterator(base_it it) : _it(it) {}

        // Deref to raw pointer
        value_type operator*() const { return _it->second.get(); }
        value_type operator->() const { return _it->second.get(); }

        iterator& operator++() {
            ++_it;
            return *this;
        }

        iterator operator++(int) {
            auto tmp = *this;
            ++_it;
            return tmp;
        }

        iterator& operator--() {
            --_it;
            return *this;
        }

        iterator operator--(int) {
            auto tmp = *this;
            --_it;
            return tmp;
        }

        friend bool operator==(const iterator& a, const iterator& b) { return a._it == b._it; }
        friend bool operator!=(const iterator& a, const iterator& b) { return a._it != b._it; }

        // expose underlying for internal helpers
        base_it base() const { return _it; }

       private:
        base_it _it{};
    };

    std::size_t size() const noexcept { return _systems.size(); }
    bool empty() const noexcept { return _systems.empty(); }

    IOkaySystem* nth_ptr(std::size_t i) {
        auto it = _systems.begin();
        std::advance(it, static_cast<std::ptrdiff_t>(i));
        return it->second.get();
    }

    IOkaySystem* operator[](std::size_t i) { return nth_ptr(i); }

    IOkaySystem* at(std::size_t i) {
        if (i >= _systems.size()) throw std::out_of_range("OkaySystemPool::at");
        return nth_ptr(i);
    }

    iterator begin() { return iterator{_systems.begin()}; }
    iterator end() { return iterator{_systems.end()}; }

   private:
    std::map<std::size_t, std::unique_ptr<IOkaySystem>> _systems;
};

class OkaySystemManager {
   public:
    OkaySystemManager() {
        for (std::size_t i = 0; i < OkaySystemScope::__COUNT; ++i) {
            _pools[i] = OkaySystemPool();
        }
    }

    template <typename T>
    Option<T*> getSystem() {
        return _pools[T::scope].template getSystem<T>();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        _pools[T::scope].registerSystem(std::move(system));
    }

    OkaySystemPool& getPool(const OkaySystemScope scope) { return _pools[scope]; }

    bool hasSystem(std::size_t hash) {
        for (const OkaySystemPool& pool : _pools) {
            if (pool.hasSystem(hash)) return true;
        }
        return false;
    }

   private:
    static std::array<OkaySystemPool, OkaySystemScope::__COUNT> _pools;
};
};  // namespace okay

#endif  // __OKAY_SYSTEM_H__