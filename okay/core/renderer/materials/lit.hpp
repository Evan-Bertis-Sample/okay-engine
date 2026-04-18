#ifndef __LIT_H__
#define __LIT_H__

#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/type.hpp>
#include "glm/detail/qualifier.hpp"

#include <glm/glm.hpp>

namespace okay {

template <std::size_t MaxLights>
struct alignas(16) LightBlock {
    glm::vec4 meta;
    std::array<Light, MaxLights> lights;
};

using DefaultLightBlock = LightBlock<16>;

struct LitMaterial : public SceneMaterialProperties, public OkayMaterialProperties<LitMaterial> {
    BlockProperty<DefaultLightBlock, FixedString("u_lights")> lights{};

    LitMaterial() { lights.setBindingPoint(0); }
    UniformProperty<float, FixedString("u_ambient")> ambient{0.05f};
    TextureProperty<FixedString("u_albedo")> albedo;
<<<<<<< HEAD
    UniformProperty<glm::vec3, FixedString("u_color")> color{glm::vec3(1.0f)};
    UniformProperty<float, FixedString("u_metallic")> metallic{0.0f};
    UniformProperty<float, FixedString("u_specular")> specular{0.5f};
    UniformProperty<float, FixedString("u_specularTint")> specularTint{0.0f};
    UniformProperty<float, FixedString("u_roughness")> roughness{0.5f};
    UniformProperty<float, FixedString("u_anisotropic")> anisotropic{0.0f};
    UniformProperty<float, FixedString("u_sheen")> sheen{0.0};
    UniformProperty<float, FixedString("u_sheenTint")> sheenTint{0.5f};
    UniformProperty<float, FixedString("u_clearcoat")> clearcoat{0.0f};
    UniformProperty<float, FixedString("u_clearcoatGloss")> clearcoatGloss{1.0f};
    UniformProperty<float, FixedString("u_specularTrans")> specularTrans{0.0f};
    UniformProperty<float, FixedString("u_flatness")> flatness{0.0f};
    UniformProperty<int, FixedString("u_thin")> thin{0};
=======
    TextureProperty<FixedString("u_shadowMap")> shadowMap;
    TextureProperty<FixedString("u_shadowMap")> shadowMap;
    UniformProperty<glm::vec3, FixedString("u_color")> color{};
>>>>>>> 6c7a2b3 (added shadow-mapping pass 1)

    auto uniformRefs() {
        return std::tuple_cat(SceneMaterialProperties::uniformRefs(), std::tie(
            ambient, color, metallic, specular, specularTint, roughness, anisotropic, 
            sheen, sheenTint, clearcoat, clearcoatGloss, specularTrans, flatness, thin
        ));
    }
    auto uniformRefs() const {
        return std::tuple_cat(SceneMaterialProperties::uniformRefs(), std::tie(
            ambient, color, metallic, specular, specularTint, roughness, anisotropic, 
            sheen, sheenTint, clearcoat, clearcoatGloss, specularTrans, flatness, thin
        ));
    }

    auto uniformBlockRefs() { return std::tie(lights); }
    auto uniformBlockRefs() const { return std::tie(lights); }

    auto textureRefs() { return std::tie(albedo, shadowMap); }
    auto textureRefs() const { return std::tie(albedo, shadowMap); }

    MaterialFlagCollection flags() {
        MaterialFlagCollection flags = SceneMaterialProperties::flags();
        return flags;
    }
};

static_assert(sizeof(okay::Light) == 64, "OkayLight must be 64 bytes (4 vec4s)");
static_assert(alignof(okay::Light) == 16, "OkayLight must be 16-byte aligned");

static_assert(sizeof(okay::DefaultLightBlock) == 16 + 16 * 64,
              "DefaultLightBlock must be 1040 bytes (vec4 + 16 Lights)");
static_assert(alignof(okay::DefaultLightBlock) == 16, "DefaultLightBlock must be 16-byte aligned");

}  // namespace okay

#endif  // __LIT_H__
