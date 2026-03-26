#ifndef __RENDER_COMPONENT_H__
#define __RENDER_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/renderer.hpp>

#include <glm/glm.hpp>

namespace okay {

struct RenderComponent {
   public:
    Mesh mesh{Mesh::none()};
    MaterialHandle material{MaterialHandle::none()};
};

}  // namespace okay

#endif  // __RENDER_COMPONENT_H__