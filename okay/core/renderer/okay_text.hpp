#ifndef __OKAY_TEXT_H__
#define __OKAY_TEXT_H__

#include <okay/core/renderer/okay_font.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_mesh.hpp>

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <cmath>

namespace okay {

struct OkayTextOptions {
    enum class HoriztonalAlignment { LEFT, CENTER, RIGHT };
    enum class VerticalAlignment { TOP, MIDDLE, BOTTOM };

    OkayFontManager::FontHandle font;
    OkayMeshBuffer& meshBuffer;

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

class OkayTextMeshBuilder {
   public:
    explicit OkayTextMeshBuilder(const OkayTextOptions& options);

    OkayMesh build(const std::string& text);

   private:
    struct TextQuad {
        OkayVertex vertices[4];
    };

    struct LineInfo {
        float widthPx{0.0f};
        int ascentPx{0};
        int descentPx{0};

        int heightPx() const {
            return ascentPx + descentPx;
        }
        float advancePx(float extraSpacingPx) const {
            return (float)heightPx() + extraSpacingPx;
        }
    };

    const OkayTextOptions& _opt;
    OkayFontManager& _fm;

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
    static TextQuad generateQuadForGlyph(const OkayFontManager::Glyph& glyph,
                                         float dx,
                                         float dy,
                                         float scale);
};

class OkayText {
   public:
    static OkayMesh generateTextMesh(const std::string& text, const OkayTextOptions& options) {
        OkayTextMeshBuilder b(options);
        return b.build(text);
    }
};

}  // namespace okay

#endif  // __OKAY_TEXT_H__