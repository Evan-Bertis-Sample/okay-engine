#ifndef __TEXT_SDF_H__
#define __TEXT_SDF_H__

#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/uniform.hpp>

namespace okay {

struct TextSDFProperties : public SceneMaterialProperties {
    UniformProperty<glm::vec4, FixedString("u_color")> color{glm::vec4(1.0f)};
    TextureProperty<FixedString("u_albedo")> albedo;
    UniformProperty<float, FixedString("u_pxRange")> pxRange{32.0};
    UniformProperty<glm::vec2, FixedString("u_clipSpaceTL")> clipSpaceTL{glm::vec2(-1.0f, 1.0f)};
    UniformProperty<glm::vec2, FixedString("u_clipSpaceBR")> clipSpaceBR{glm::vec2(1.0f, -1.0f)};

    auto uniformRefs() const {
        return std::tuple_cat(SceneMaterialProperties::uniformRefs(),
            std::tie(color, pxRange, clipSpaceTL, clipSpaceBR));
    }

    auto uniformRefs() {
        return std::tuple_cat(SceneMaterialProperties::uniformRefs(),
            std::tie(color, pxRange, clipSpaceTL, clipSpaceBR));
    }

    auto uniformBlockRefs() {
        return std::tie();
    }

    auto uniformBlockRefs() const {
        return std::tie();
    }

    auto textureRefs() {
        return std::tie(albedo);
    }

    auto textureRefs() const {
        return std::tie(albedo);
    };
};

struct TextSDFMaterial : public TextSDFProperties, public OkayMaterialProperties<TextSDFMaterial> {
    auto uniformRefs() {
        return TextSDFProperties::uniformRefs();
    }

    auto uniformRefs() const {
        return TextSDFProperties::uniformRefs();
    }

    auto uniformBlockRefs() {
        return TextSDFProperties::uniformBlockRefs();
    }

    auto uniformBlockRefs() const {
        return TextSDFProperties::uniformBlockRefs();
    }

    auto textureRefs() {
        return TextSDFProperties::textureRefs();
    }

    auto textureRefs() const {
        return TextSDFProperties::textureRefs();
    }

    MaterialFlagCollection flags() {
        MaterialFlagCollection flags = SceneMaterialProperties::flags();
        flags.addFlag(MaterialFlags::UNLIT);
        return flags;
    }
};

}  // namespace okay

#endif  // __TEXT_SDF_H__
