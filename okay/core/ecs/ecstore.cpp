#include "ecstore.hpp"

#include <okay/core/util/object_pool.hpp>

namespace okay {

ECSEntity EntityComponentStore::createEntity() {
    EntityMeta meta{};
    meta.id = _nextEntityID++;

    ObjectPoolHandle handle = _entityMetas.emplace(meta);

    const std::size_t desiredCapacity =
        _reservedEntityCount * static_cast<std::size_t>(POOL_GROWTH_FACTOR);

    for (auto& pool : _componentPools) {
        pool->reserve(desiredCapacity);
    }

    return ECSEntity(this, handle);
}

ECSEntity EntityComponentStore::createEntity(const ECSEntity& parent) {
    ECSEntity entity = createEntity();
    addChild(parent, entity);
    return entity;
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

std::uint32_t EntityComponentStore::getEntityID(const ECSEntity& entity) const {
    if (!isValidEntity(entity)) {
        Engine.logger.error("Invalid entity");
        return 0;
    }
    return getEntityMeta(entity).id;
}

void EntityComponentStore::destroyEntity(ECSEntity& entity) {
    EntityMeta& meta = getEntityMeta(entity);

    // delete from the bottom of the hierarchy up to avoid invalidating parent/child relationships
    // before they're processed
    ObjectPoolHandle childHandle = meta.firstChild;
    while (childHandle != ObjectPoolHandle::invalidHandle()) {
        EntityMeta& childMeta = _entityMetas.get(childHandle);
        ECSEntity child{this, childHandle};
        destroyEntity(child);
        childHandle = childMeta.nextSibling;
    }

    for (auto& pool : _componentPools) {
        pool->remove(entity._handle.index);
    }
    _entityMetas.destroy(entity._handle);
    entity._ecs = nullptr;
    entity._handle = ObjectPoolHandle::invalidHandle();
}

void EntityComponentStore::addChild(const ECSEntity& parent, const ECSEntity& child) {
    if (!isValidEntity(parent) || !isValidEntity(child)) {
        Engine.logger.error("Invalid parent or child entity");
        return;
    }
    EntityMeta& parentMeta = getEntityMeta(parent);
    EntityMeta& childMeta = getEntityMeta(child);

    childMeta.parent = parent._handle;

    ObjectPoolHandle& siblingHead = parentMeta.firstChild;
    while (siblingHead != ObjectPoolHandle::invalidHandle()) {
        EntityMeta& siblingMeta = _entityMetas.get(siblingHead);
        if (siblingMeta.nextSibling == ObjectPoolHandle::invalidHandle()) {
            siblingMeta.nextSibling = child._handle;
            childMeta.previousSibling = siblingHead;
            return;
        }
        siblingHead = siblingMeta.nextSibling;
    }
}

bool EntityComponentStore::isChildOf(const ECSEntity& parent, const ECSEntity& child) const {
    if (!isValidEntity(parent) || !isValidEntity(child)) {
        Engine.logger.error("Invalid parent or child entity");
        return false;
    }
    const EntityMeta& childMeta = getEntityMeta(child);
    return childMeta.parent == parent._handle;
}

void EntityComponentStore::removeChild(const ECSEntity& parent, const ECSEntity& child) {
    if (!isValidEntity(parent) || !isValidEntity(child)) {
        Engine.logger.error("Invalid parent or child entity");
        return;
    }
    EntityMeta& parentMeta = getEntityMeta(parent);
    EntityMeta& childMeta = getEntityMeta(child);

    ObjectPoolHandle& siblingHead = parentMeta.firstChild;
    while (siblingHead != ObjectPoolHandle::invalidHandle()) {
        EntityMeta& siblingMeta = _entityMetas.get(siblingHead);
        if (siblingMeta.nextSibling == child._handle) {
            siblingMeta.nextSibling = childMeta.nextSibling;
            childMeta.previousSibling = ObjectPoolHandle::invalidHandle();
            return;
        }
        siblingHead = siblingMeta.nextSibling;
    }
}

ECSEntity EntityComponentStore::getParent(const ECSEntity& entity) {
    if (!isValidEntity(entity)) {
        Engine.logger.error("Invalid entity");
        return ECSEntity::invalid();
    }
    const EntityMeta& meta = getEntityMeta(entity);
    if (meta.parent == ObjectPoolHandle::invalidHandle()) {
        return ECSEntity::invalid();
    }
    const EntityMeta& parentMeta = _entityMetas.get(meta.parent);
    return ECSEntity(this, meta.parent);
};

};  // namespace okay