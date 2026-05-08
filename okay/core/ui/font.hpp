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

    int width() const {
        return _width;
    }
    int height() const {
        return _height;
    }

    const std::vector<std::uint8_t>& pixels() const {
        return _pixels;
    }

    bool addGlyph(const FT_Bitmap& bm, int& outX, int& outY);

    void reset();

   private:
    int _width;
    int _height;

    int _penX{PAD};
    int _penY{PAD};
    int _rowH{0};

    std::vector<std::uint8_t> _pixels;

    void blit(const FT_Bitmap& bm, int dstX, int dstY);
};

struct FontLoadOptions {
    int width{0};
    int height{64};

    bool operator==(const FontLoadOptions& other) const {
        return width == other.width && height == other.height;
    }

    bool operator<(const FontLoadOptions& other) const {
        return width < other.width;
    }
};

class FontManager {
   public:
    struct FontHandle {
       public:
        std::uint32_t id;

        bool operator==(const FontHandle& other) const {
            return id == other.id;
        }
        bool operator!=(const FontHandle& other) const {
            return !(*this == other);
        }
        bool operator<(const FontHandle& other) const {
            return id < other.id;
        }
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

    struct FontMetrics {
        float ascender;
        float descender;
        float height;
    };

    static FontManager& instance() {
        static FontManager instance;
        return instance;
    }

    FontManager();

    ~FontManager() {
        FT_Done_FreeType(_ftLibrary);
    }

    Option<FontHandle> loadFont(const std::string& fontPath, const FontLoadOptions& options = {});
    Glyph getGlyph(FontHandle font, std::uint32_t codepoint);
    FontMetrics getFontMetrics(FontHandle font) {
        return _metricsPerFace[font.id];
    }

    Texture getGlyphAtlas(FontHandle font) {
        return _glyphAtlases[font.id];
    }

    bool isLoadedFont(FontHandle font) {
        return font.id < _loadedFaces.size();
    }

    FontHandle defaultFont();

    // Freetype's default
    constexpr static int SDF_SPREAD_PX = 8;

   private:
    constexpr static int ATLAS_W = 2048;
    constexpr static int ATLAS_H = 2048;

    FT_Library _ftLibrary;
    std::unordered_map<std::string, std::uint32_t> _fontFaces;
    std::vector<FT_Face> _loadedFaces;
    std::vector<std::unordered_map<std::uint32_t, Glyph>> _glyphsPerFace;
    std::vector<FontMetrics> _metricsPerFace;
    std::vector<Texture> _glyphAtlases;
    FontAtlas _atlas{ATLAS_W, ATLAS_H};
    Option<FontHandle> _defaultFont;

    std::unordered_map<std::uint32_t, Glyph>& getGlyphsForFace(std::uint32_t faceId);
    FontMetrics& getFontMetricsForFace(std::uint32_t faceId);

    void generateGlyphSetAndAtlasForFace(std::uint32_t faceId, int width, int height);
};

};  // namespace okay

#endif  // _FONT_H__
