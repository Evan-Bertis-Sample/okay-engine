#ifndef __RENDERER_SYSTEM_H__
#define __RENDERER_SYSTEM_H__

#include "okay/core/renderer/render_world.hpp"
#include "okay/core/renderer/renderer.hpp"

#include <okay/core/ecs/components/camera_component.hpp>
#include <okay/core/ecs/components/light_component.hpp>
#include <okay/core/ecs/components/render_component.hpp>
#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/engine/engine.hpp>

namespace okay {

class RendererSystem : public ECSSystem<query::Get<TransformComponent, RenderComponent>> {
   public:
    void onEntityAdded(QueryT::Item& item) override {
        auto& [transform, render] = item.components;
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
        RenderEntity entity =
            renderer->world().addRenderEntity(transform.transform, render.material, render.mesh);
        render.renderEntity = entity;

        if (item.entity.getParent().isValid()) {
            auto parentRenderComponent = item.entity.getParent().getComponent<RenderComponent>();
            if (parentRenderComponent.isSome()) {
                renderer->world().addChild(parentRenderComponent.value().get().renderEntity,
                                           entity);
            }
        }
    };

    void onPreTick(QueryT::Item& item) override {
        auto& [transform, render] = item.components;
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

        // we do this to avoid temporaries from accessing
        // the fields directly, and resubmission of the dirty entity
        // in the destructor of the property proxy
        RenderEntity::Properties props = render.renderEntity.prop();
        props.transform = transform.transform;
        props.mesh = render.mesh;
        props.material = render.material;

        // Deconsturctor of props will resubmit the entity with the updated properties
    };

    void onEntityRemoved(QueryT::Item& item) override {
        auto& [transform, render] = item.components;
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

        // TODO: implement remove entity in render world
    };
};

}  // namespace okay

#endif  // __RENDERER_SYSTEM_H__