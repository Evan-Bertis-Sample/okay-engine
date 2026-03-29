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

class ECS : public System<SystemScope::LEVEL> {
   public:
    ECSEntity createEntity() { return _store.createEntity(); }
    bool isValidEntity(const ECSEntity& entity) const { return _store.isValidEntity(entity); }
    void destroyEntity(ECSEntity& entity) { _store.destroyEntity(entity); }
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

    template <typename Query>
    QueryRange<Query> query() {
        return QueryRange<Query>(this);
    }

   private:
    EntityComponentStore _store;
};

}  // namespace okay

#endif  // __ECS_H__