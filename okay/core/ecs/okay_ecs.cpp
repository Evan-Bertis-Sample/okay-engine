#include <okay/core/ecs/okay_ecs.hpp>

namespace okay {

OkayEntity OkayECS::createEntity() {
    const std::size_t id = _entityMetas.size();
    if (id >= MAX_ENTITIES) {
        Engine.logger.error("Exceeded maximum number of entities ({})", MAX_ENTITIES);
        return OkayEntity{};
    }

    _entityMetas.push_back(EntityMeta{});
    _reservedEntityCount = _entityMetas.size();

    const std::size_t desiredCapacity =
        _reservedEntityCount * static_cast<std::size_t>(POOL_GROWTH_FACTOR);

    for (auto& pool : _componentPools) {
        pool->reserve(desiredCapacity);
    }

    return OkayEntity(this, id);
}

OkayECS::EntityMeta& OkayECS::getEntityMeta(const OkayEntity& entity) {
    return _entityMetas[entity._id];
}

const OkayECS::EntityMeta& OkayECS::getEntityMeta(const OkayEntity& entity) const {
    return _entityMetas[entity._id];
}

};  // namespace okay