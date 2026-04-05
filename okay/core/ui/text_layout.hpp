#ifndef __TEXT_LAYOUT_H__
#define __TEXT_LAYOUT_H__

#include <okay/core/ui/font.hpp>

#include <cstddef>
#include <string_view>

namespace okay {

struct TextStyle {
    enum class HorizontalAlignment { Left, Center, Right };
    enum class VerticalAlignment { Top, Middle, Bottom };

    FontManager::FontHandle font{};

    // Final height of the tallest line after scaling.
    float targetFontHeight{1.0f};

    // Extra spacing between lines, in glyph metric space, before scaling.
    float extraLineSpacingGlyph{0.0f};

    HorizontalAlignment horizontalAlignment{HorizontalAlignment::Left};
    VerticalAlignment verticalAlignment{VerticalAlignment::Top};
};

struct TextLineLayout {
    std::size_t lineIndex{0};
    std::string_view text{};

    float lineWidthGlyph{0.0f};
    int lineAscentGlyph{0};
    int lineDescentGlyph{0};

    // Final baseline position in layout space.
    float baselineY{0.0f};

    // Final bounds in layout space.
    float layoutLeft{0.0f};
    float layoutRight{0.0f};
    float layoutTop{0.0f};
    float layoutBottom{0.0f};

    int lineHeightGlyph() const { return lineAscentGlyph + lineDescentGlyph; }

    float lineAdvanceGlyph(float extraLineSpacingGlyph) const {
        return static_cast<float>(lineHeightGlyph()) + extraLineSpacingGlyph;
    }

    float layoutWidth() const { return layoutRight - layoutLeft; }
    float layoutHeight() const { return layoutTop - layoutBottom; }
};

struct TextLayoutMetrics {
    float layoutLeft{0.0f};
    float layoutRight{0.0f};
    float layoutTop{0.0f};
    float layoutBottom{0.0f};

    float layoutScale{1.0f};

    std::size_t lineCount{0};
    std::size_t glyphCount{0};
    int maxLineHeightGlyph{0};

    float width() const { return layoutRight - layoutLeft; }
    float height() const { return layoutTop - layoutBottom; }
};

class TextLayout {
   public:
    class LineIterator;

    TextLayout(std::string_view text, const TextStyle& style);

    std::string_view text() const { return _text; }
    const TextStyle& style() const { return _style; }
    const TextLayoutMetrics& metrics() const { return _metrics; }

    LineIterator begin() const;
    LineIterator end() const;

   private:
    struct RawLineMetrics {
        std::string_view text{};
        std::size_t nextLineBegin{0};

        float lineWidthGlyph{0.0f};
        int lineAscentGlyph{0};
        int lineDescentGlyph{0};

        int lineHeightGlyph() const { return lineAscentGlyph + lineDescentGlyph; }

        float lineAdvanceGlyph(float extraLineSpacingGlyph) const {
            return static_cast<float>(lineHeightGlyph()) + extraLineSpacingGlyph;
        }
    };

    std::string_view _text;
    TextStyle _style;
    FontManager& _fontManager;

    TextLayoutMetrics _metrics{};
    float _verticalAlignmentOffset{0.0f};

    void measure();

    RawLineMetrics measureRawLine(std::size_t lineBegin) const;

    TextLineLayout makeLineLayout(std::size_t lineIndex,
                                  const RawLineMetrics& raw,
                                  float baselineY) const;

    static float computeLayoutScale(float targetFontHeight, int maxLineHeightGlyph);
    static float computeAlignedLineStartX(TextStyle::HorizontalAlignment alignment,
                                          float scaledLineWidth);
    static float computeVerticalAlignmentOffset(TextStyle::VerticalAlignment alignment,
                                                float layoutTop,
                                                float layoutBottom);

   public:
    class LineIterator {
       public:
        using value_type = TextLineLayout;

        LineIterator() = default;

        value_type operator*() const;
        LineIterator& operator++();

        bool operator==(const LineIterator& other) const;
        bool operator!=(const LineIterator& other) const { return !(*this == other); }

       private:
        friend class TextLayout;

        LineIterator(const TextLayout* layout,
                     std::size_t lineIndex,
                     std::size_t lineBegin,
                     float baselineY,
                     bool atEnd)
            : _layout(layout),
              _lineIndex(lineIndex),
              _lineBegin(lineBegin),
              _baselineY(baselineY),
              _atEnd(atEnd) {}

        const TextLayout* _layout{nullptr};
        std::size_t _lineIndex{0};
        std::size_t _lineBegin{0};
        float _baselineY{0.0f};
        bool _atEnd{true};
    };
};

}  // namespace okay

#endif  // __TEXT_LAYOUT_H__