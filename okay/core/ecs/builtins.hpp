#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <okay/core/ecs/components/camera_component.hpp>
#include <okay/core/ecs/components/light_component.hpp>
#include <okay/core/ecs/components/render_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/systems/camera_system.hpp>
#include <okay/core/ecs/systems/light_system.hpp>
#include <okay/core/ecs/systems/renderer_system.hpp>

namespace okay {

inline void registerBuiltinComponentsAndSystems(ECS& ecs) {
    ecs.registerComponentType<TransformComponent>();
    ecs.registerComponentType<MeshRendererComponent>();
    ecs.registerComponentType<CameraComponent>();
    ecs.registerComponentType<LightComponent>();

    ecs.addSystem(std::make_unique<RendererSystem>());
    ecs.addSystem(std::make_unique<CameraSystem>());
    ecs.addSystem(std::make_unique<LightSystem>());
};

}  // namespace okay

#endif  // __BUILTINS_H__