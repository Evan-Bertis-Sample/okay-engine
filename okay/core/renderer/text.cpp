#include "text.hpp"

namespace okay {

TextMeshBuilder::TextMeshBuilder(const TextOptions& options)
    : _opt(options), _fm(FontManager::instance()) {
    _lines.reserve(8);
}

float TextMeshBuilder::computeFontScale(float desiredFontSize, int maxLineHeightPx) {
    if (desiredFontSize <= 0.0f)
        return 1.0f;
    if (maxLineHeightPx <= 0)
        return 1.0f;
    return desiredFontSize / (float)maxLineHeightPx;
}

float TextMeshBuilder::computeLineStartX(float lineWidthScaled) const {
    switch (_opt.horizontalAlignment) {
        case TextOptions::HoriztonalAlignment::LEFT:
            return 0.0f;
        case TextOptions::HoriztonalAlignment::CENTER:
            return -lineWidthScaled * 0.5f;
        case TextOptions::HoriztonalAlignment::RIGHT:
            return -lineWidthScaled;
    }
    return 0.0f;
}

void TextMeshBuilder::calcSpacing(const std::string& text) {
    _lines.clear();
    _lines.push_back(LineInfo{});

    _quadCount = 0;

    for (char c : text) {
        if (c == '\r')
            continue;

        if (c == '\n') {
            _lines.push_back(LineInfo{});
            continue;
        }

        const auto g = _fm.getGlyph(_opt.font, (std::uint32_t)(unsigned char)c);

        auto& line = _lines.back();
        line.widthPx += (float)g.advance;
        line.ascentPx = std::max(line.ascentPx, g.ascent);
        line.descentPx = std::max(line.descentPx, g.descent);

        _quadCount++;
    }

    _maxLineHeightPx = 0;
    for (const auto& line : _lines) {
        _maxLineHeightPx = std::max(_maxLineHeightPx, line.heightPx());
    }

    _scale = computeFontScale(_opt.fontSize, _maxLineHeightPx);
    _bounds = computeBounds();
    _yOffset = computeVerticalOffset();
}

OkayTextBounds TextMeshBuilder::computeBounds() const {
    float top = -std::numeric_limits<float>::infinity();
    float bottom = std::numeric_limits<float>::infinity();

    float baselineY = 0.0f;

    for (std::size_t i = 0; i < _lines.size(); ++i) {
        const float lineTop = baselineY + (float)_lines[i].ascentPx;
        const float lineBottom = baselineY - (float)_lines[i].descentPx;

        top = std::max(top, lineTop);
        bottom = std::min(bottom, lineBottom);

        baselineY -= _lines[i].advancePx(_opt.verticalSpacing);
    }

    top *= _scale;
    bottom *= _scale;

    if (!std::isfinite(top))
        top = 0.0f;
    if (!std::isfinite(bottom))
        bottom = 0.0f;

    return OkayTextBounds{top, bottom};
}

float TextMeshBuilder::computeVerticalOffset() const {
    const float top = _bounds.top;
    const float bottom = _bounds.bottom;

    switch (_opt.verticalAlignment) {
        case TextOptions::VerticalAlignment::TOP:
            return -bottom;  // anchor bottom at y=0
        case TextOptions::VerticalAlignment::MIDDLE:
            return -0.5f * (top + bottom);  // anchor center at y=0
        case TextOptions::VerticalAlignment::BOTTOM:
            return -top;  // anchor top at y=0
    }
    return 0.0f;
}

void TextMeshBuilder::appendQuadIndices(std::vector<std::uint32_t>& indices,
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

    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
    indices.push_back(base + 0);
}

TextMeshBuilder::TextQuad TextMeshBuilder::generateQuadForGlyph(
    const FontManager::Glyph& glyph, float dx, float dy, float scale) {
    TextQuad quad;

    const float w = (float)glyph.w * scale;
    const float h = (float)glyph.h * scale;

    // dy is the bottom of the glyph quad
    // baseline sits 'descent' above the bottom.
    const float baselineY = dy + (float)glyph.descent * scale;
    const float xpos = dx + (float)glyph.bearingX * scale;
    const float topY = baselineY + (float)glyph.bearingY * scale;
    const float bottomY = topY - h;

    const glm::vec3 normal{0, 0, 1};
    const glm::vec3 color{1, 1, 1};

    quad.vertices[0] = {{xpos, topY, 0}, normal, color, {glyph.u0, glyph.v0}};         // TL
    quad.vertices[1] = {{xpos + w, topY, 0}, normal, color, {glyph.u1, glyph.v0}};     // TR
    quad.vertices[2] = {{xpos + w, bottomY, 0}, normal, color, {glyph.u1, glyph.v1}};  // BR
    quad.vertices[3] = {{xpos, bottomY, 0}, normal, color, {glyph.u0, glyph.v1}};      // BL

    return quad;
}

Mesh TextMeshBuilder::build(const std::string& text) {
    calcSpacing(text);

    MeshData meshData;
    meshData.vertices.reserve(_quadCount * 4);
    meshData.indices.reserve(_quadCount * (_opt.doubleSided ? 12 : 6));

    std::size_t lineIdx = 0;
    float penY = _yOffset;
    float penX = computeLineStartX(_lines[lineIdx].widthPx * _scale);

    std::size_t quadIndex = 0;

    for (char c : text) {
        if (c == '\r')
            continue;

        if (c == '\n') {
            penY -= _lines[lineIdx].advancePx(_opt.verticalSpacing) * _scale;
            lineIdx++;
            penX = computeLineStartX(_lines[lineIdx].widthPx * _scale);
            continue;
        }

        const auto g = _fm.getGlyph(_opt.font, (std::uint32_t)(unsigned char)c);

        TextQuad quad = generateQuadForGlyph(g, penX, penY, _scale);
        for (int i = 0; i < 4; i++) {
            meshData.vertices.push_back(quad.vertices[i]);
        }

        const std::uint32_t base = (std::uint32_t)(quadIndex * 4);
        appendQuadIndices(meshData.indices, base, _opt.doubleSided);

        quadIndex++;
        penX += (float)g.advance * _scale;
    }

    return _opt.meshBuffer.addMesh(meshData);
}

};  // namespace okay