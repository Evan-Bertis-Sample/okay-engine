#ifndef __UNLIT_H__
#define __UNLIT_H__

#include <glm/glm.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/okay_material.hpp>

namespace okay {

struct BaseMatricesProps {
    UniformProperty<glm::mat4, FixedString("u_modelMatrix")> modelMatrix{};
    UniformProperty<glm::mat4, FixedString("u_viewMatrix")> viewMatrix{};
    UniformProperty<glm::mat4, FixedString("u_projectionMatrix")> projectionMatrix{};
    UniformProperty<glm::vec3, FixedString("u_cameraPosition")> cameraPosition{};
    UniformProperty<glm::vec3, FixedString("u_cameraDirection")> cameraDirection{};
    UniformProperty<glm::vec3, FixedString("u_color")> color{};

    auto uniformRefs() { return std::tie(modelMatrix, viewMatrix, projectionMatrix, cameraPosition, cameraDirection, color); }
    auto uniformRefs() const { return std::tie(modelMatrix, viewMatrix, projectionMatrix, cameraPosition, cameraDirection, color); }

    auto uniformBlockRefs() { return std::tie(); }
    auto uniformBlockRefs() const { return std::tie(); }

    auto textureRefs() { return std::tie(); }
    auto textureRefs() const { return std::tie(); }
};

struct UnlitMaterial : public BaseMatricesProps,
                       public OkayMaterialProperties<UnlitMaterial> {
};

};  // namespace okay

#endif  // __UNLIT_H__