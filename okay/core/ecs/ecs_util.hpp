#ifndef __ECS_UTIL_H__
#define __ECS_UTIL_H__

#include <okay/core/ecs/builtins.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/engine/engine.hpp>

#include <functional>

namespace okay::ecs {

inline void registerBuiltins(SystemParameter<ECS> ecs = nullptr) {
    okay::registerBuiltinComponentsAndSystems(ecs);
}

template <typename T>
inline void registerComponent(SystemParameter<ECS> ecs = nullptr) {
    ecs->registerComponentType<T>();
}

template <typename... Ts>
inline void registerComponents(SystemParameter<ECS> ecs = nullptr) {
    (ecs->registerComponentType<Ts>(), ...);
}

template <typename T>
inline void registerSystem(std::unique_ptr<T> system, SystemParameter<ECS> ecs = nullptr) {
    ecs->addSystem(std::move(system));
}

template <typename... Ts>
inline void registerSystems(std::unique_ptr<Ts>... systems, SystemParameter<ECS> ecs = nullptr) {
    (ecs->addSystem(std::move(systems)), ...);
}

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

inline ECSEntity sceneEntity(
    Transform transform = Transform{}, SystemParameter<ECS> ecs = nullptr) {
    return ecs->createEntity().addComponent<TransformComponent>(transform);
}

inline ECSEntity uiEntity(std::function<UIElement()> uiBuilder,
    std::uint8_t uiLayer = 0,
    SystemParameter<ECS> ecs = nullptr) {
    return ecs->createEntity().addComponent<TransformComponent>().addComponent<UIComponent>(
        uiBuilder, uiLayer);
}

};  // namespace okay::ecs

#endif  // __ECS_UTIL_H__
