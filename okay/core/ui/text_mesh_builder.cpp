#include "text_mesh_builder.hpp"

#include <string_view>

namespace okay {

void TextMeshBuilder::appendQuadIndices(std::vector<std::uint32_t>& indices,
                                        std::uint32_t baseVertex,
                                        bool doubleSided) {
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 3);
    indices.push_back(baseVertex + 2);

    if (!doubleSided)
        return;

    indices.push_back(baseVertex + 0);
    indices.push_back(baseVertex + 1);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 2);
    indices.push_back(baseVertex + 3);
    indices.push_back(baseVertex + 0);
}

TextMeshBuilder::TextQuad TextMeshBuilder::generateQuadForGlyph(const FontManager::Glyph& glyph,
                                                                float baselineX,
                                                                float baselineY,
                                                                float layoutScale) {
    TextQuad quad{};

    const float glyphWidth = static_cast<float>(glyph.w) * layoutScale;
    const float glyphHeight = static_cast<float>(glyph.h) * layoutScale;

    const float glyphLeft = baselineX + static_cast<float>(glyph.bearingX) * layoutScale;
    const float glyphTop = baselineY + static_cast<float>(glyph.bearingY) * layoutScale;
    const float glyphRight = glyphLeft + glyphWidth;
    const float glyphBottom = glyphTop - glyphHeight;

    const glm::vec3 normal{0.0f, 0.0f, 1.0f};
    const glm::vec3 color{1.0f, 1.0f, 1.0f};

    quad.vertices[0] = {{glyphLeft, glyphTop, 0.0f}, normal, color, {glyph.u0, glyph.v0}};
    quad.vertices[1] = {{glyphRight, glyphTop, 0.0f}, normal, color, {glyph.u1, glyph.v0}};
    quad.vertices[2] = {{glyphRight, glyphBottom, 0.0f}, normal, color, {glyph.u1, glyph.v1}};
    quad.vertices[3] = {{glyphLeft, glyphBottom, 0.0f}, normal, color, {glyph.u0, glyph.v1}};

    return quad;
}

MeshData TextMeshBuilder::build(std::string_view text, const TextStyle& style, bool doubleSided) {
    FontManager& fontManager = FontManager::instance();
    const TextLayout layout(text, style);

    MeshData meshData;
    meshData.vertices.reserve(layout.metrics().glyphCount * 4);
    meshData.indices.reserve(layout.metrics().glyphCount * (doubleSided ? 12 : 6));

    std::size_t quadIndex = 0;

    for (const TextLineLayout line : layout) {
        float baselineX = line.layoutLeft;
        const float baselineY = line.baselineY;

        for (char c : line.text) {
            const auto glyph = fontManager.getGlyph(
                layout.style().font, static_cast<std::uint32_t>(static_cast<unsigned char>(c)));

            const TextQuad quad = generateQuadForGlyph(glyph, baselineX, baselineY, 1.0f);

            for (int i = 0; i < 4; ++i) {
                meshData.vertices.push_back(quad.vertices[i]);
            }

            appendQuadIndices(
                meshData.indices, static_cast<std::uint32_t>(quadIndex * 4), doubleSided);

            quadIndex++;
            baselineX += static_cast<float>(glyph.advance);
        }
    }

    return meshData;
}

}  // namespace okay