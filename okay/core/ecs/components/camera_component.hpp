#ifndef __CAMERA_COMPONENT_H__
#define __CAMERA_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

namespace okay {

struct CameraComponent {
    Camera camera;
    CameraComponent(const Camera& camera = Camera{}) : camera(camera) {}

    bool operator==(const CameraComponent& other) const { return camera == other.camera; }
    bool operator!=(const CameraComponent& other) const { return !(*this == other); }
};

}  // namespace okay

#endif  // __CAMERA_COMPONENT_H__