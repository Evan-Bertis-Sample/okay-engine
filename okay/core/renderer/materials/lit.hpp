#ifndef __LIT_H__
#define __LIT_H__

#include <glm/glm.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/okay_render_world.hpp>
#include "okay/core/util/type.hpp"

namespace okay {

template <std::size_t MaxLights>
struct alignas(16) LightBlock {
    glm::vec4 meta;
    std::array<OkayLight, MaxLights> lights;
};

using DefaultLightBlock = LightBlock<16>;

struct LitMaterial : public BaseMatricesProps, public OkayMaterialProperties<LitMaterial> {
    BlockProperty<DefaultLightBlock, FixedString("u_lights")> lights{};
    UniformProperty<float, FixedString("u_shininess")> shininess{8.0f};
    UniformProperty<float, FixedString("u_ambient")> ambient{0.05f};
    TextureProperty<FixedString("u_albedo")> albedo;

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

    auto textureRefs() {
        return std::tie(albedo);
    }
    auto textureRefs() const {
        return std::tie(albedo);
    }
};

static_assert(sizeof(okay::OkayLight) == 64, "OkayLight must be 64 bytes (4 vec4s)");
static_assert(alignof(okay::OkayLight) == 16, "OkayLight must be 16-byte aligned");

static_assert(sizeof(okay::DefaultLightBlock) == 16 + 16 * 64,
              "DefaultLightBlock must be 1040 bytes (vec4 + 16 Lights)");
static_assert(alignof(okay::DefaultLightBlock) == 16, "DefaultLightBlock must be 16-byte aligned");

}  // namespace okay

#endif  // __LIT_H__