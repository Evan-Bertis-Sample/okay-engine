#ifndef __CAMERA_SYSTEM_H__
#define __CAMERA_SYSTEM_H__

#include <okay/core/ecs/components/camera_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

namespace okay {

class CameraSystem : public ECSSystem<query::Get<TransformComponent, CameraComponent>> {
   public:
};

}  // namespace okay

#endif  // __CAMERA_SYSTEM_H__