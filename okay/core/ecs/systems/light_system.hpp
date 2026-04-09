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
        glm::vec3 worldPosition = transform.getWorldPosition(entity);

        if (std::holds_alternative<LightComponent::PointLightOptions>(light.options)) {
            const auto& options = std::get<LightComponent::PointLightOptions>(light.options);
            return Light::point(worldPosition, options.radius, light.color, light.intensity);
        } else if (std::holds_alternative<LightComponent::SpotLightOptions>(light.options)) {
            const auto& options = std::get<LightComponent::SpotLightOptions>(light.options);
            return Light::spot(worldPosition,
                               transform.forward(entity),
                               options.radius,
                               options.angleRad,
                               light.color,
                               light.intensity);
        } else if (std::holds_alternative<LightComponent::DirectionalLightOptions>(light.options)) {
            return Light::directional(transform.forward(entity), light.color, light.intensity);
        }

        Engine.logger.error("LightSystem: Entity {} has invalid light options", entity.id());
        return Light::point(worldPosition, 1.0f, light.color, light.intensity);
    }
};

}  // namespace okay

#endif  // __LIGHT_SYSTEM_H__