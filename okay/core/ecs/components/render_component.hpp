#ifndef __RENDER_COMPONENT_H__
#define __RENDER_COMPONENT_H__

#include <glm/glm.hpp>
#include <okay/core/ecs/okay_ecs.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_renderer.hpp>

namespace okay {

struct RenderComponent {
   public:
    OkayMesh mesh{OkayMesh::none()};
    OkayMaterialHandle material{OkayMaterialHandle::none()};
};

}  // namespace okay

#endif  // __RENDER_COMPONENT_H__    