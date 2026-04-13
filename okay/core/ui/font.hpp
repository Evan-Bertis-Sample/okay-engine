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

        bool operator==(const FontHandle& other) const { return id == other.id; }
        bool operator!=(const FontHandle& other) const { return !(*this == other); }
        bool operator<(const FontHandle& other) const { return id < other.id; }
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

    FontManager();

    ~FontManager() { FT_Done_FreeType(_ftLibrary); }

    Option<FontHandle> loadFont(const std::string& fontPath, const FontLoadOptions& options = {});

    Glyph getGlyph(FontHandle font, std::uint32_t codepoint);

    Texture getGlyphAtlas(FontHandle font) { return _glyphAtlases[font.id]; }

    bool isLoadedFont(FontHandle font) { return font.id < _loadedFaces.size(); }

    FontHandle defaultFont();

   private:
    constexpr static int ATLAS_W = 2048;
    constexpr static int ATLAS_H = 2048;

    FT_Library _ftLibrary;
    std::unordered_map<std::string, std::uint32_t> _fontFaces;
    std::vector<FT_Face> _loadedFaces;
    std::vector<std::unordered_map<std::uint32_t, Glyph>> _glyphsPerFace;
    std::vector<Texture> _glyphAtlases;
    FontAtlas _atlas{ATLAS_W, ATLAS_H};
    Option<FontHandle> _defaultFont;

    std::unordered_map<std::uint32_t, Glyph>& getGlyphsForFace(std::uint32_t faceId);

    void generateGlyphSetAndAtlasForFace(std::uint32_t faceId, int width, int height);
};

};  // namespace okay

#endif  // _FONT_H__