#include "text_layout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace okay {

TextLayout::TextLayout(std::string_view text, const TextStyle& style)
    : _text(text), _style(style), _fontManager(FontManager::instance()) {
    measure();
}

float TextLayout::computeLayoutScale(float targetFontHeight, int maxLineHeightGlyph) {
    if (targetFontHeight <= 0.0f)
        return 1.0f;
    if (maxLineHeightGlyph <= 0)
        return 1.0f;
    return 1.0;
}

float TextLayout::computeAlignedLineStartX(TextStyle::HorizontalAlignment alignment,
                                           float scaledLineWidth) {
    switch (alignment) {
        case TextStyle::HorizontalAlignment::Left:
            return 0.0f;
        case TextStyle::HorizontalAlignment::Center:
            return -scaledLineWidth * 0.5f;
        case TextStyle::HorizontalAlignment::Right:
            return -scaledLineWidth;
    }
    return 0.0f;
}

float TextLayout::computeVerticalAlignmentOffset(TextStyle::VerticalAlignment alignment,
                                                 float layoutTop,
                                                 float layoutBottom) {
    switch (alignment) {
        case TextStyle::VerticalAlignment::Top:
            return -layoutTop;
        case TextStyle::VerticalAlignment::Middle:
            return -0.5f * (layoutTop + layoutBottom);
        case TextStyle::VerticalAlignment::Bottom:
            return -layoutBottom;
    }
    return 0.0f;
}

TextLayout::RawLineMetrics TextLayout::measureRawLine(std::size_t lineBegin) const {
    RawLineMetrics raw{};
    if (lineBegin > _text.size()) {
        raw.nextLineBegin = _text.size();
        return raw;
    }

    const std::size_t start = lineBegin;
    std::size_t cursor = lineBegin;
    std::size_t end = lineBegin;

    while (cursor < _text.size()) {
        const char c = _text[cursor];

        if (c == '\r') {
            cursor++;
            continue;
        }

        if (c == '\n')
            break;

        const auto glyph = _fontManager.getGlyph(
            _style.font, static_cast<std::uint32_t>(static_cast<unsigned char>(c)));

        raw.lineWidthGlyph += static_cast<float>(glyph.advance);
        raw.lineAscentGlyph = std::max(raw.lineAscentGlyph, glyph.ascent);
        raw.lineDescentGlyph = std::max(raw.lineDescentGlyph, glyph.descent);

        cursor++;
        end = cursor;
    }

    raw.text = _text.substr(start, end - start);

    if (cursor < _text.size() && _text[cursor] == '\n') {
        raw.nextLineBegin = cursor + 1;
    } else {
        raw.nextLineBegin = _text.size();
    }

    return raw;
}

TextLineLayout TextLayout::makeLineLayout(std::size_t lineIndex,
                                          const RawLineMetrics& raw,
                                          float baselineY) const {
    TextLineLayout line{};
    line.lineIndex = lineIndex;
    line.text = raw.text;
    line.lineWidthGlyph = raw.lineWidthGlyph;
    line.lineAscentGlyph = raw.lineAscentGlyph;
    line.lineDescentGlyph = raw.lineDescentGlyph;
    line.baselineY = baselineY;

    const float scaledLineWidth = raw.lineWidthGlyph;
    const float startX = computeAlignedLineStartX(_style.horizontalAlignment, scaledLineWidth);

    line.layoutLeft = startX;
    line.layoutRight = startX + scaledLineWidth;
    line.layoutTop = baselineY + static_cast<float>(raw.lineAscentGlyph);
    line.layoutBottom = baselineY - static_cast<float>(raw.lineDescentGlyph);

    return line;
}

void TextLayout::measure() {
    _metrics = {};
    _verticalAlignmentOffset = 0.0f;

    float widestLineWidthGlyph = 0.0f;
    float topGlyph = -std::numeric_limits<float>::infinity();
    float bottomGlyph = std::numeric_limits<float>::infinity();

    float baselineYGlyph = 0.0f;
    std::size_t lineBegin = 0;

    while (true) {
        const RawLineMetrics raw = measureRawLine(lineBegin);

        widestLineWidthGlyph = std::max(widestLineWidthGlyph, raw.lineWidthGlyph);
        _metrics.maxLineHeightGlyph = std::max(_metrics.maxLineHeightGlyph, raw.lineHeightGlyph());
        _metrics.lineCount++;
        _metrics.glyphCount += raw.text.size();

        const float lineTopGlyph = baselineYGlyph + static_cast<float>(raw.lineAscentGlyph);
        const float lineBottomGlyph = baselineYGlyph - static_cast<float>(raw.lineDescentGlyph);

        topGlyph = std::max(topGlyph, lineTopGlyph);
        bottomGlyph = std::min(bottomGlyph, lineBottomGlyph);

        if (raw.nextLineBegin >= _text.size()) {
            break;
        }

        baselineYGlyph -= raw.lineAdvanceGlyph(_style.extraLineSpacingGlyph);
        lineBegin = raw.nextLineBegin;
    }

    if (!std::isfinite(topGlyph))
        topGlyph = 0.0f;
    if (!std::isfinite(bottomGlyph))
        bottomGlyph = 0.0f;

    const float unalignedTop = topGlyph;
    const float unalignedBottom = bottomGlyph;
    const float scaledWidestLineWidth = widestLineWidthGlyph;

    _verticalAlignmentOffset =
        computeVerticalAlignmentOffset(_style.verticalAlignment, unalignedTop, unalignedBottom);

    _metrics.layoutLeft =
        computeAlignedLineStartX(_style.horizontalAlignment, scaledWidestLineWidth);
    _metrics.layoutRight = _metrics.layoutLeft + scaledWidestLineWidth;
    _metrics.layoutTop = unalignedTop + _verticalAlignmentOffset;
    _metrics.layoutBottom = unalignedBottom + _verticalAlignmentOffset;

    _metrics.lineHeight = _fontManager.getGlyph(_style.font, 'A').h;
}

TextLayout::LineIterator TextLayout::begin() const {
    return LineIterator(this, 0, 0, _verticalAlignmentOffset, false);
}

TextLayout::LineIterator TextLayout::end() const {
    return LineIterator(this, _metrics.lineCount, _text.size(), 0.0f, true);
}

TextLayout::LineIterator::value_type TextLayout::LineIterator::operator*() const {
    const RawLineMetrics raw = _layout->measureRawLine(_lineBegin);
    return _layout->makeLineLayout(_lineIndex, raw, _baselineY);
}

TextLayout::LineIterator& TextLayout::LineIterator::operator++() {
    if (_atEnd)
        return *this;

    const RawLineMetrics raw = _layout->measureRawLine(_lineBegin);

    if (raw.nextLineBegin >= _layout->_text.size()) {
        _atEnd = true;
        _lineIndex = _layout->_metrics.lineCount;
        _lineBegin = _layout->_text.size();
        return *this;
    }

    _baselineY -= raw.lineAdvanceGlyph(_layout->_style.extraLineSpacingGlyph);
    _lineBegin = raw.nextLineBegin;
    _lineIndex++;

    return *this;
}

bool TextLayout::LineIterator::operator==(const LineIterator& other) const {
    if (_atEnd && other._atEnd)
        return true;

    return _layout == other._layout && _lineIndex == other._lineIndex &&
           _lineBegin == other._lineBegin && _atEnd == other._atEnd;
}

}  // namespace okay