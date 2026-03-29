#include "ecs.hpp"

#include <okay/core/util/object_pool.hpp>

namespace okay {

ECSEntity ECS::createEntity() {
    ObjectPoolHandle handle = _entityMetas.emplace();

    const std::size_t desiredCapacity =
        _reservedEntityCount * static_cast<std::size_t>(POOL_GROWTH_FACTOR);

    for (auto& pool : _componentPools) {
        pool->reserve(desiredCapacity);
    }

    return ECSEntity(this, handle);
}

ECS::EntityMeta& ECS::getEntityMeta(const ECSEntity& entity) {
    return _entityMetas.get(entity._handle);
}

const ECS::EntityMeta& ECS::getEntityMeta(const ECSEntity& entity) const {
    return _entityMetas.get(entity._handle);
}

bool ECS::isValidEntity(const ECSEntity& entity) const {
    return _entityMetas.valid(entity._handle) && entity._ecs == this;
};

void ECS::destroyEntity(ECSEntity& entity) {
    EntityMeta& meta = getEntityMeta(entity);
    for (auto& pool : _componentPools) {
        pool->remove(entity._handle.index);
    }
    _entityMetas.destroy(entity._handle);
    entity._ecs = nullptr;
    entity._handle = ObjectPoolHandle::invalidHandle();
}

};  // namespace okay