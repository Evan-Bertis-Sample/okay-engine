#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <okay/core/util/option.hpp>

#include <array>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <type_traits>

namespace okay {

enum SystemScope : std::uint8_t { ENGINE, GAME, LEVEL, SCOPE_COUNT };

class ISystem {
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

    virtual void preTick() {}
    virtual void tick() {}
    virtual void postTick() {}

    virtual void shutdown() {}
};

template <SystemScope ScopeV>
class System : public ISystem {
   public:
    static_assert(ScopeV >= SystemScope::ENGINE && ScopeV < SystemScope::SCOPE_COUNT,
                  "ScopeV must be a valid OkaySystemScope value.");

    static const SystemScope SCOPE = ScopeV;
};

struct OkaySystemDescriptor {
    std::size_t SysId;
    const char* SystemName;

    template <typename T>
    static OkaySystemDescriptor create() {
        static_assert(std::is_base_of<ISystem, T>::value, "T must inherit from OkaySystem");

        return OkaySystemDescriptor(ISystem::sysid<T>(), ISystem::sysName<T>());
    }

   private:
    OkaySystemDescriptor(std::size_t sysid, const char* systemName)
        : SysId(sysid), SystemName(systemName) {}
};

class SystemPool {
   public:
    template <typename T>
    Option<T*> getSystem() {
        static_assert(std::is_base_of<ISystem, T>::value, "T must inherit from OkaySystem.");
        auto it = _systems.find(ISystem::sysid<T>());
        if (it != _systems.end()) {
            return Option<T*>::some(static_cast<T*>(it->second.get()));
        }
        return Option<T*>::none();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from OkaySystem.");
        _systems.emplace(ISystem::sysid<T>(), std::move(system));
    }

    template <typename T>
    bool hasSystem() const {
        static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from OkaySystem.");
        return _systems.find(ISystem::sysid<T>) != _systems.end();
    }

    bool hasSystem(std::size_t hash) const { return _systems.find(hash) != _systems.end(); }

    class Iterator {
       private:
        using base_it = std::map<std::size_t, std::unique_ptr<ISystem>>::iterator;

       public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = ISystem*;
        using difference_type = std::ptrdiff_t;
        using pointer = ISystem**;
        using reference = ISystem*&;

        Iterator() = default;
        explicit Iterator(base_it it) : _it(it) {}

        // Deref to raw pointer
        value_type operator*() const { return _it->second.get(); }
        value_type operator->() const { return _it->second.get(); }

        Iterator& operator++() {
            ++_it;
            return *this;
        }

        Iterator operator++(int) {
            auto tmp = *this;
            ++_it;
            return tmp;
        }

        Iterator& operator--() {
            --_it;
            return *this;
        }

        Iterator operator--(int) {
            auto tmp = *this;
            --_it;
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a._it == b._it; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return a._it != b._it; }

        // expose underlying for internal helpers
        base_it base() const { return _it; }

       private:
        base_it _it{};
    };

    std::size_t size() const noexcept { return _systems.size(); }
    bool empty() const noexcept { return _systems.empty(); }

    Iterator begin() { return Iterator{_systems.begin()}; }
    Iterator end() { return Iterator{_systems.end()}; }

   private:
    std::map<std::size_t, std::unique_ptr<ISystem>> _systems;
};

class SystemManager {
   public:
    SystemManager() {
        for (std::size_t i = 0; i < SystemScope::SCOPE_COUNT; ++i) {
            _pools[i] = SystemPool();
        }
    }

    template <typename T>
    Option<T*> getSystem() {
        return _pools[T::SCOPE].template getSystem<T>();
    }

    template <typename T>
    T* getSystemChecked() {
        Option<T*> opt = getSystem<T>();
        if (!opt) {
            while (true) {
            }
        }
        return opt.value();
    }

    template <typename T>
    void registerSystem(std::unique_ptr<T> system) {
        _pools[T::SCOPE].registerSystem(std::move(system));
    }

    SystemPool& getPool(const SystemScope scope) { return _pools[scope]; }

    bool hasSystem(std::size_t hash) {
        for (const SystemPool& pool : _pools) {
            if (pool.hasSystem(hash))
                return true;
        }
        return false;
    }

   private:
    static std::array<SystemPool, SystemScope::SCOPE_COUNT> _pools;
};
};  // namespace okay

#endif  // _SYSTEM_H__