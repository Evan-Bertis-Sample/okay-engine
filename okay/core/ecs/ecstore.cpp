#include "ecstore.hpp"

#include <okay/core/util/object_pool.hpp>

namespace okay {

ECSEntity EntityComponentStore::createEntity() {
    ObjectPoolHandle handle = _entityMetas.emplace();

    const std::size_t desiredCapacity =
        _reservedEntityCount * static_cast<std::size_t>(POOL_GROWTH_FACTOR);

    for (auto& pool : _componentPools) {
        pool->reserve(desiredCapacity);
    }

    return ECSEntity(this, handle);
}

EntityComponentStore::EntityMeta& EntityComponentStore::getEntityMeta(const ECSEntity& entity) {
    return _entityMetas.get(entity._handle);
}

const EntityComponentStore::EntityMeta& EntityComponentStore::getEntityMeta(
    const ECSEntity& entity) const {
    return _entityMetas.get(entity._handle);
}

bool EntityComponentStore::isValidEntity(const ECSEntity& entity) const {
    return _entityMetas.valid(entity._handle) && entity._ecs == this;
};

void EntityComponentStore::destroyEntity(ECSEntity& entity) {
    EntityMeta& meta = getEntityMeta(entity);
    for (auto& pool : _componentPools) {
        pool->remove(entity._handle.index);
    }
    _entityMetas.destroy(entity._handle);
    entity._ecs = nullptr;
    entity._handle = ObjectPoolHandle::invalidHandle();
}

};  // namespace okay