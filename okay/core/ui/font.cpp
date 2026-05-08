#include "font.hpp"

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/asset/generic/font_loader.hpp>

#include "freetype/freetype.h"

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

FontManager::FontMetrics& FontManager::getFontMetricsForFace(std::uint32_t faceId) {
    if (faceId >= _metricsPerFace.size()) {
        _metricsPerFace.resize(faceId + 1);
    }
    return _metricsPerFace[faceId];
}

void FontManager::generateGlyphSetAndAtlasForFace(std::uint32_t faceId, int width, int height) {
    auto& glyphs = getGlyphsForFace(faceId);
    glyphs.clear();

    FT_Face face = _loadedFaces[faceId];
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (FT_Set_Pixel_Sizes(face, width, height)) {
        Engine.logger.error("Failed to set font size {} x {}", width, height);
        return;
    }

    FT_Size_Metrics m = face->size->metrics;

    FontMetrics& metrics = getFontMetricsForFace(faceId);
    metrics.ascender = static_cast<float>(m.ascender >> 6);
    metrics.descender = static_cast<float>(-(m.descender >> 6));
    metrics.height = static_cast<float>(m.height >> 6);

    for (std::uint32_t codepoint = 32; codepoint < 127; ++codepoint) {
        FT_UInt gIndex = FT_Get_Char_Index(face, codepoint);
        if (gIndex == 0) {
            continue;
        }

        if (FT_Load_Glyph(face, gIndex, FT_LOAD_DEFAULT)) {
            Engine.logger.warn("Failed to load glyph for codepoint {}", codepoint);
            continue;
        }

        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF)) {
            Engine.logger.warn("Failed to SDF-render glyph for codepoint {}", codepoint);
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
        glyph.codepoint = codepoint;
        glyph.w = slot->bitmap.width;
        glyph.h = slot->bitmap.rows;
        glyph.bearingX = slot->bitmap_left;
        glyph.bearingY = slot->bitmap_top;
        glyph.advance = slot->advance.x >> 6;
        glyph.ascent = face->size->metrics.ascender >> 6;
        glyph.descent = -(face->size->metrics.descender >> 6);

        glyph.u0 = static_cast<float>(x) / ATLAS_W;
        glyph.v0 = static_cast<float>(y) / ATLAS_H;
        glyph.u1 = static_cast<float>(x + glyph.w) / ATLAS_W;
        glyph.v1 = static_cast<float>(y + glyph.h) / ATLAS_H;

        glyphs[codepoint] = glyph;
    }

    if (faceId >= _glyphAtlases.size()) {
        _glyphAtlases.resize(faceId + 1);
    }

    OkayTextureMeta meta;
    meta.width = ATLAS_W;
    meta.height = ATLAS_H;
    meta.channels = 4;
    meta.mipLevels = 1;
    meta.format = OkayTextureMeta::Format::RGBA8;

    auto store = TextureDataStore::mainStore();

    auto handle = store->addTexture(meta, (void*)(_atlas.pixels().data()), _atlas.pixels().size());

    _glyphAtlases[faceId] = Texture(store, handle);

    _atlas.reset();
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

bool FontAtlas::addGlyph(const FT_Bitmap& bm, int& outX, int& outY) {
    const int gw = (int)bm.width;
    const int gh = (int)bm.rows;
    if (gw == 0 || gh == 0) {
        outX = 0;
        outY = 0;
        return true;
    }

    const int bw = gw + PAD * 2;
    const int bh = gh + PAD * 2;
    if (_penX + bw > _width) {
        _penX = PAD;
        _penY += _rowH;
        _rowH = 0;
    }

    if (_penY + bh > _height)
        return false;

    outX = _penX + PAD;
    outY = _penY + PAD;
    blit(bm, outX, outY);
    _penX += bw;
    if (bh > _rowH)
        _rowH = bh;

    return true;
}

void FontAtlas::reset() {
    _penX = PAD;
    _penY = PAD;
    _rowH = 0;
    std::fill(_pixels.begin(), _pixels.end(), 0);
}

void FontAtlas::blit(const FT_Bitmap& bm, int dstX, int dstY) {
    for (int row = 0; row < (int)bm.rows; ++row) {
        const std::uint8_t* src = (const std::uint8_t*)(bm.buffer + row * bm.pitch);

        for (int col = 0; col < (int)bm.width; ++col) {
            int x = dstX + col;
            int y = dstY + row;

            size_t idx = ((size_t)y * _width + x) * 4;

            _pixels[idx + 0] = 255;
            _pixels[idx + 1] = 255;
            _pixels[idx + 2] = 255;
            _pixels[idx + 3] = src[col];
        }
    }
}

};  // namespace okay
