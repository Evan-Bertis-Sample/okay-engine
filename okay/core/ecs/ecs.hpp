#ifndef __ECS_H__
#define __ECS_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/logger.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/util/type.hpp>

#include <bitset>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace okay {

class ECS;
struct ECSEntity;
template <typename... Components>
struct ECSSystemView;

class IComponentPool {
   public:
    virtual ~IComponentPool() = default;

    virtual void reserve(std::size_t size) = 0;
    virtual void ensureSize(std::size_t size) = 0;
    virtual void remove(std::size_t index) = 0;
    virtual bool has(std::size_t index) const = 0;
};

template <typename T>
class ComponentPool final : public IComponentPool {
   public:
    void reserve(std::size_t size) override {
        _pool.reserve(size);
        _present.reserve(size);
    }

    void ensureSize(std::size_t size) override {
        if (_pool.size() < size) {
            _pool.resize(size);
        }
        if (_present.size() < size) {
            _present.resize(size, false);
        }
    }

    void remove(std::size_t index) override {
        if (index < _present.size()) {
            _present[index] = false;
        }
    }

    bool has(std::size_t index) const override {
        return index < _present.size() && _present[index];
    }

    template <typename... Args>
    T& emplaceAt(std::size_t index, Args&&... args) {
        ensureSize(index + 1);
        _pool[index] = T(std::forward<Args>(args)...);
        _present[index] = true;
        return _pool[index];
    }

    T& get(std::size_t index) { return _pool[index]; }
    const T& get(std::size_t index) const { return _pool[index]; }

   private:
    std::vector<T> _pool;
    std::vector<bool> _present;
};

class ECS : public System<SystemScope::LEVEL> {
   public:
    ECS() = default;

    template <typename T>
    void registerComponentType();

    template <typename T, typename... Args>
    void addComponent(ECSEntity& entity, Args&&... args);

    template <typename T>
    void removeComponent(ECSEntity& entity);

    template <typename T>
    Option<std::reference_wrapper<T>> getComponent(ECSEntity& entity);

    template <typename T>
    Option<std::reference_wrapper<const T>> getComponent(const ECSEntity& entity) const;

    template <typename T, typename... Args>
    T& getOrAddComponent(ECSEntity& entity, Args&&... args);

    template <typename T>
    bool hasComponent(const ECSEntity& entity);

    ECSEntity createEntity();
    bool isValidEntity(const ECSEntity& entity) const;

    std::size_t getEntityCount() const { return _entityMetas.size(); }

    template <typename... Components>
    ECSSystemView<Components...> query();

   private:
    static constexpr std::size_t MAX_COMPONENTS = 32;
    static constexpr int POOL_GROWTH_FACTOR = 2;

    struct ComponentInfo {
        std::size_t hash{0};

        template <typename T>
        static ComponentInfo create() {
            return {typeid(T).hash_code()};
        }

        bool operator==(const ComponentInfo& other) const noexcept { return hash == other.hash; }
    };

    struct ComponentInfoHash {
        std::size_t operator()(const ComponentInfo& info) const noexcept { return info.hash; }
    };

    struct EntityMeta {
        std::bitset<MAX_COMPONENTS> componentMask;
        bool active{true};
    };

    template <typename T>
    ComponentPool<T>& getPool();

    template <typename T>
    const ComponentPool<T>& getPool() const;

    template <typename T>
    Option<std::size_t> getComponentID() const;

    EntityMeta& getEntityMeta(const ECSEntity& entity);
    const EntityMeta& getEntityMeta(const ECSEntity& entity) const;

    ObjectPool<EntityMeta> _entityMetas;
    std::unordered_map<ComponentInfo, std::size_t, ComponentInfoHash> _infoToPoolIndex;
    std::vector<std::unique_ptr<IComponentPool>> _componentPools;
    std::size_t _reservedEntityCount{0};
};

struct ECSEntity {
   public:
    Transform transform;

    ECSEntity() = default;

    bool operator==(const ECSEntity& other) const {
        return _ecs == other._ecs && _handle == other._handle;
    }

    bool operator!=(const ECSEntity& other) const { return !(*this == other); }
    bool operator<(const ECSEntity& other) const { return _handle < other._handle; }
    bool operator>(const ECSEntity& other) const { return _handle > other._handle; }

    template <typename T>
    Option<std::reference_wrapper<const T>> getComponent() const {
        return _ecs->getComponent<T>(*this);
    }

