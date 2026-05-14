#ifndef __RENDER_RESOURCES_H__
#define __RENDER_RESOURCES_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/text_sdf.hpp>
#include <okay/core/renderer/materials/ui_rect.hpp>
#include <okay/core/renderer/primitive.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/ui/text_layout.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/option.hpp>

#include <string_view>

namespace okay {

class UIRenderResoruces {
   public:
    using RectMaterial = UIRectMaterial;
    using TextMaterial = TextSDFMaterial;

    static UIRenderResoruces& get() {
        static UIRenderResoruces instance;
        return instance;
    }

    UIRenderResoruces() {
        loadResoruces();
    }

    Option<MaterialHandle> getRectMaterial(const UIElement& element);

    Option<MaterialHandle> getTextMaterial(const UIElement& element);

    static constexpr std::string_view UI_TEXT_SHADER = "shaders/text_sdf";
    static constexpr std::string_view UI_RECT_SHADER = "shaders/ui_rect";
    static constexpr std::string_view WHITE_TEXTURE = "textures/white.jpg";

    struct TextMaterialKey {
        FontManager::FontHandle font;
        glm::vec3 color;
        int atlasTextureWidth;
        Texture clipMask;

        bool operator==(const TextMaterialKey& other) const;

        bool operator!=(const TextMaterialKey& other) const;

        bool operator<(const TextMaterialKey& other) const;
    };

    struct RectMaterialKey {
        Texture texture;
        Texture clipMask;
        glm::vec4 color;
        glm::vec2 borderRadius;
        glm::vec2 borderWidth;
        glm::vec4 borderColor;

        bool operator==(const RectMaterialKey& o) const;

        bool operator!=(const RectMaterialKey& o) const;

        bool operator<(const RectMaterialKey& o) const;
    };

    Mesh quadMesh() const {
        return _quadMesh;
    }
    Texture whiteTexture() const {
        return _whiteTexture;
    }

   private:
    std::map<TextMaterialKey, MaterialHandle> _textMaterialCache;
    std::map<RectMaterialKey, MaterialHandle> _rectMaterialCache;

    ShaderHandle _uiTextShader;
    ShaderHandle _uiRectShader;

    Texture _whiteTexture;
    Mesh _quadMesh;

    void loadResoruces();

    Option<TextMaterialKey> getTextMaterialKey(const UIElement& element) const;
    Option<RectMaterialKey> getRectMaterialKey(const UIElement& element) const;
    Texture getElementTexture(const UIElement& element) const;
    Texture getElementClipMask(const UIElement& element) const;
};

};  // namespace okay

#endif  // __RENDER_RESOURCES_H__
