#ifndef __ECS_UTIL_H__
#define __ECS_UTIL_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/engine/engine.hpp>

namespace okay::ecs {

inline ECSEntity entity(SystemParameter<ECS> ecs = nullptr) {
    return ecs->createEntity();
}

inline ECSEntity entity(ECSEntity& parent, SystemParameter<ECS> ecs = nullptr) {
    return ecs->createEntity(parent);
}

template <typename... QueryArgs>
inline ECS::QueryRange<ECSQuery<QueryArgs...>> query(SystemParameter<ECS> ecs = nullptr) {
    return ecs->query<QueryArgs...>();
}

inline void destroyEntity(ECSEntity entity, SystemParameter<ECS> ecs = nullptr) {
    ecs->destroyEntity(entity);
}

inline std::size_t entityCount(SystemParameter<ECS> ecs = nullptr) {
    return ecs->getEntityCount();
}

};  // namespace okay::ecs

#endif  // __ECS_UTIL_H__