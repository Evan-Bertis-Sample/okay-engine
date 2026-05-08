#ifndef __LIGHT_COMPONENT_H__
#define __LIGHT_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/util/option.hpp>

namespace okay {

struct LightComponent {
    struct PointLightOptions {
        float radius{1.0f};

        bool operator==(const PointLightOptions& other) const {
            return radius == other.radius;
        }
    };

    struct SpotLightOptions {
        float radius{1.0f};
        float angleRad{glm::radians(45.0f)};
        bool operator==(const SpotLightOptions& other) const {
            return radius == other.radius && angleRad == other.angleRad;
        }
    };

    // Implied by the transform!
    struct DirectionalLightOptions {
        bool operator==(const DirectionalLightOptions& other) const {
            return true;
        }
    };

    std::variant<PointLightOptions, SpotLightOptions, DirectionalLightOptions> options{
        PointLightOptions{}};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    Option<std::size_t> lightID;

    static LightComponent directional(glm::vec3 rgb, float intensity = 1.0f) {
        LightComponent lc{};
        lc.options = DirectionalLightOptions{};
        lc.color = rgb;
        lc.intensity = intensity;
        return lc;
    }

    static LightComponent point(glm::vec3 rgb, float intensity = 1.0f, float radius = 1.0f) {
        LightComponent lc{};
        lc.options = PointLightOptions{radius};
        lc.color = rgb;
        lc.intensity = intensity;
        return lc;
    }

    static LightComponent spot(glm::vec3 rgb,
        float intensity = 1.0f,
        float radius = 1.0f,
        float angleRad = glm::radians(45.0f)) {
        LightComponent lc{};
        lc.options = SpotLightOptions{radius, angleRad};
        lc.color = rgb;
        lc.intensity = intensity;
        return lc;
    }

    Light::Type type() const {
        if (std::holds_alternative<PointLightOptions>(options)) {
            return Light::Type::POINT;
        } else if (std::holds_alternative<SpotLightOptions>(options)) {
            return Light::Type::SPOT;
        } else {
            return Light::Type::DIRECTIONAL;
        }
    }

    bool operator==(const LightComponent& other) const {
        return options == other.options && color == other.color && intensity == other.intensity;
    }

    bool operator!=(const LightComponent& other) const {
        return !(*this == other);
    }
};

};  // namespace okay

#endif  // __LIGHT_COMPONENT_H__