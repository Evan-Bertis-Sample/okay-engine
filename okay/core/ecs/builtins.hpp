#ifndef __BUILTINS_H__
#define __BUILTINS_H__

// components
#include <okay/core/ecs/components/camera_component.hpp>
#include <okay/core/ecs/components/light_component.hpp>
#include <okay/core/ecs/components/render_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/components/ui_component.hpp>

// systems
#include <okay/core/ecs/systems/camera_system.hpp>
#include <okay/core/ecs/systems/light_system.hpp>
#include <okay/core/ecs/systems/renderer_system.hpp>
#include <okay/core/ecs/systems/ui_system.hpp>
#include <okay/core/engine/engine.hpp>

namespace okay {

inline void registerBuiltinComponentsAndSystems(SystemParameter<ECS> ecs) {
    // components
    ecs->registerComponentType<TransformComponent>();
    ecs->registerComponentType<MeshRendererComponent>();
    ecs->registerComponentType<CameraComponent>();
    ecs->registerComponentType<LightComponent>();
    ecs->registerComponentType<UIComponent>();

    // systems
    ecs->addSystem(std::make_unique<RendererSystem>());
    ecs->addSystem(std::make_unique<CameraSystem>());
    ecs->addSystem(std::make_unique<LightSystem>());
    ecs->addSystem(std::make_unique<UISystem>());
};

}  // namespace okay

#endif  // __BUILTINS_H__