    template <typename T, typename... Args>
    T& getOrAddComponent(Args&&... args) const {
        return _ecs->getOrAddComponent<T>(const_cast<ECSEntity&>(*this),
                                          std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    ECSEntity& addComponent(Args&&... args) {
        _ecs->addComponent<T>(*this, std::forward<Args>(args)...);
        return *this;
    }

    template <typename T>
    ECSEntity& removeComponent() {
        _ecs->removeComponent<T>(*this);
        return *this;
    }

    template <typename T>
    bool hasComponent() const {
        return _ecs->hasComponent<T>(*this);
    }

   private:
    ECSEntity(ECS* ecs, ObjectPoolHandle handle) : _handle(handle), _ecs(ecs) {}

    ObjectPoolHandle _handle;
    ECS* _ecs{nullptr};

    friend class ECS;
    template <typename... Components>
    friend struct ECSSystemView;
};

template <typename T>
void ECS::registerComponentType() {
    static_assert(!std::is_reference_v<T>, "Component type cannot be a reference");
    static_assert(!std::is_const_v<T>, "Component type cannot be const");

    const ComponentInfo info = ComponentInfo::create<T>();
    if (_infoToPoolIndex.find(info) != _infoToPoolIndex.end()) {
        return;
    }

    const std::size_t componentID = _componentPools.size();
    if (componentID >= MAX_COMPONENTS) {
        Engine.logger.error("Exceeded maximum number of component types ({})", MAX_COMPONENTS);
        return;
    }

    _infoToPoolIndex.emplace(info, componentID);
    _componentPools.push_back(std::make_unique<ComponentPool<T>>());
    _componentPools.back()->reserve(_reservedEntityCount);
}

template <typename T>
ComponentPool<T>& ECS::getPool() {
    const ComponentInfo info = ComponentInfo::create<T>();
    auto it = _infoToPoolIndex.find(info);
    if (it == _infoToPoolIndex.end()) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        std::abort();
    }

    return *static_cast<ComponentPool<T>*>(_componentPools[it->second].get());
}

template <typename T>
const ComponentPool<T>& ECS::getPool() const {
    const ComponentInfo info = ComponentInfo::create<T>();
    auto it = _infoToPoolIndex.find(info);
    if (it == _infoToPoolIndex.end()) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        std::abort();
    }

    return *static_cast<const ComponentPool<T>*>(_componentPools[it->second].get());
}

template <typename T>
Option<std::size_t> ECS::getComponentID() const {
    const ComponentInfo info = ComponentInfo::create<T>();
    auto it = _infoToPoolIndex.find(info);
    if (it == _infoToPoolIndex.end()) {
        return Option<std::size_t>::none();
    }

    return Option<std::size_t>::some(it->second);
}

template <typename T, typename... Args>
void ECS::addComponent(ECSEntity& entity, Args&&... args) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return;
    }

    EntityMeta& meta = getEntityMeta(entity);
    meta.componentMask.set(componentID.value(), true);

    auto& pool = getPool<T>();
    pool.emplaceAt(entity._handle.index, std::forward<Args>(args)...);
}

template <typename T>
void ECS::removeComponent(ECSEntity& entity) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return;
    }

    EntityMeta& meta = getEntityMeta(entity);
    meta.componentMask.set(componentID.value(), false);

    getPool<T>().remove(entity._handle.index);
}

template <typename T>
Option<std::reference_wrapper<T>> ECS::getComponent(ECSEntity& entity) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return Option<std::reference_wrapper<T>>::none();
    }

    const EntityMeta& meta = getEntityMeta(entity);
    if (!meta.componentMask.test(componentID.value())) {
        return Option<std::reference_wrapper<T>>::none();
    }

    auto& pool = getPool<T>();
    if (!pool.has(entity._handle.index)) {
        return Option<std::reference_wrapper<T>>::none();
    }

    return Option<std::reference_wrapper<T>>::some(std::ref(pool.get(entity._handle.index)));
}

template <typename T>
Option<std::reference_wrapper<const T>> ECS::getComponent(const ECSEntity& entity) const {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return Option<std::reference_wrapper<const T>>::none();
    }

    const EntityMeta& meta = getEntityMeta(entity);
    if (!meta.componentMask.test(componentID.value())) {
        return Option<std::reference_wrapper<const T>>::none();
    }

    const auto& pool = getPool<T>();
    if (!pool.has(entity._handle.index)) {
        return Option<std::reference_wrapper<const T>>::none();
    }

    return Option<std::reference_wrapper<const T>>::some(std::cref(pool.get(entity._handle.index)));
}

template <typename T, typename... Args>
T& ECS::getOrAddComponent(ECSEntity& entity, Args&&... args) {
    auto existing = getComponent<T>(entity);
    if (existing) {
        return existing.value().get();
    }

    addComponent<T>(entity, std::forward<Args>(args)...);
    return getComponent<T>(entity).value().get();
}

template <typename T>
bool ECS::hasComponent(const ECSEntity& entity) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        return false;
    }

    const EntityMeta& meta = getEntityMeta(entity);
    if (!meta.componentMask.test(componentID.value())) {
        return false;
    }

    return getPool<T>().has(entity._handle.index);
}

template <typename... Components>
ECSSystemView<Components...> ECS::query() {
    return ECSSystemView<Components...>(this);
}

}  // namespace okay

#endif