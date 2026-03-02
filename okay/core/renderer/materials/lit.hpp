#ifndef __LIT_H__
#define __LIT_H__

#include <glm/glm.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/okay_render_world.hpp>

namespace okay {

template <std::size_t MaxLights>
struct LightBlock {
    glm::vec4 meta;
    std::array<OkayLight, MaxLights> lights;
};

using DefaultLightBlock = LightBlock<16>;

struct LitMaterial : public BaseMatricesProps, public OkayMaterialProperties<LitMaterial> {
    TemplatedUniformBlock<DefaultLightBlock, FixedString("u_lights")> lights{};

    auto uniformRefs() {
        return BaseMatricesProps::uniformRefs();
    }
    auto uniformRefs() const {
        return BaseMatricesProps::uniformRefs();
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