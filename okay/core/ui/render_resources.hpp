#ifndef __RENDER_RESOURCES_H__
#define __RENDER_RESOURCES_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/ui_rect.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
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
    using TextMaterial = UnlitMaterial;

    static UIRenderResoruces& get() {
        static UIRenderResoruces instance;
        return instance;
    }

    UIRenderResoruces() { loadResoruces(); }

    Option<MaterialHandle> getRectMaterial(const UIElement& element) {
        Option<RectMaterialKey> key = getRectMaterialKey(element);

        if (key.isNone())
            return Option<MaterialHandle>::none();

        if (_rectMaterialCache.contains(*key)) {
            return Option<MaterialHandle>::some(_rectMaterialCache.at(*key));
        }
        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

        // create a material
        auto materialProperties = std::make_unique<RectMaterial>();
        materialProperties->color = glm::vec3(
            element.backgroundColor.r, element.backgroundColor.g, element.backgroundColor.b);
        materialProperties->albedo = getElementTexture(element);
        materialProperties->borderColor = element.borderColor;

        // flags
        materialProperties->isTransparent = true;
        materialProperties->useScreenspaceCoords = true;
        materialProperties->castsShadows = false;
        materialProperties->recievesShadows = false;

        MaterialHandle handle = renderer->materialRegistry().registerMaterial(
            _uiRectShader, std::move(materialProperties));

        // Engine.logger.debug("Creating new material for UI element with background image");

        _rectMaterialCache[*key] = handle;
        return Option<MaterialHandle>::some(handle);
    };

    Option<MaterialHandle> getTextMaterial(const UIElement& element) {
        Option<TextMaterialKey> key = getTextMaterialKey(element);

        if (key.isNone())
            return Option<MaterialHandle>::none();

        // Engine.logger.debug("Getting material for UI element with text {}",
        // element.text.value());

        if (_textMaterialCache.contains(*key)) {
            return Option<MaterialHandle>::some(_textMaterialCache.at(*key));
        }

        TextStyle style = element.textStyle;

        // create a material
        auto materialProperties = std::make_unique<TextMaterial>();
        materialProperties->color = element.textColor;
        materialProperties->albedo = FontManager::instance().getGlyphAtlas(style.font);
        materialProperties->isTransparent = true;
        materialProperties->useScreenspaceCoords = true;
        materialProperties->castsShadows = false;
        materialProperties->recievesShadows = false;

        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
        MaterialHandle handle = renderer->materialRegistry().registerMaterial(
            _uiTextShader, std::move(materialProperties));

        // Engine.logger.debug("Creating new material for UI element with text {}",
        // element.text.value());

        _textMaterialCache[*key] = handle;
        return Option<MaterialHandle>::some(handle);
    };

    static constexpr std::string_view UI_TEXT_SHADER = "shaders/unlit";
    static constexpr std::string_view UI_RECT_SHADER = "shaders/ui_rect";
    static constexpr std::string_view WHITE_TEXTURE = "textures/white.jpg";

    struct TextMaterialKey {
        FontManager::FontHandle font;
        glm::vec3 color;
        int atlasTextureWidth;

        bool operator==(const TextMaterialKey& other) const {
            return font == other.font && color == other.color &&
                   atlasTextureWidth == other.atlasTextureWidth;
        }

        bool operator!=(const TextMaterialKey& other) const { return !(*this == other); }

        bool operator<(const TextMaterialKey& other) const {
            return font < other.font ||
                   (font == other.font && glm::length(color) < glm::length(other.color)) ||
                   (font == other.font && color == other.color &&
                    atlasTextureWidth < other.atlasTextureWidth);
        }
    };

    struct RectMaterialKey {
        Texture texture;
        glm::vec4 color;

        bool operator==(const RectMaterialKey& other) const {
            return texture == other.texture && color == other.color &&
                   glm::length(color) == glm::length(other.color);
        }

        bool operator!=(const RectMaterialKey& other) const { return !(*this == other); }

        bool operator<(const RectMaterialKey& other) const {
            return texture < other.texture ||
                   (texture == other.texture && glm::length(color) < glm::length(other.color));
        }
    };

    Mesh quadMesh() const { return _quadMesh; }
    Texture whiteTexture() const { return _whiteTexture; }

   private:
    std::map<TextMaterialKey, MaterialHandle> _textMaterialCache;
    std::map<RectMaterialKey, MaterialHandle> _rectMaterialCache;

    ShaderHandle _uiTextShader;
    ShaderHandle _uiRectShader;

    Texture _whiteTexture;
    Mesh _quadMesh;

    void loadResoruces() {
        AssetManager* am = Engine.systems.getSystemChecked<AssetManager>();

        _whiteTexture = unwrapAssetResult(
            am->loadEngineAssetSync<Texture>(WHITE_TEXTURE, TextureLoadSettings{}));

        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
        Shader textShader = unwrapAssetResult(am->loadEngineAssetSync<Shader>(UI_TEXT_SHADER));
        _uiTextShader = renderer->materialRegistry().registerShader(textShader.vertexShader,
                                                                    textShader.fragmentShader);

        Shader rectShader = unwrapAssetResult(am->loadEngineAssetSync<Shader>(UI_RECT_SHADER));
        _uiRectShader = renderer->materialRegistry().registerShader(rectShader.vertexShader,
                                                                    rectShader.fragmentShader);

        _quadMesh = renderer->meshBuffer().addMesh(
            primitives::rect().sizeSet(glm::vec2(1.0f, 1.0f)).twoSidedSet(true).build());
    }

    Option<TextMaterialKey> getTextMaterialKey(const UIElement& element) const {
        if (!element.text.isSome()) {
            return Option<TextMaterialKey>::none();
        }

        TextStyle style = element.textStyle;
        // compute the texture width
        // we will do this by bucketing the target height
        const float PX_PER_UNIT = 64.0f;  // number of pixels per unit of height
        const int TARGET_WIDTH = static_cast<int>(style.targetFontHeight) * PX_PER_UNIT;

        TextMaterialKey key = {
            .font = style.font,
            .color = element.textColor,
            .atlasTextureWidth = static_cast<int>(TARGET_WIDTH),
        };

        return Option<TextMaterialKey>::some(key);
    }

    Option<RectMaterialKey> getRectMaterialKey(const UIElement& element) const {
        if (element.backgroundColor.a < 0.0001f) {
            return Option<RectMaterialKey>::none();
        }

        RectMaterialKey key = {
            .texture = getElementTexture(element),
            .color = element.backgroundColor,
        };
        return Option<RectMaterialKey>::some(key);
    }

    Texture getElementTexture(const UIElement& element) const {
        if (element.backgroundImage.isSome()) {
            return element.backgroundImage.value();
        }

        // Engine.logger.debug("Using white texture for UI element with no background image");
        return _whiteTexture;
    }
};

};  // namespace okay

#endif  // __RENDER_RESOURCES_H__