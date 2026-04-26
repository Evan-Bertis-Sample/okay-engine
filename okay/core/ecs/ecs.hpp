#ifndef __ECS_H__
#define __ECS_H__

#include "ecstore.hpp"
#include "query.hpp"

#include <okay/core/engine/system.hpp>

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace okay {

class ECS;

class IECSSystem {
   public:
    virtual ~IECSSystem() = default;
    virtual bool matches(ECS& ecs, ECSComponentMask componentMask) const = 0;
    virtual void systemInitialize(ECS& ecs) = 0;
    virtual void systemShutdown(ECS& ecs) = 0;

    virtual void preTick(ECS& ecs) = 0;
    virtual void tick(ECS& ecs) = 0;
    virtual void postTick(ECS& ecs) = 0;

    virtual void entityAdded(ECS& ecs, ECSEntity& entity) = 0;
    virtual void entityRemoved(ECS& ecs, ECSEntity& entity) = 0;
};

class ECS : public EntityComponentStore, public System<SystemScope::LEVEL> {
   public:
    template <typename Query>
    class EntityIterator {
       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename Query::Item;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        EntityIterator() = default;

        EntityIterator(ECS* ecs, std::uint32_t index) : _ecs(ecs), _index(index) {
            Query::initialize(*_ecs);
            skipInvalid();
        }

        value_type operator*() const {
            ObjectPoolHandle handle{_index, _ecs->_entityMetas.generationAt(_index)};
            ECSEntity entity{_ecs, handle};
            return Query::createItem(std::move(entity), *_ecs);
        }

        EntityIterator& operator++() {
            ++_index;
            skipInvalid();
            return *this;
        }

        EntityIterator operator++(int) {
            EntityIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const EntityIterator& other) const {
            return _ecs == other._ecs && _index == other._index;
        }

        bool operator!=(const EntityIterator& other) const {
            return !(*this == other);
        }

       private:
        void skipInvalid() {
            while (_ecs != nullptr && _index < _ecs->_entityMetas.capacity()) {
                if (!_ecs->_entityMetas.aliveAt(_index)) {
                    ++_index;
                    continue;
                }

                const auto& meta = _ecs->_entityMetas.atIndex(_index);
                if (!Query::matches(meta.componentMask)) {
                    ++_index;
                    continue;
                }

                break;
            }
        }

        ECS* _ecs{nullptr};
        std::uint32_t _index{0};
    };

    template <typename Query>
    class QueryRange {
       public:
        explicit QueryRange(ECS* ecs) : _ecs(ecs) {
            Query::initialize(*_ecs);
        }

        EntityIterator<Query> begin() {
            return EntityIterator<Query>(_ecs, 0);
        }

        EntityIterator<Query> end() {
            return EntityIterator<Query>(
                _ecs, static_cast<std::uint32_t>(_ecs->_entityMetas.capacity()));
        }

       private:
        ECS* _ecs;
    };

    template <typename... QueryArgs>
    QueryRange<ECSQuery<QueryArgs...>> query() {
        return QueryRange<ECSQuery<QueryArgs...>>(this);
    }

    void onComponentChange(ECSEntity& entity, EntityMeta& oldMeta) override {
        ECSComponentMask oldMask = oldMeta.componentMask;
        ECSComponentMask newMask = getEntityMeta(entity).componentMask;
        for (auto& system : _systems) {
            if (system->matches(*this, newMask) && !system->matches(*this, oldMask)) {
                system->entityAdded(*this, entity);
            }

            if (!system->matches(*this, newMask) && system->matches(*this, oldMask)) {
                system->entityRemoved(*this, entity);
            }
        }
    }

    void onEntityAdded(ECSEntity& entity) override {
        for (auto& system : _systems) {
            if (system->matches(*this, getEntityMeta(entity).componentMask)) {
                system->entityAdded(*this, entity);
            }
        }
    }

    void onEntityRemoved(ECSEntity& entity) override {
        for (auto& system : _systems) {
            if (system->matches(*this, getEntityMeta(entity).componentMask)) {
                system->entityRemoved(*this, entity);
            }
        }
    }

    void shutdown() override {
        for (auto& system : _systems) {
            system->systemShutdown(*this);
        }
    }

    void preTick() override {
        for (auto& system : _systems) {
            system->preTick(*this);
        }
    }

    void tick() override {
        for (auto& system : _systems) {
            system->tick(*this);
        }
    }

    void postTick() override {
        for (auto& system : _systems) {
            system->postTick(*this);
        }
    }

    template <typename T>
    void addSystem(std::unique_ptr<T> system) {
        static_assert(std::derived_from<T, IECSSystem>, "T must derive from IECSSystem");
        // Engine.logger.info("Adding system: {}", typeid(T).name());
        _systems.push_back(std::move(system));
        _systems.back()->systemInitialize(*this);
    }

    template <typename... Ts>
    void addSystems(std::unique_ptr<Ts>... systems) {
        (addSystem(std::move(systems)), ...);
    }

   private:
    std::vector<std::unique_ptr<IECSSystem>> _systems;
};

template <typename... QueryArgs>
class ECSSystem : public IECSSystem {
   public:
    using QueryT = ECSQuery<QueryArgs...>;

    bool matches(ECS& ecs, ECSComponentMask componentMask) const override {
        QueryT::initialize(ecs);
        return QueryT::matches(componentMask);
    }

    void systemInitialize(ECS& ecs) override {}
    void systemShutdown(ECS& ecs) override {}

    virtual void onEntityAdded(QueryT::Item& item) {};
    virtual void onEntityRemoved(QueryT::Item& item) {};
    virtual void onPreTick(QueryT::Item& item) {};
    virtual void onTick(QueryT::Item& item) {};
    virtual void onPostTick(QueryT::Item& item) {};

    void entityAdded(ECS& ecs, ECSEntity& entity) override {
        auto item = QueryT::createItem(entity, ecs);
        onEntityAdded(item);
    }

    void entityRemoved(ECS& ecs, ECSEntity& entity) override {
        auto item = QueryT::createItem(entity, ecs);
        onEntityRemoved(item);
    }

    void preTick(ECS& ecs) override {
        for (auto item : ecs.query<QueryArgs...>()) {
            onPreTick(item);
        }
    }

    void tick(ECS& ecs) override {
        for (auto item : ecs.query<QueryArgs...>()) {
            onTick(item);
        }
    }

    void postTick(ECS& ecs) override {
        for (auto item : ecs.query<QueryArgs...>()) {
            onPostTick(item);
        }
    }
};

}  // namespace okay

#endif  // __ECS_H__