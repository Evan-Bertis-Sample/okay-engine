#ifndef __LIGHT_SYSTEM_H__
#define __LIGHT_SYSTEM_H__

#include <okay/core/ecs/components/light_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

namespace okay {

class LightSystem : public ECSSystem<query::Get<TransformComponent, LightComponent>> {
   public:
};

}  // namespace okay

#endif  // __LIGHT_SYSTEM_H__