#include "font.hpp"

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/asset/generic/font_loader.hpp>

namespace okay {

using FontHandle = FontManager::FontHandle;

FontManager::FontManager() {
    if (FT_Init_FreeType(&_ftLibrary)) {
        throw std::runtime_error("Failed to initialize FreeType library");
    }
};

Option<FontHandle> FontManager::loadFont(
    const std::string& fontPath, const FontLoadOptions& options) {
    if (_fontFaces.find(fontPath) != _fontFaces.end()) {
        return Option<FontHandle>::some(FontHandle{_fontFaces[fontPath]});
    }

    FT_Face face;
    if (FT_New_Face(_ftLibrary, fontPath.c_str(), 0, &face)) {
        return Option<FontHandle>::none();  // Failed to load font
    }

    // Store the face and return a handle
    _loadedFaces.push_back(face);
    std::uint32_t id = static_cast<std::uint32_t>(_loadedFaces.size() - 1);
    _fontFaces[fontPath] = id;
    generateGlyphSetAndAtlasForFace(id, options.width, options.height);
    // Free the face since we don't need it anymore
    FT_Done_Face(face);
    return Option<FontHandle>::some(FontHandle{id});
}

FontManager::Glyph FontManager::getGlyph(FontHandle font, std::uint32_t codepoint) {
    auto& glyphs = getGlyphsForFace(font.id);
    if (glyphs.find(codepoint) != glyphs.end()) {
        return glyphs[codepoint];
    }
    // Return an empty glyph if not found
    return Glyph{codepoint, 0, 0, 0, 0, 0, 0, 0};
}

std::unordered_map<std::uint32_t, FontManager::Glyph>& FontManager::getGlyphsForFace(
    std::uint32_t faceId) {
    if (faceId >= _glyphsPerFace.size()) {
        _glyphsPerFace.resize(faceId + 1);
    }
    return _glyphsPerFace[faceId];
};

void FontManager::generateGlyphSetAndAtlasForFace(std::uint32_t faceId, int width, int height) {
    auto& glyphs = getGlyphsForFace(faceId);
    glyphs.clear();

    FT_Face face = _loadedFaces[faceId];

    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    FT_Set_Pixel_Sizes(face, width, height);

    FT_UInt gindex;
    FT_ULong charcode = FT_Get_First_Char(face, &gindex);

    while (gindex != 0) {
        if (FT_Load_Glyph(face, gindex, FT_LOAD_RENDER)) {
            // Failed to load glyph, skip it
            charcode = FT_Get_Next_Char(face, charcode, &gindex);
            continue;
        }

        FT_GlyphSlot slot = face->glyph;

        int x = 0;
        int y = 0;

        if (!_atlas.addGlyph(slot->bitmap, x, y)) {
            Engine.logger.error("Font atlas overflow");
            break;
        }

        Glyph glyph;
        glyph.codepoint = charcode;
        glyph.w = slot->bitmap.width;
        glyph.h = slot->bitmap.rows;
        glyph.bearingX = slot->bitmap_left;
        glyph.bearingY = slot->bitmap_top;
        glyph.advance = slot->advance.x >> 6;
        glyph.ascent = glyph.bearingY;
        glyph.descent = glyph.h - glyph.bearingY;
        glyph.u0 = (float)x / ATLAS_W;
        glyph.v0 = (float)y / ATLAS_H;
        glyph.u1 = (float)(x + glyph.w) / ATLAS_W;
        glyph.v1 = (float)(y + glyph.h) / ATLAS_H;
        glyphs[charcode] = glyph;

        charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    if (faceId >= _glyphAtlases.size())
        _glyphAtlases.resize(faceId + 1);

    OkayTextureMeta meta;
    meta.width = ATLAS_W;
    meta.height = ATLAS_H;
    meta.channels = 4;
    meta.mipLevels = 1;
    meta.format = OkayTextureMeta::Format::RGBA8;

    auto store = TextureDataStore::mainStore();

    auto handle = store->addTexture(meta, (void*)_atlas.pixels().data(), _atlas.pixels().size());
    _glyphAtlases[faceId] = Texture(store, handle);

    _atlas.reset();  // clear the atlas for the next font face
}

FontManager::FontHandle FontManager::defaultFont() {
    if (_defaultFont.isNone()) {
        Engine.logger.debug("Loading default font");
        AssetManager* am = Engine.systems.getSystemChecked<AssetManager>();
        _defaultFont = Option<FontHandle>::some(
            unwrapAssetResult(am->loadEngineAssetSync<FontManager::FontHandle>(
                "fonts/ARIAL.ttf", FontLoadOptions{})));
    }
    return _defaultFont.value();
}

};  // namespace okay