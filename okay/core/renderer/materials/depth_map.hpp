#ifndef __DEPTH_MAP_H__
#define __DEPTH_MAP_H__


#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/type.hpp>
#include <glm/glm.hpp>

namespace okay {




struct DepthMapMaterial : public SceneMaterialProperties, public OkayMaterialProperties<DepthMapMaterial> {
    
    UniformProperty<glm::mat4, FixedString("u_lightSpaceMatrix")> lightSpaceMatrix{};
    UniformProperty<glm::mat4, FixedString("u_modelMatrix")> modelMatrix{};
    auto uniformRefs() {
        return std::tie(lightSpaceMatrix, modelMatrix);
    }
    auto uniformRefs() const {
        return std::tie(lightSpaceMatrix, modelMatrix);
    }

    auto uniformBlockRefs() { return std::tie(); }
    auto uniformBlockRefs() const { return std::tie(); }

    auto textureRefs() { return std::tie(); }
    auto textureRefs() const { return std::tie(); }
    
    MaterialFlagCollection flags() {
        MaterialFlagCollection flags = SceneMaterialProperties::flags();
        return flags;
    }
};



}  // namespace okay

#endif  // __DEPTH_MAP_H__