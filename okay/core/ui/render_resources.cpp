#include "render_resources.hpp"

namespace okay {

Option<MaterialHandle> UIRenderResoruces::getRectMaterial(const UIElement& element) {
    if (element.backgroundMaterialOverride.isValid())
        return Option<MaterialHandle>::some(element.backgroundMaterialOverride);

    Option<RectMaterialKey> key = getRectMaterialKey(element);

    if (key.isNone())
        return Option<MaterialHandle>::none();

    if (_rectMaterialCache.contains(*key)) {
        return Option<MaterialHandle>::some(_rectMaterialCache.at(*key));
    }
    Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();

    // create a material
    auto materialProperties = std::make_unique<RectMaterial>();
    materialProperties->color = element.backgroundColor;
    materialProperties->albedo = getElementTexture(element);
    materialProperties->borderColor = element.borderColor;
    materialProperties->clipMask = getElementClipMask(element);

    // flags
    materialProperties->isTransparent = true;
    materialProperties->useScreenspaceCoords = true;
    materialProperties->castsShadows = false;
    materialProperties->recievesShadows = false;

    MaterialHandle handle =
        renderer->materialRegistry().registerMaterial(_uiRectShader, std::move(materialProperties));

    // Engine.logger.debug("Creating new material for UI element with background image");

    _rectMaterialCache[*key] = handle;
    return Option<MaterialHandle>::some(handle);
}

Option<MaterialHandle> UIRenderResoruces::getTextMaterial(const UIElement& element) {
    if (element.textMaterialOverride.isValid())
        return Option<MaterialHandle>::some(element.textMaterialOverride);

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
    materialProperties->pxRange = static_cast<float>(FontManager::SDF_SPREAD_PX);
    materialProperties->clipMask = getElementClipMask(element);

    Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
    MaterialHandle handle =
        renderer->materialRegistry().registerMaterial(_uiTextShader, std::move(materialProperties));

    // Engine.logger.debug("Creating new material for UI element with text {}",
    // element.text.value());

    _textMaterialCache[*key] = handle;
    return Option<MaterialHandle>::some(handle);
}

bool UIRenderResoruces::TextMaterialKey::operator==(const TextMaterialKey& other) const {
    return font == other.font && color == other.color &&
           atlasTextureWidth == other.atlasTextureWidth && clipMask == other.clipMask;
}

bool UIRenderResoruces::TextMaterialKey::operator!=(const TextMaterialKey& other) const {
    return !(*this == other);
}

bool UIRenderResoruces::TextMaterialKey::operator<(const TextMaterialKey& other) const {
    return font < other.font ||
           (font == other.font && glm::length(color) < glm::length(other.color)) ||
           (font == other.font && color == other.color &&
               atlasTextureWidth < other.atlasTextureWidth && clipMask < other.clipMask);
}

bool UIRenderResoruces::RectMaterialKey::operator==(const RectMaterialKey& o) const {
    return texture == o.texture && color == o.color && borderRadius == o.borderRadius &&
           borderWidth == o.borderWidth && borderColor == o.borderColor && clipMask == o.clipMask;
}

bool UIRenderResoruces::RectMaterialKey::operator!=(const RectMaterialKey& o) const {
    return !(*this == o);
}

bool UIRenderResoruces::RectMaterialKey::operator<(const RectMaterialKey& o) const {
    if (texture != o.texture)
        return texture < o.texture;

    if (color.r != o.color.r)
        return color.r < o.color.r;
    if (color.g != o.color.g)
        return color.g < o.color.g;
    if (color.b != o.color.b)
        return color.b < o.color.b;
    if (color.a != o.color.a)
        return color.a < o.color.a;

    if (borderRadius.x != o.borderRadius.x)
        return borderRadius.x < o.borderRadius.x;
    if (borderRadius.y != o.borderRadius.y)
        return borderRadius.y < o.borderRadius.y;

    if (borderWidth.x != o.borderWidth.x)
        return borderWidth.x < o.borderWidth.x;
    if (borderWidth.y != o.borderWidth.y)
        return borderWidth.y < o.borderWidth.y;

    if (borderColor.r != o.borderColor.r)
        return borderColor.r < o.borderColor.r;
    if (borderColor.g != o.borderColor.g)
        return borderColor.g < o.borderColor.g;
    if (borderColor.b != o.borderColor.b)
        return borderColor.b < o.borderColor.b;
    if (borderColor.a != o.borderColor.a)
        return borderColor.a < o.borderColor.a;

    return false;
}

void UIRenderResoruces::loadResoruces() {
    AssetManager* am = Engine.systems.getSystemChecked<AssetManager>();

    _whiteTexture =
        unwrapAssetResult(am->loadEngineAssetSync<Texture>(WHITE_TEXTURE, TextureLoadSettings{}));

    Renderer* renderer = Engine.systems.getSystemChecked<Renderer>();
    Shader textShader = unwrapAssetResult(am->loadEngineAssetSync<Shader>(UI_TEXT_SHADER));
    _uiTextShader = renderer->materialRegistry().registerShader(
        textShader.vertexShader, textShader.fragmentShader);

    Shader rectShader = unwrapAssetResult(am->loadEngineAssetSync<Shader>(UI_RECT_SHADER));
    _uiRectShader = renderer->materialRegistry().registerShader(
        rectShader.vertexShader, rectShader.fragmentShader);

    _quadMesh = renderer->meshBuffer().addMesh(

        primitives::rect().sizeSet(glm::vec2(1.0f, 1.0f)).twoSidedSet(true).build());
}

Option<UIRenderResoruces::TextMaterialKey> UIRenderResoruces::getTextMaterialKey(
    const UIElement& element) const {
    if (!element.text.isSome()) {
        return Option<TextMaterialKey>::none();
    }

    TextStyle style = element.textStyle;
    // compute the texture width
    // we will do this by bucketing the target height
    const float PX_PER_UNIT = 64.0f;  // number of pixels per unit of height
    const int TARGET_WIDTH = static_cast<int>(style.fontHeight) * PX_PER_UNIT;

    TextMaterialKey key = {
        .font = style.font,
        .color = element.textColor,
        .atlasTextureWidth = static_cast<int>(TARGET_WIDTH),
    };

    return Option<TextMaterialKey>::some(key);
}

Option<UIRenderResoruces::RectMaterialKey> UIRenderResoruces::getRectMaterialKey(
    const UIElement& element) const {
    if (element.backgroundColor.a < 0.0001f) {
        return Option<RectMaterialKey>::none();
    }

    RectMaterialKey key = {
        .texture = getElementTexture(element),
        .color = element.backgroundColor,
        .borderRadius = glm::vec2(element.borderRadius.pixels),
        .borderWidth = glm::vec2(element.borderWidth.pixels),
        .borderColor = element.borderColor,
    };
    return Option<RectMaterialKey>::some(key);
}

Texture UIRenderResoruces::getElementTexture(const UIElement& element) const {
    if (element.backgroundImage.isSome()) {
        return element.backgroundImage.value();
    }

    // Engine.logger.debug("Using white texture for UI element with no background image");
    return _whiteTexture;
}

Texture UIRenderResoruces::getElementClipMask(const UIElement& element) const {
    if (element.clipMask.isSome()) {
        return element.clipMask.value();
    }
    return _whiteTexture;
}

}  // namespace okay
