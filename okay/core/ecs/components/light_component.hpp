#ifndef __LIGHT_COMPONENT_H__
#define __LIGHT_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/util/option.hpp>

namespace okay {

struct LightComponent {
    Light light;
    Option<std::size_t> lightID;

    LightComponent(const Light& light = Light{}) : light(light) {}

    bool operator==(const LightComponent& other) const { return light == other.light; }
    bool operator!=(const LightComponent& other) const { return !(*this == other); }
};

};  // namespace okay

#endif  // __LIGHT_COMPONENT_H__