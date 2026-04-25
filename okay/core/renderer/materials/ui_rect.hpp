#ifndef __UI_RECT_H__
#define __UI_RECT_H__

#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/uniform.hpp>

namespace okay {

struct UIRectMaterial : public SceneMaterialProperties,
                        public OkayMaterialProperties<UIRectMaterial> {
    UniformProperty<glm::vec4, FixedString("u_color")> color{glm::vec4(1.0f)};
    TextureProperty<FixedString("u_albedo")> albedo;

    UniformProperty<glm::vec2, FixedString("u_borderRadius")> borderRadius{glm::vec2(0.0f)};
    UniformProperty<glm::vec2, FixedString("u_borderWidth")> borderWidth{glm::vec2(0.0f)};
    UniformProperty<glm::vec4, FixedString("u_borderColor")> borderColor{glm::vec4(0.0f)};

    UniformProperty<glm::vec2, FixedString("u_clipSpaceTL")> clipSpaceTL{glm::vec2(-1.0f, 1.0f)};
    UniformProperty<glm::vec2, FixedString("u_clipSpaceBR")> clipSpaceBR{glm::vec2(1.0f, -1.0f)};

    auto uniformRefs() const {
        return std::tuple_cat(
            SceneMaterialProperties::uniformRefs(),
            std::tie(color, borderRadius, borderWidth, borderColor, clipSpaceTL, clipSpaceBR));
    }

    auto uniformRefs() {
        return std::tuple_cat(
            SceneMaterialProperties::uniformRefs(),
            std::tie(color, borderRadius, borderWidth, borderColor, clipSpaceTL, clipSpaceBR));
    }

    auto uniformBlockRefs() { return std::tie(); }

    auto uniformBlockRefs() const { return std::tie(); }

    auto textureRefs() { return std::tie(albedo); }

    auto textureRefs() const { return std::tie(albedo); };

    MaterialFlagCollection flags() {
        MaterialFlagCollection flags = SceneMaterialProperties::flags();
        flags.addFlag(MaterialFlags::UNLIT);
        return flags;
    }
};

}  // namespace okay

#endif  // __UI_RECT_H__