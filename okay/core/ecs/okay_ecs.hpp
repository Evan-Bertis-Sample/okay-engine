#ifndef __OKAY_ECS_H__
#define __OKAY_ECS_H__

#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/util/type.hpp>

#include <bitset>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace okay {

class OkayECS;
struct OkayEntity;
template <typename... Components>
struct OkaySystemView;

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

class OkayECS : public OkaySystem<OkaySystemScope::LEVEL> {
   public:
    OkayECS() = default;

    template <typename T>
    void registerComponentType();

    template <typename T, typename... Args>
    void addComponent(OkayEntity& entity, Args&&... args);

    template <typename T>
    void removeComponent(OkayEntity& entity);

    template <typename T>
    Option<T> getComponent(const OkayEntity& entity);

    template <typename T, typename... Args>
    T getOrAddComponent(OkayEntity& entity, Args&&... args);

    template <typename T>
    bool hasComponent(const OkayEntity& entity);

    OkayEntity createEntity();
    bool isValidEntity(const OkayEntity& entity) const;

    std::size_t getEntityCount() const { return _entityMetas.size(); }

   private:
    static constexpr std::size_t MAX_COMPONENTS = 32;
    static constexpr std::size_t MAX_ENTITIES = 2048;
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

    EntityMeta& getEntityMeta(const OkayEntity& entity);
    const EntityMeta& getEntityMeta(const OkayEntity& entity) const;

    std::vector<EntityMeta> _entityMetas;
    std::unordered_map<ComponentInfo, std::size_t, ComponentInfoHash> _infoToPoolIndex;
    std::vector<std::unique_ptr<IComponentPool>> _componentPools;
    std::size_t _reservedEntityCount{0};
};

struct OkayEntity {
   public:
    OkayTransform transform;

    OkayEntity() = default;

    bool operator==(const OkayEntity& other) const {
        return _ecs == other._ecs && _id == other._id;
    }

    bool operator!=(const OkayEntity& other) const { return !(*this == other); }
    bool operator<(const OkayEntity& other) const { return _id < other._id; }
    bool operator>(const OkayEntity& other) const { return _id > other._id; }

    template <typename T>
    Option<T> getComponent() const {
        return _ecs->getComponent<T>(*this);
    }

    template <typename T, typename... Args>
    T getOrAddComponent(Args&&... args) const {
        return _ecs->getOrAddComponent<T>(const_cast<OkayEntity&>(*this),
                                          std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    OkayEntity& addComponent(Args&&... args) {
        _ecs->addComponent<T>(*this, std::forward<Args>(args)...);
        return *this;
    }

    template <typename T>
    OkayEntity& removeComponent() {
        _ecs->removeComponent<T>(*this);
        return *this;
    }

    template <typename T>
    bool hasComponent() const {
        return _ecs->hasComponent<T>(*this);
    }

   private:
    OkayEntity(OkayECS* ecs, std::size_t id) : _id(id), _ecs(ecs) {}

    std::size_t _id{0};
    OkayECS* _ecs{nullptr};

    friend class OkayECS;
    template <typename... Components>
    friend struct OkaySystemView;
};

template <typename... Components>
struct OkaySystemView {
    OkayECS* _ecs{nullptr};

    OkaySystemView(OkayECS* ecs) : _ecs(ecs), _entity(ecs, 0) {};
    OkaySystemView(const OkaySystemView& other) : _ecs(other._ecs), _entity(other._entity) {}
    OkaySystemView(OkayECS* ecs, std::size_t id) : _ecs(ecs), _entity(ecs, id) {}

    OkaySystemView& operator++() {
        do {
            ++_entity._id;
        } while (!hasAllComponents(_entity) && _ecs->isValidEntity(_entity));

        return *this;
    }

    OkayEntity operator*() const { return _entity; }

    bool operator==(const OkaySystemView& other) const { return _entity == other._entity; }
    bool operator!=(const OkaySystemView& other) const { return !(*this == other); }
    bool operator<(const OkaySystemView& other) const { return _entity < other._entity; }
    bool operator>(const OkaySystemView& other) const { return _entity > other._entity; }

    OkaySystemView begin() const { return OkaySystemView(_ecs); }
    OkaySystemView end() const { return OkaySystemView(_ecs, _ecs->getEntityCount()); }

   private:
    OkayEntity _entity;

    bool hasAllComponents(const OkayEntity& entity) const {
        bool hasAll = true;
        ((hasAll &= entity.hasComponent<Components>()), ...);
        return hasAll;
    }
};

template <typename T>
void OkayECS::registerComponentType() {
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
ComponentPool<T>& OkayECS::getPool() {
    const ComponentInfo info = ComponentInfo::create<T>();
    auto it = _infoToPoolIndex.find(info);
    if (it == _infoToPoolIndex.end()) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        std::abort();
    }
    
    return *static_cast<ComponentPool<T>*>(_componentPools[it->second].get());
}

template <typename T>
const ComponentPool<T>& OkayECS::getPool() const {
    const ComponentInfo info = ComponentInfo::create<T>();
    auto it = _infoToPoolIndex.find(info);
    if (it == _infoToPoolIndex.end()) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        std::abort();
    }

    return *static_cast<const ComponentPool<T>*>(_componentPools[it->second].get());
}

template <typename T>
Option<std::size_t> OkayECS::getComponentID() const {
    const ComponentInfo info = ComponentInfo::create<T>();
    auto it = _infoToPoolIndex.find(info);
    if (it == _infoToPoolIndex.end()) {
        return Option<std::size_t>::none();
    }

    return Option<std::size_t>::some(it->second);
}

template <typename T, typename... Args>
void OkayECS::addComponent(OkayEntity& entity, Args&&... args) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return;
    }

    EntityMeta& meta = getEntityMeta(entity);
    meta.componentMask.set(componentID.value(), true);

    auto& pool = getPool<T>();
    pool.emplaceAt(entity._id, std::forward<Args>(args)...);
}

template <typename T>
void OkayECS::removeComponent(OkayEntity& entity) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return;
    }

    EntityMeta& meta = getEntityMeta(entity);
    meta.componentMask.set(componentID.value(), false);

    getPool<T>().remove(entity._id);
}

template <typename T>
Option<T> OkayECS::getComponent(const OkayEntity& entity) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        Engine.logger.error("Component {} not registered", typeid(T).name());
        return Option<T>::none();
    }

    const EntityMeta& meta = getEntityMeta(entity);
    if (!meta.componentMask.test(componentID.value())) {
        return Option<T>::none();
    }

    const auto& pool = getPool<T>();
    if (!pool.has(entity._id)) {
        return Option<T>::none();
    }

    return Option<T>::some(pool.get(entity._id));
}

template <typename T, typename... Args>
T OkayECS::getOrAddComponent(OkayEntity& entity, Args&&... args) {
    Option<T> existing = getComponent<T>(entity);
    if (existing) {
        return existing.value();
    }

    addComponent<T>(entity, std::forward<Args>(args)...);
    return getComponent<T>(entity).value();
}

template <typename T>
bool OkayECS::hasComponent(const OkayEntity& entity) {
    const Option<std::size_t> componentID = getComponentID<T>();
    if (!componentID) {
        return false;
    }

    const EntityMeta& meta = getEntityMeta(entity);
    if (!meta.componentMask.test(componentID.value())) {
        return false;
    }

    return getPool<T>().has(entity._id);
}

}  // namespace okay

#endif  // __OKAY_ECS_H__