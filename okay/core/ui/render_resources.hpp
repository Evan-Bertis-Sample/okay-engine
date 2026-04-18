#ifndef __RENDER_RESOURCES_H__
#define __RENDER_RESOURCES_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/primitive.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/ui/text_layout.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/option.hpp>

namespace okay {

class UIRenderResoruces {
   public:
    static UIRenderResoruces& get() {
        static UIRenderResoruces instance;
        return instance;
    }

    UIRenderResoruces() { loadResoruces(); }

    Option<MaterialHandle> getMaterial(const UIElement& element) {
        if (Option<TextMaterialKey> key = getTextMaterialKey(element)) {
            Engine.logger.debug("Getting material for UI element with text {}",
                                element.text.value());

            if (_textMaterialCache.contains(*key)) {
                return Option<MaterialHandle>::some(_textMaterialCache.at(*key));
            }

            TextStyle style = element.textStyle;

            // create a material
            auto materialProperties = std::make_unique<UnlitMaterial>();
            materialProperties->color = element.textColor;
            materialProperties->albedo = FontManager::instance().getGlyphAtlas(style.font);
            materialProperties->isTransparent = true;
            materialProperties->useScreenspaceCoords = true;
            materialProperties->castsShadows = false;
            materialProperties->recievesShadows = false;

            Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
            MaterialHandle handle = renderer->materialRegistry().registerMaterial(
                _uiElementShader, std::move(materialProperties));

            Engine.logger.debug("Creating new material for UI element with text {}",
                                element.text.value());

            _textMaterialCache[*key] = handle;
            return Option<MaterialHandle>::some(handle);
        }

        if (Option<TextureMaterialKey> key = getTextureMaterialKey(element)) {
            if (_textureMaterialCache.contains(*key)) {
                return Option<MaterialHandle>::some(_textureMaterialCache.at(*key));
            }

            // create a material
            auto materialProperties = std::make_unique<UnlitMaterial>();
            materialProperties->color = element.backgroundColor;
            materialProperties->albedo = element.backgroundImage.value();
            materialProperties->isTransparent = true;
            materialProperties->useScreenspaceCoords = true;
            materialProperties->castsShadows = false;
            materialProperties->recievesShadows = false;

            Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
            MaterialHandle handle = renderer->materialRegistry().registerMaterial(
                _uiElementShader, std::move(materialProperties));

            Engine.logger.debug("Creating new material for UI element with background image");

            _textureMaterialCache[*key] = handle;
            return Option<MaterialHandle>::some(handle);
        }

        return Option<MaterialHandle>::none();
    };

    static constexpr std::string_view UI_ELEMENT_SHADER = "shaders/unlit";
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

    struct TextureMaterialKey {
        Texture texture;
        glm::vec3 color;

        bool operator==(const TextureMaterialKey& other) const {
            return texture == other.texture && color == other.color;
        }

        bool operator!=(const TextureMaterialKey& other) const { return !(*this == other); }

        bool operator<(const TextureMaterialKey& other) const {
            return texture < other.texture ||
                   (texture == other.texture && glm::length(color) < glm::length(other.color));
        }
    };

    Mesh quadMesh() const { return _quadMesh; }
    Texture whiteTexture() const { return _whiteTexture; }

   private:
    std::map<TextMaterialKey, MaterialHandle> _textMaterialCache;
    std::map<TextureMaterialKey, MaterialHandle> _textureMaterialCache;

    ShaderHandle _uiElementShader;
    Texture _whiteTexture;
    Mesh _quadMesh;

    void loadResoruces() {
        AssetManager* am = Engine.systems.getSystemChecked<AssetManager>();

        _whiteTexture = unwrapAssetResult(
            am->loadEngineAssetSync<Texture>(WHITE_TEXTURE, TextureLoadSettings{}));

        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
        Shader shader = unwrapAssetResult(am->loadEngineAssetSync<Shader>(UI_ELEMENT_SHADER));
        _uiElementShader =
            renderer->materialRegistry().registerShader(shader.vertexShader, shader.fragmentShader);

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

    Option<TextureMaterialKey> getTextureMaterialKey(const UIElement& element) const {
        if (!element.backgroundImage.isSome()) {
            return Option<TextureMaterialKey>::none();
        }
        TextureMaterialKey key = {
            .texture = element.backgroundImage.value(),
            .color = element.backgroundColor,
        };
        return Option<TextureMaterialKey>::some(key);
    }
};

};  // namespace okay

#endif  // __RENDER_RESOURCES_H__