#ifndef __LIGHT_COMPONENT_H__
#define __LIGHT_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/util/option.hpp>

namespace okay {

struct LightComponent {
    Light::Type type{Light::Type::POINT};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};

    struct PointLightOptions {
        float radius{1.0f};
    };

    struct SpotLightOptions {
        float radius{1.0f};
        float angleRad{glm::radians(45.0f)};
    };

    union {
        PointLightOptions pointOptions;
        SpotLightOptions spotOptions;
    };

    Option<std::size_t> lightID;

    bool operator==(const LightComponent& other) const { return light == other.light; }
    bool operator!=(const LightComponent& other) const { return !(*this == other); }
};

};  // namespace okay

#endif  // __LIGHT_COMPONENT_H__