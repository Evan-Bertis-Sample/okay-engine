#ifndef __LIGHT_SYSTEM_H__
#define __LIGHT_SYSTEM_H__

#include <okay/core/ecs/components/light_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

namespace okay {

class LightSystem : public ECSSystem<query::Get<TransformComponent, LightComponent>> {
   public:
    void onEntityAdded(QueryT::Item& item) override {
        auto& [transform, light] = item.components;
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

        Light worldLight = createLightFromComponent(item.entity, transform, light);
        std::size_t id = renderer->world().addLight(worldLight);
        light.lightID = id;
    };

    void onPreTick(QueryT::Item& item) override {
        auto& [transform, light] = item.components;
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

        if (!light.lightID) {
            Engine.logger.error("LightSystem: Entity {} has invalid light ID", item.entity.id());
            Engine.shutdown();
            return;
        }

        Light worldLight = createLightFromComponent(item.entity, transform, light);
        Light& rendererLight = renderer->world().getLight(light.lightID.value());
        rendererLight = worldLight;
    };

    void onEntityRemoved(QueryT::Item& item) override {
        auto& [transform, light] = item.components;
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

        if (!light.lightID) {
            Engine.logger.error("LightSystem: Entity {} has invalid light ID", item.entity.id());
            Engine.shutdown();
            return;
        }

        renderer->world().removeLight(light.lightID.value());
    };

   private:
    Light createLightFromComponent(ECSEntity& entity,
                                   const TransformComponent& transform,
                                   const LightComponent& light) {
        switch (light.type) {
            case Light::Type::DIRECTIONAL:
                return Light::directional(transform.forward(entity), light.color, light.intensity);
            case Light::Type::POINT:
                return Light::point(transform.getWorldPosition(entity),
                                    light.pointOptions.radius,
                                    light.color,
                                    light.intensity);
            case Light::Type::SPOT:
                return Light::spot(transform.getWorldPosition(entity),
                                   transform.forward(entity),
                                   light.spotOptions.radius,
                                   light.spotOptions.angleRad,
                                   light.color,
                                   light.intensity);
            default:
                return Light::directional(transform.forward(entity), light.color, light.intensity);
        }
    }
};

}  // namespace okay

#endif  // __LIGHT_SYSTEM_H__