#ifndef __OKAY_ECS_H__
#define __OKAY_ECS_H__

#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/option.hpp>
#include <bitset>
#include <typeinfo>
#include <type_traits>
#include <memory>
#include <unordered_map>

namespace okay {

class IComponentPool {
   public:
    virtual void reserve(std::size_t size) = 0;
    ~IComponentPool();
};

template <typename T>
class ComponentPool : public IComponentPool {
   public:
    void reserve(std::size_t size) { _pool.reserve(size); }

   private:
    std::vector<T> _pool;
};

class OkayECS : public OkaySystem<OkaySystemScope::LEVEL> {
   public:
    struct Entity {
       public:
        Entity() {}
        Entity(const Entity& other) : _ecs(other._ecs), _id(other._id) {}
        Entity(Entity&& other) : _ecs(other._ecs), _id(other._id) {}
        Entity& operator=(const Entity& other) {
            _ecs = other._ecs;
            _id = other._id;
            return *this;
        }
        Entity& operator=(Entity&& other) {
            _ecs = other._ecs;
            _id = other._id;
            return *this;
        }
        bool operator==(const Entity& other) const {
            return _ecs == other._ecs && _id == other._id;
        }
        bool operator!=(const Entity& other) const { return !(*this == other); }
        bool operator<(const Entity& other) const { return _id < other._id; }
        bool operator>(const Entity& other) const { return _id > other._id; }

        template <typename T>
        Option<T> getComponent() const {
            return _ecs->getComponent<T>(*this);
        }

        template <typename T>
        T getOrAddComponent() const {
            return _ecs->getOrAddComponent<T>(*this);
        }

        template <typename T>
        Entity& addComponent(const T& component) {
            _ecs->addComponent<T>(*this, component);
            return *this;
        }

        template <typename T, typename... Args>
        Entity& addComponent(Args&&... args) {
            _ecs->addComponent<T>(*this, std::forward<Args>(args)...);
            return *this;
        }

        template <typename T>
        Entity& removeComponent() {
            _ecs->removeComponent<T>(*this);
            return *this;
        }

        template <typename T>
        bool hasComponent() const {
            return _ecs->hasComponent<T>(*this);
        }

       private:
        Entity(OkayECS* ecs, std::size_t id) : _ecs(ecs), _id(id) {}

        std::size_t _id{0};
        OkayECS* _ecs{nullptr};
        friend class OkayECS;
    };

    template <typename T>
    void registerComponentType() {
        _componentPools[ComponentInfo::create<T>()] = std::make_unique<ComponentPool<T>>();
    }

    template <typename T>
    void addComponent(Entity& entity, const T& component) {
        Option<std::size_t> componentID = getComponentID<T>();
        if (!componentID) {
            Engine.logger.error("Component {} not registered", typeid(T).name());
            return;
        }
        _entityMetas[entity._id].componentMask.set(componentID.value(), true);
        getPool<T>()->_pool[entity._id] = component;
    }

    template <typename T>
    void removeComponent(Entity& entity) {
        Option<std::size_t> componentID = getComponentID<T>();
        if (!componentID) {
            Engine.logger.error("Component {} not registered", typeid(T).name());
            return;
        }
        _entityMetas[entity._id].componentMask.set(componentID.value(), false);
    }

    template <typename T>
    Option<T> getComponent(Entity& entity) {
        Option<std::size_t> componentID = getComponentID<T>();
        if (!componentID) {
            Engine.logger.error("Component {} not registered", typeid(T).name());
            return Option<T>::none();
        }
        if (!_entityMetas[entity._id].componentMask.test(componentID.value())) {
            return Option<T>::none();
        }
        return Option<T>::some(getPool<T>()->_pool[entity._id]);
    }

    template <typename T>
    T getOrAddComponent(Entity& entity) {
        Option<T> opt = getComponent<T>(entity);
        if (opt) {
            return opt.value();
        }
        T component;
        addComponent<T>(entity, component);
        return component;
    }

    template <typename T>
    bool hasComponent(Entity& entity) {
        Option<std::size_t> componentID = getComponentID<T>();
        if (!componentID) {
            Engine.logger.error("Component {} not registered", typeid(T).name());
            return false;
        }
        return _entityMetas[entity._id].componentMask.test(componentID.value());
    }

    Entity createEntity() {
        Entity entity(this, 0);
        // check if we need to reserve
        if (_entityMetas.size() == _reservedEntityCount) {
            _reservedEntityCount *= POOL_GROWTH_FACTOR;
            _entityMetas.reserve(_reservedEntityCount);

            for (auto& pool : _componentPools) {
                pool->reserve(_reservedEntityCount);
            }
        }

        entity._id = _entityMetas.size();
        _entityMetas.push_back(EntityMeta{});
        return entity;
    };

   private:
    static constexpr std::size_t MAX_COMPONENTS = 32;
    static constexpr std::size_t MAX_ENTITIES = 2048;
    static constexpr int POOL_GROWTH_FACTOR = 2;

    struct ComponentInfo {
        std::size_t hash{0};
        template <typename T>
        static const ComponentInfo create() {
            auto hash = typeid(T).hash_code();
            return {hash};
        }
    };

    struct EntityMeta {
        std::bitset<MAX_COMPONENTS> componentMask;
        bool active{true};
    };

    template <typename T>
    IComponentPool* getPool() {
        ComponentInfo info = ComponentInfo::create<T>();
        auto it = _infoToPoolIndex.find(info);
        if (it == _infoToPoolIndex.end()) {
            Engine.logger.error("Component {} not registered", typeid(T).name());
            return nullptr;
        }
        return _componentPools[_infoToPoolIndex[info]].get();
    }

    template <typename T>
    Option<std::size_t> getComponentID() {
        ComponentInfo info = ComponentInfo::create<T>();
        auto it = _infoToPoolIndex.find(info);
        if (it == _infoToPoolIndex.end()) {
            Engine.logger.error("Component {} not registered", typeid(T).name());
            return Option<std::size_t>::none();
        }
        return Option<std::size_t>::some(it->second);
    }

    std::vector<EntityMeta> _entityMetas;
    std::unordered_map<ComponentInfo, std::size_t> _infoToPoolIndex;
    std::vector<std::unique_ptr<IComponentPool>> _componentPools;
    std::size_t _reservedEntityCount{0};
};

};  // namespace okay

#endif  // __OKAY_ECS_H__