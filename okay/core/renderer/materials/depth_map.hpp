#ifndef __DEPTH_MAP_H__
#define __DEPTH_MAP_H__


#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/type.hpp>
#include <glm/glm.hpp>

namespace okay {


using DefaultLightBlock = LightBlock<16>;

struct DepthMapMaterial : public SceneMaterialProperties, public OkayMaterialProperties<DepthMapMaterial> {
    
    UniformProperty<glm::mat4, FixedString("u_lightSpaceMatrix")> lightSpaceMatrix{};
    auto uniformRefs() {
        return std::tie(lightSpaceMatrix);
    }
    auto uniformRefs() const {
        return std::tie(lightSpaceMatrix);
    }

    auto uniformBlockRefs() { return std::tie(); }
    auto uniformBlockRefs() const { return std::tie(); }

    auto textureRefs() { return std::tie(); }
    auto textureRefs() const { return std::tie(); }
    
};



}  // namespace okay

#endif  // __DEPTH_MAP_H__