#ifndef __RESOURCE_HPP__
#define __RESOURCE_HPP__

#include <okay/core/util/option.hpp>

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>

namespace okay {

enum class ResourceScope : std::uint8_t { RUNTIME, ENGINE, GAME, LEVEL, SCOPE_COUNT };

struct IResource {};

template <ResourceScope Scope>
struct Resource : public IResource {
    static constexpr ResourceScope SCOPE{Scope};
};

template <typename T>
concept ScopedResource = requires(T a) { std::is_base_of<IResource, T>::value; };

struct ResourceDescriptor {
    std::size_t sysId;
    const char* systemName;

    template <typename T>
        requires ScopedResource<T>
    static ResourceDescriptor create() {
        return ResourceDescriptor(getSysId<T>(), getSysName<T>());
    }

    template <typename T>
        requires ScopedResource<T>
    static constexpr std::size_t getSysId() {
        return typeid(T).hash_code();
    }

    template <typename T>
        requires ScopedResource<T>
    static const char* getSysName() {
        return typeid(T).name();
    }

   private:
    ResourceDescriptor(std::size_t sysId, const char* systemName)
        : sysId(sysId), systemName(systemName) {}
};

class ResourcePool {
   public:
    template <typename T>
        requires ScopedResource<T>
    Option<T*> getResource() {
        auto it = _resources.find(ResourceDescriptor::getSysId<T>());
        if (it != _resources.end()) {
            return Option<T*>::some(static_cast<T*>(it->second.get()));
        }
        return Option<T*>::none();
    }

    template <typename T, typename... Ts>
        requires ScopedResource<T>
    void registerResource(Ts... args) {
        auto it = _resources.find(ResourceDescriptor::getSysId<T>());
        if (it != _resources.end()) {
            _resources.erase(it);
        }
        _resources.emplace(ResourceDescriptor::getSysId<T>(), std::make_unique<T>(args...));
    }

    template <typename T>
        requires ScopedResource<T>
    bool hasResource() const {
        return _resources.contains(ResourceDescriptor::getSysId<T>());
    }

    class Iterator {
       private:
        using base_it = std::map<std::size_t, std::unique_ptr<IResource>>::iterator;

       public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = IResource*;
        using difference_type = std::ptrdiff_t;
        using pointer = IResource**;
        using reference = IResource*&;

        Iterator() = default;
        explicit Iterator(base_it it) : _it(it) {}

        value_type operator*() const {
            return _it->second.get();
        }
        value_type operator->() const {
            return _it->second.get();
        }

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

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a._it == b._it;
        }
        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return a._it != b._it;
        }

        base_it base() const {
            return _it;
        }

       private:
        base_it _it{};
    };

    std::size_t size() const noexcept {
        return _resources.size();
    }
    bool empty() const noexcept {
        return _resources.empty();
    }

    Iterator begin() {
        return Iterator{_resources.begin()};
    }
    Iterator end() {
        return Iterator{_resources.end()};
    }

    void clear() {
        _resources.clear();
    }

   private:
    std::map<std::size_t, std::unique_ptr<IResource>> _resources;
};

class ResourceManager {
   public:
    template <typename T>
        requires ScopedResource<T>
    Option<T*> getResource() {
        return _pools[static_cast<std::size_t>(T::SCOPE)].template getResource<T>();
    }

    template <typename T>
        requires ScopedResource<T>
    T* getResourceChecked() {
        Option<T*> opt = getResource<T>();
        if (!opt) {
            std::cout << "ERROR: Unable to get system " << typeid(T).name() << std::endl;
            while (true) {
            }
        }
        return opt.value();
    }

    template <typename T, typename... Ts>
        requires ScopedResource<T>
    void addResource(Ts... args) {
        _pools[static_cast<std::size_t>(T::SCOPE)].registerResource<T>(args...);
    }

    ResourcePool& getPool(const ResourceScope scope) {
        return _pools[static_cast<std::size_t>(scope)];
    }

    template <typename T>
        requires ScopedResource<T>
    bool hasResource() {
        return _pools[static_cast<std::size_t>(T::SCOPE)].template hasResource<T>();
    }

    void clearResources(const ResourceScope scope) {
        _pools[static_cast<std::size_t>(scope)].clear();
    }

   private:
    std::array<ResourcePool, static_cast<std::size_t>(ResourceScope::SCOPE_COUNT)> _pools;
};

};  // namespace okay

#endif  // __RESOURCE_HPP__
