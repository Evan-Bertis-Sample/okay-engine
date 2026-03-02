#ifndef __LIT_H__
#define __LIT_H__

#include <glm/glm.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/okay_render_world.hpp>
#include "okay/core/util/type.hpp"

namespace okay {

template <std::size_t MaxLights>
struct LightBlock {
    glm::vec4 meta;
    std::array<OkayLight, MaxLights> lights;
};

using DefaultLightBlock = LightBlock<16>;

struct LitMaterial : public BaseMatricesProps, public OkayMaterialProperties<LitMaterial> {
    TemplatedUniformBlock<DefaultLightBlock, FixedString("u_lights")> lights{};
    TemplatedMaterialUniform<float, FixedString("u_shininess")> shininess{32.0f};
    TemplatedMaterialUniform<float, FixedString("u_ambient")> ambient{0.05f};

    auto uniformRefs() {
        return std::tuple_cat(BaseMatricesProps::uniformRefs(), std::tie(shininess, ambient));
    }
    auto uniformRefs() const {
        return std::tuple_cat(BaseMatricesProps::uniformRefs(), std::tie(shininess, ambient));
    }

    auto uniformBlockRefs() {
        return std::tie(lights);
    }
    auto uniformBlockRefs() const {
        return std::tie(lights);
    }
};

}  // namespace okay

#endif  // __LIT_H__