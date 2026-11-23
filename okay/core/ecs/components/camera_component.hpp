#pragma once

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

namespace okay {

struct CameraComponent {
    Camera::Lens lens{Camera::PerspectiveLens{}};

    CameraComponent() {}
    CameraComponent(const Camera::Lens& lens) : lens(lens) {}

    Camera::ProjectionType projectionType() const {
        return std::holds_alternative<Camera::PerspectiveLens>(lens)
                   ? Camera::ProjectionType::PERPSECTIVE
                   : Camera::ProjectionType::ORTHOGRAPHIC;
    }

    bool operator==(const CameraComponent& other) const {
        return lens == other.lens;
    }
    bool operator!=(const CameraComponent& other) const {
        return !(*this == other);
    }
};

}  // namespace okay
