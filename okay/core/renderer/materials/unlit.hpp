#ifndef __UNLIT_H__
#define __UNLIT_H__

#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/uniform.hpp>

#include <glm/glm.hpp>

namespace okay {

struct SceneMaterialProperties {
    UniformProperty<glm::mat4, FixedString("u_modelMatrix")> modelMatrix{};
    UniformProperty<glm::mat4, FixedString("u_viewMatrix")> viewMatrix{};
    UniformProperty<glm::mat4, FixedString("u_projectionMatrix")> projectionMatrix{};
    UniformProperty<glm::vec3, FixedString("u_cameraPosition")> cameraPosition{};
    UniformProperty<glm::vec3, FixedString("u_cameraDirection")> cameraDirection{};

    bool isTransparent{false};
    bool recievesShadows{false};
    bool castsShadows{false};

    auto uniformRefs() {
        return std::tie(modelMatrix, viewMatrix, projectionMatrix, cameraPosition, cameraDirection);
    }
    auto uniformRefs() const {
        return std::tie(modelMatrix, viewMatrix, projectionMatrix, cameraPosition, cameraDirection);
    }

    auto uniformBlockRefs() { return std::tie(); }
    auto uniformBlockRefs() const { return std::tie(); }

    auto textureRefs() { return std::tie(); }
    auto textureRefs() const { return std::tie(); }

    MaterialFlagCollection flags() {
        MaterialFlagCollection flags;
        if (isTransparent)
            flags.addFlag(MaterialFlags::TRANSPARENT);
        if (recievesShadows)
            flags.addFlag(MaterialFlags::RECEIVE_SHADOWS);
        if (castsShadows)
            flags.addFlag(MaterialFlags::CAST_SHADOWS);
        return flags;
    }
};

struct UnlitMaterial : public SceneMaterialProperties,
                       public OkayMaterialProperties<UnlitMaterial> {
    UniformProperty<glm::vec3, FixedString("u_color")> color{};
    TextureProperty<FixedString("u_albedo")> albedo;

    auto uniformRefs() {
        return std::tuple_cat(SceneMaterialProperties::uniformRefs(), std::tie(color));
    }

    auto uniformRefs() const {
        return std::tuple_cat(SceneMaterialProperties::uniformRefs(), std::tie(color));
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

};  // namespace okay

#endif  // __UNLIT_H__