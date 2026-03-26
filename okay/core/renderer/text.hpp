#ifndef _TEXT_H__
#define _TEXT_H__

#include <okay/core/renderer/font.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/renderer.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace okay {

struct TextOptions {
    enum class HoriztonalAlignment { LEFT, CENTER, RIGHT };
    enum class VerticalAlignment { TOP, MIDDLE, BOTTOM };

    FontManager::FontHandle font;
    MeshBuffer& meshBuffer;

    float fontSize{1.0f};
    float verticalSpacing{0.0f};  // pixels before scaling

    HoriztonalAlignment horizontalAlignment{HoriztonalAlignment::LEFT};
    VerticalAlignment verticalAlignment{VerticalAlignment::TOP};

    bool doubleSided{false};
};

struct OkayTextBounds {
    float top{0.0f};
    float bottom{0.0f};
};

class TextMeshBuilder {
   public:
    explicit TextMeshBuilder(const TextOptions& options);

    Mesh build(const std::string& text);

   private:
    struct TextQuad {
        MeshVertex vertices[4];
    };

    struct LineInfo {
        float widthPx{0.0f};
        int ascentPx{0};
        int descentPx{0};

        int heightPx() const { return ascentPx + descentPx; }
        float advancePx(float extraSpacingPx) const { return (float)heightPx() + extraSpacingPx; }
    };

    const TextOptions& _opt;
    FontManager& _fm;

    std::vector<LineInfo> _lines;
    std::size_t _quadCount{0};

    int _maxLineHeightPx{0};
    float _scale{1.0f};

    OkayTextBounds _bounds{};
    float _yOffset{0.0f};

    void calcSpacing(const std::string& text);
    float computeLineStartX(float lineWidthScaled) const;
    OkayTextBounds computeBounds() const;
    float computeVerticalOffset() const;

    static float computeFontScale(float desiredFontSize, int maxLineHeightPx);
    static void appendQuadIndices(std::vector<std::uint32_t>& indices,
                                  std::uint32_t base,
                                  bool doubleSided);
    static TextQuad generateQuadForGlyph(const FontManager::Glyph& glyph,
                                         float dx,
                                         float dy,
                                         float scale);
};

class OkayText {
   public:
    static Mesh generateTextMesh(const std::string& text, const TextOptions& options) {
        TextMeshBuilder b(options);
        return b.build(text);
    }
};

}  // namespace okay

#endif  // _TEXT_H__