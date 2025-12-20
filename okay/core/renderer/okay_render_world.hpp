#ifndef __OKAY_RENDER_WORLD_H__
#define __OKAY_RENDER_WORLD_H__

#include <cstdint>
#include <span>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>

#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/dirty_set.hpp>

namespace okay {

struct OkayTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct OkayDrawItem {
    glm::mat4 worldFromMesh{1.0f};
    OkayMesh mesh{};
    IOkayMaterial material{};
    std::uint64_t sortKey{0};
    std::uint32_t entityIndex{0xFFFFFFFFu};
};

}  // namespace okay

#endif  // __OKAY_RENDER_WORLD_H__
