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

class ECS : public System<SystemScope::LEVEL> {
   public:
    ECSEntity createEntity() { return _store.createEntity(); }
    ECSEntity createEntity(const ECSEntity& parent) { return _store.createEntity(parent); }
    bool isValidEntity(const ECSEntity& entity) const { return _store.isValidEntity(entity); }
    void destroyEntity(ECSEntity& entity) { _store.destroyEntity(entity); }

    void addChild(const ECSEntity& parent, const ECSEntity& child) {
        _store.addChild(parent, child);
    }

    bool isChildOf(const ECSEntity& parent, const ECSEntity& child) const {
        return _store.isChildOf(parent, child);
    }

    void removeChild(const ECSEntity& parent, const ECSEntity& child) {
        _store.removeChild(parent, child);
    }

    std::size_t getEntityCount() const { return _store.getEntityCount(); }

    template <typename T>
    void registerComponentType() {
        _store.registerComponentType<T>();
    }

    EntityComponentStore& getStore() { return _store; }
    const EntityComponentStore& getStore() const { return _store; }

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
            Query::initialize(_ecs->_store);
            skipInvalid();
        }

        value_type operator*() const {
            ObjectPoolHandle handle{_index, _ecs->_store._entityMetas.generationAt(_index)};
            ECSEntity entity{&_ecs->_store, handle};
            return Query::createItem(std::move(entity), _ecs->_store);
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

        bool operator!=(const EntityIterator& other) const { return !(*this == other); }

       private:
        void skipInvalid() {
            while (_ecs != nullptr && _index < _ecs->_store._entityMetas.capacity()) {
                if (!_ecs->_store._entityMetas.aliveAt(_index)) {
                    ++_index;
                    continue;
                }

                const auto& meta = _ecs->_store._entityMetas.atIndex(_index);
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
        explicit QueryRange(ECS* ecs) : _ecs(ecs) { Query::initialize(_ecs->_store); }

        EntityIterator<Query> begin() { return EntityIterator<Query>(_ecs, 0); }

        EntityIterator<Query> end() {
            return EntityIterator<Query>(
                _ecs, static_cast<std::uint32_t>(_ecs->_store._entityMetas.capacity()));
        }

       private:
        ECS* _ecs;
    };

    template <typename... QueryArgs>
    QueryRange<ECSQuery<QueryArgs...>> query() {
        return QueryRange<ECSQuery<QueryArgs...>>(this);
    }

   private:
    EntityComponentStore _store;
    std::vector<std::unique_ptr<IECSSystem>> _systems;
};

template <typename... QueryArgs>
class ECSSystem : public IECSSystem {
   public:
    using QueryT = ECSQuery<QueryArgs...>;

    bool matches(ECS& ecs, ECSComponentMask componentMask) const override {
        QueryT::initialize(ecs.getStore());
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
        auto item = QueryT::createItem(entity, ecs.getStore());
        onEntityAdded(item);
    }

    void entityRemoved(ECS& ecs, ECSEntity& entity) override {
        auto item = QueryT::createItem(entity, ecs.getStore());
        onEntityRemoved(item);
    }

    void preTick(ECS& ecs) override {
        for (auto& item : ecs.query<QueryArgs...>()) {
            onPreTick(item);
        }
    }

    void tick(ECS& ecs) override {
        for (auto& item : ecs.query<QueryArgs...>()) {
            onTick(item);
        }
    }

    void postTick(ECS& ecs) override {
        for (auto& item : ecs.query<QueryArgs...>()) {
            onPostTick(item);
        }
    }
};

}  // namespace okay

#endif  // __ECS_H__