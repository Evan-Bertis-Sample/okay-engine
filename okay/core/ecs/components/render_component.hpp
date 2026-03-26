#ifndef __RENDER_COMPONENT_H__
#define __RENDER_COMPONENT_H__

#include <glm/glm.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/renderer.hpp>

namespace okay {

struct RenderComponent {
   public:
    OkayMesh mesh{OkayMesh::none()};
    OkayMaterialHandle material{OkayMaterialHandle::none()};
};

}  // namespace okay

#endif  // __RENDER_COMPONENT_H__