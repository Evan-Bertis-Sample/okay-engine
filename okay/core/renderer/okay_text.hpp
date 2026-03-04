#ifndef __OKAY_TEXT_H__
#define __OKAY_TEXT_H__

#include <okay/core/renderer/okay_font.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_mesh.hpp>

namespace okay {

struct OkayTextOptions {
    enum class Alignment { LEFT, CENTER, RIGHT };

    OkayFontManager::FontHandle font;
    OkayMeshBuffer& meshBuffer;
    Alignment alignment{Alignment::LEFT};
    bool doubleSided{false};
};

class OkayText {
   public:
    static OkayMesh generateTextMesh(const std::string& text, const OkayTextOptions& options) {
        OkayMeshData meshData;

        auto& fontManager = OkayFontManager::instance();

        float penX = 0.0f;
        float penY = 0.0f;
        std::size_t quadIndex = 0;

        for (char c : text) {
            if (c == '\n') {
                penX = 0;
                penY -= 32;  // simple line spacing (could use font height later)
                continue;
            }

            auto glyph = fontManager.getGlyph(options.font, (std::uint32_t)c);
            TextQuad quad = generateQuadForGlyph(glyph, penX, penY);

            // add vertices
            for (int i = 0; i < 4; i++) {
                meshData.vertices.push_back(quad.vertices[i]);
            }

            // add indices
            std::uint32_t base = quadIndex * 4;

            meshData.indices.push_back(base + 2);
            meshData.indices.push_back(base + 1);
            meshData.indices.push_back(base + 0);

            meshData.indices.push_back(base + 0);
            meshData.indices.push_back(base + 3);
            meshData.indices.push_back(base + 2);

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

    static TextQuad generateQuadForGlyph(OkayFontManager::Glyph glyph, float dx, float dy) {
        TextQuad quad;

        float xpos = dx + (float)glyph.bearingX;
        float ypos = dy - (float)glyph.bearingY;

        float w = (float)glyph.w;
        float h = (float)glyph.h;

        glm::vec3 normal{0, 0, 1};
        glm::vec3 color{1, 1, 1};

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

};  // namespace okay

#endif  // __OKAY_TEXT_H__