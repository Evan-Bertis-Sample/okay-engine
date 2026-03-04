#ifndef __OKAY_TEXT_H__
#define __OKAY_TEXT_H__

#include <okay/core/renderer/okay_font.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_mesh.hpp>

#include <vector>
#include <string>
#include <cstdint>

namespace okay {

struct OkayTextOptions {
    enum class HoriztonalAlignment { LEFT, CENTER, RIGHT };
    enum class VerticalAlignment { TOP, MIDDLE, BOTTOM };

    OkayFontManager::FontHandle font;
    OkayMeshBuffer& meshBuffer;
    HoriztonalAlignment horizontalAlignment{HoriztonalAlignment::LEFT};
    VerticalAlignment verticalAlignment{VerticalAlignment::TOP};
    bool doubleSided{false};
};

class OkayText {
   public:
    static OkayMesh generateTextMesh(const std::string& text, const OkayTextOptions& options) {
        OkayMeshData meshData;
        auto& fm = OkayFontManager::instance();

        std::vector<float> lineWidths;
        lineWidths.reserve(8);

        float curWidth = 0.0f;
        std::size_t quadCount = 0;

        // line widths + quad count
        for (char c : text) {
            if (c == '\r')
                continue;

            if (c == '\n') {
                lineWidths.push_back(curWidth);
                curWidth = 0.0f;
                continue;
            }

            const auto glyph = fm.getGlyph(options.font, (std::uint32_t)(unsigned char)c);
            curWidth += (float)glyph.advance;
            quadCount++;
        }
        lineWidths.push_back(curWidth);

        meshData.vertices.reserve(quadCount * 4);
        meshData.indices.reserve(quadCount * (options.doubleSided ? 12 : 6));

        constexpr float LINE_SPACING = 32.0f;
        const float yOffset =
            computeVerticalOffset(lineWidths.size(), LINE_SPACING, options.verticalAlignment);

        float penY = yOffset;
        std::size_t lineIdx = 0;

        float penX = computeLineStartX(lineWidths[lineIdx], options.horizontalAlignment);
        std::size_t quadIndex = 0;

        // emit quads
        for (char c : text) {
            if (c == '\r')
                continue;

            if (c == '\n') {
                lineIdx++;
                penY -= LINE_SPACING;
                penX = computeLineStartX(lineWidths[lineIdx], options.horizontalAlignment);
                continue;
            }

            const auto glyph = fm.getGlyph(options.font, (std::uint32_t)(unsigned char)c);
            TextQuad quad = generateQuadForGlyph(glyph, penX, penY);

            for (int i = 0; i < 4; i++) {
                meshData.vertices.push_back(quad.vertices[i]);
            }

            const std::uint32_t base = (std::uint32_t)(quadIndex * 4);
            appendQuadIndices(meshData.indices, base, options.doubleSided);

            quadIndex++;
            penX += (float)glyph.advance;
        }

        return options.meshBuffer.addMesh(meshData);
    }

   private:
    struct TextQuad {
        // top left, top right, bottom right, bottom left
        OkayVertex vertices[4];
    };

    static float computeLineStartX(float lineWidth, OkayTextOptions::HoriztonalAlignment align) {
        switch (align) {
            case OkayTextOptions::HoriztonalAlignment::LEFT:
                return 0.0f;
            case OkayTextOptions::HoriztonalAlignment::CENTER:
                return -lineWidth * 0.5f;
            case OkayTextOptions::HoriztonalAlignment::RIGHT:
                return -lineWidth;
        }
        return 0.0f;
    }

    static float computeVerticalOffset(std::size_t lineCount,
                                       float lineSpacing,
                                       OkayTextOptions::VerticalAlignment align) {
        if (lineCount == 0)
            return 0.0f;

        const float blockHeight = (lineCount > 1) ? (float)(lineCount - 1) * lineSpacing : 0.0f;
        switch (align) {
            case OkayTextOptions::VerticalAlignment::TOP:
                // first baseline at y=0
                return 0.0f;
            case OkayTextOptions::VerticalAlignment::MIDDLE:
                // center baselines around y=0
                return blockHeight * 0.5f;
            case OkayTextOptions::VerticalAlignment::BOTTOM:
                // last baseline at y=0
                return blockHeight;
        }
        return 0.0f;
    }

    static void appendQuadIndices(std::vector<std::uint32_t>& indices,
                                  std::uint32_t base,
                                  bool doubleSided) {
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 0);
        indices.push_back(base + 0);
        indices.push_back(base + 3);
        indices.push_back(base + 2);

        if (!doubleSided)
            return;

        // Reverse winding for the back face
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 0);
    }

    static TextQuad generateQuadForGlyph(const OkayFontManager::Glyph& glyph, float dx, float dy) {
        TextQuad quad;

        const float xpos = dx + (float)glyph.bearingX;
        const float ypos = dy - (float)glyph.bearingY;

        const float w = (float)glyph.w;
        const float h = (float)glyph.h;

        const glm::vec3 normal{0, 0, 1};
        const glm::vec3 color{1, 1, 1};

        // top-left
        quad.vertices[0] = {{xpos, ypos + h, 0}, normal, color, {glyph.u0, glyph.v0}};
        // top-right
        quad.vertices[1] = {{xpos + w, ypos + h, 0}, normal, color, {glyph.u1, glyph.v0}};
        // bottom-right
        quad.vertices[2] = {{xpos + w, ypos, 0}, normal, color, {glyph.u1, glyph.v1}};
        // bottom-left
        quad.vertices[3] = {{xpos, ypos, 0}, normal, color, {glyph.u0, glyph.v1}};

        return quad;
    }
};

}  // namespace okay

#endif  // __OKAY_TEXT_H__