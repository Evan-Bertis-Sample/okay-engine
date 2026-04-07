#ifndef __RENDER_RESOURCES_H__
#define __RENDER_RESOURCES_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
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
            if (_textMaterialCache.contains(*key)) {
                return Option<MaterialHandle>::some(_textMaterialCache.at(*key));
            }

            // create a material
            auto materialProperties = std::make_unique<UnlitMaterial>();
            materialProperties->color = element.textColor;
            materialProperties->albedo =
                FontManager::instance().getGlyphAtlas(element.textStyle.value().font);
            materialProperties->isTransparent = true;
            materialProperties->useProjectionMatrix = false;
            materialProperties->castsShadows = false;
            materialProperties->recievesShadows = false;

            Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
            MaterialHandle handle = renderer->materialRegistry().registerMaterial(
                _uiElementShader, std::move(materialProperties));

            _textMaterialCache[*key] = handle;
            return Option<MaterialHandle>::some(handle);
        }

        if (Option<TextureMaterialKey> key = getTextureMaterialKey(element)) {
            if (_textureMaterialCache.contains(*key)) {
                return Option<MaterialHandle>::some(_textureMaterialCache.at(*key));
            }

            // create a material
            auto materialProperties = std::make_unique<UnlitMaterial>();
            materialProperties->color = element.textColor;
            materialProperties->albedo = element.backgroundImage.value();
            materialProperties->isTransparent = true;
            materialProperties->useProjectionMatrix = false;
            materialProperties->castsShadows = false;
            materialProperties->recievesShadows = false;

            Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
            MaterialHandle handle = renderer->materialRegistry().registerMaterial(
                _uiElementShader, std::move(materialProperties));

            _textureMaterialCache[*key] = handle;
            return Option<MaterialHandle>::some(handle);
        }

        return Option<MaterialHandle>::none();
    };

   private:
    static constexpr std::string_view UI_ELEMENT_SHADER = "shader/ui";
    static constexpr std::string_view WHITE_TEXTURE = "shader/white.jpg";

    struct TextMaterialKey {
        FontManager::FontHandle font;
        glm::vec3 color;
        int atlasTextureWidth;
    };

    struct TextureMaterialKey {
        Texture texture;
        glm::vec3 color;
    };

    std::unordered_map<TextMaterialKey, MaterialHandle> _textMaterialCache;
    std::unordered_map<TextureMaterialKey, MaterialHandle> _textureMaterialCache;

    ShaderHandle _uiElementShader;
    Texture _whiteTexture;

    void loadResoruces() {
        AssetManager* am = Engine.systems.getSystemChecked<AssetManager>();

        _whiteTexture = unwrapAssetResult(am->loadEngineAssetSync<Texture>(WHITE_TEXTURE));

        Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
        Shader shader = unwrapAssetResult(am->loadEngineAssetSync<Shader>(UI_ELEMENT_SHADER));
        _uiElementShader =
            renderer->materialRegistry().registerShader(shader.vertexShader, shader.fragmentShader);
    }

    Option<TextMaterialKey> getTextMaterialKey(const UIElement& element) const {
        if (!element.text.isSome() && !element.textStyle.isSome() &&
            FontManager::instance().isLoadedFont(element.textStyle.value().font)) {
            return Option<TextMaterialKey>::none();
        }

        // compute the texture width
        // we will do this by bucketing the target height
        const float PX_PER_UNIT = 64.0f;  // number of pixels per unit of height
        const int TARGET_WIDTH =
            static_cast<int>(element.textStyle.value().targetFontHeight) * PX_PER_UNIT;

        TextMaterialKey key = {
            .font = element.textStyle.value().font,
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