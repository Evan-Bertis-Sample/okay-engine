#ifndef __FONT_H__
#define __FONT_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/util/option.hpp>

#include <cstdint>
#include <stdexcept>
#include <unordered_map>

namespace okay {

class FontAtlas {
   public:
    static constexpr int PAD = 1;

    FontAtlas(int w, int h) : _width(w), _height(h), _pixels((size_t)w * (size_t)h * 4, 0) {}

    int width() const { return _width; }
    int height() const { return _height; }

    const std::vector<std::uint8_t>& pixels() const { return _pixels; }

    bool addGlyph(const FT_Bitmap& bm, int& outX, int& outY) {
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

    void reset() {
        _penX = PAD;
        _penY = PAD;
        _rowH = 0;
        std::fill(_pixels.begin(), _pixels.end(), 0);
    }

   private:
    int _width;
    int _height;

    int _penX{PAD};
    int _penY{PAD};
    int _rowH{0};

    std::vector<std::uint8_t> _pixels;

    void blit(const FT_Bitmap& bm, int dstX, int dstY) {
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
};

struct FontLoadOptions {
    int width{32};
    int height{0};
};

class FontManager {
   public:
    struct FontHandle {
       public:
        std::uint32_t id;
    };

    struct Glyph {
        std::uint32_t codepoint;
        // width and height
        int w;
        int h;
        // normalized Uvs
        float u0, v0, u1, v1;
        // layout metrics
        int bearingX;
        int bearingY;
        int advance;

        int ascent;
        int descent;
    };

    static FontManager& instance() {
        static FontManager instance;
        return instance;
    }

    FontManager() {
        if (FT_Init_FreeType(&_ftLibrary)) {
            throw std::runtime_error("Failed to initialize FreeType library");
        }
    }

    ~FontManager() { FT_Done_FreeType(_ftLibrary); }

    Option<FontHandle> loadFont(const std::string& fontPath, const FontLoadOptions& options = {}) {
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

    Glyph getGlyph(FontHandle font, std::uint32_t codepoint) {
        auto& glyphs = getGlyphsForFace(font.id);
        if (glyphs.find(codepoint) != glyphs.end()) {
            return glyphs[codepoint];
        }
        // Return an empty glyph if not found
        return Glyph{codepoint, 0, 0, 0, 0, 0, 0, 0};
    }

    Texture getGlyphAtlas(FontHandle font) { return _glyphAtlases[font.id]; }

   private:
    constexpr static int ATLAS_W = 2048;
    constexpr static int ATLAS_H = 2048;

    FT_Library _ftLibrary;
    std::unordered_map<std::string, std::uint32_t> _fontFaces;
    std::vector<FT_Face> _loadedFaces;
    std::vector<std::unordered_map<std::uint32_t, Glyph>> _glyphsPerFace;
    std::vector<Texture> _glyphAtlases;
    FontAtlas _atlas{ATLAS_W, ATLAS_H};

    std::unordered_map<std::uint32_t, Glyph>& getGlyphsForFace(std::uint32_t faceId) {
        if (faceId >= _glyphsPerFace.size()) {
            _glyphsPerFace.resize(faceId + 1);
        }
        return _glyphsPerFace[faceId];
    };

    void generateGlyphSetAndAtlasForFace(std::uint32_t faceId, int width, int height) {
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

        auto handle =
            store->addTexture(meta, (void*)_atlas.pixels().data(), _atlas.pixels().size());
        _glyphAtlases[faceId] = Texture(store, handle);

        _atlas.reset();  // clear the atlas for the next font face
    }
};

};  // namespace okay

#endif  // _FONT_H__