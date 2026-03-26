#include "ecs.hpp"

namespace okay {

ECSEntity ECS::createEntity() {
    const std::size_t id = _entityMetas.size();
    if (id >= MAX_ENTITIES) {
        Engine.logger.error("Exceeded maximum number of entities ({})", MAX_ENTITIES);
        return ECSEntity{};
    }

    _entityMetas.push_back(EntityMeta{});
    _reservedEntityCount = _entityMetas.size();

    const std::size_t desiredCapacity =
        _reservedEntityCount * static_cast<std::size_t>(POOL_GROWTH_FACTOR);

    for (auto& pool : _componentPools) {
        pool->reserve(desiredCapacity);
    }

    return ECSEntity(this, id);
}

ECS::EntityMeta& ECS::getEntityMeta(const ECSEntity& entity) {
    return _entityMetas[entity._id];
}

const ECS::EntityMeta& ECS::getEntityMeta(const ECSEntity& entity) const {
    return _entityMetas[entity._id];
}

bool ECS::isValidEntity(const ECSEntity& entity) const {
    return entity._id < _entityMetas.size() && entity._ecs == this;
};

};  // namespace okay