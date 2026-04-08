#ifndef __UI_SYSTEM_H__
#define __UI_SYSTEM_H__

#include <okay/core/ecs/components/transform_component.hpp>
#include <okay/core/ecs/components/ui_component.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/renderer.hpp>

namespace okay {

class UISystem : public ECSSystem<query::Get<TransformComponent, UIComponent>> {
   public:
    void onEntityAdded(QueryT::Item& item) override {
        // Engine.logger.debug("RendererSystem: Entity {} added", item.entity.id());
    };

    void onPreTick(QueryT::Item& item) override {

    };

    void onEntityRemoved(QueryT::Item& item) override {};
};

}  // namespace okay

#endif  // __UI_SYSTEM_H__