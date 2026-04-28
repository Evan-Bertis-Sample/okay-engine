#ifndef __ELEMENT_H__
#define __ELEMENT_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/ui/text_layout.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/option.hpp>

#include <utility>
#include <variant>

namespace okay {

namespace size {

struct Fit {};
struct Grow {};
struct Percent {
    float percent;
};
struct Fixed {
    int pixels;

    Fixed(int pixels) : pixels(pixels) {}
    Fixed() : pixels(0) {}
};

};  // namespace size

using ElementSize = std::variant<size::Fit, size::Grow, size::Percent, size::Fixed>;
using ElementRealSize = std::variant<size::Percent, size::Fixed>;

enum class UIPrimaryAxis { Horizontal, Vertical, Parent };

#define OKAY_UI_ELEMENT_PROPERTY(typeName, valueName, ...) \
    typeName valueName{__VA_ARGS__};                       \
    UIElement& valueName##Set(typeName value) {            \
        (valueName) = value;                               \
        return *this;                                      \
    }

struct UIElement;

struct UIElementGenerator {
    struct Iterator {
        std::int32_t current;
        std::int32_t end;
        std::int32_t increment;
        std::function<UIElement(std::int32_t)> generator;

        UIElement operator*() const;

        Iterator& operator++() {
            current += increment;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return current < other.current;
        }
    };

    UIElementGenerator(std::int32_t count, std::function<UIElement(std::int32_t)> generator)
        : _start(0), _end(count), _increment(1), _generator(generator) {
        validateRange();
    }

    UIElementGenerator(std::int32_t start,
        std::int32_t end,
        std::int32_t increment,
        std::function<UIElement(std::int32_t)> generator)
        : _start(start), _end(end), _increment(increment), _generator(generator) {
        validateRange();
    }

    void validateRange() {
        // assert that you can actually reach the end
        if ((_increment > 0 && _end < _start) || (_increment < 0 && _end > _start)) {
            Engine.logger.error(
                "UIElementGenerator::Unable to reach end | start={}, end={}, increment={}",
                _start,
                _end,
                _increment);

            // fix this
            _end = _start;
        }

        // check you can take an integer number of steps
        float steps = static_cast<float>(_end - _start) / static_cast<float>(_increment);
        float iptr;
        if (std::modf(steps, &iptr) != 0.0f) {
            Engine.logger.error(
                "UIELementGenerator::Unable to take integer number of steps | start={}, end={}, "
                "increment={}",
                _start,
                _end,
                _increment);

            // rouund to the nearest amount of steps
            _end = static_cast<std::int32_t>(std::round(steps) * _increment) + _start;
        }
    }

    Iterator begin() {
        return Iterator{_start, _end, _increment, _generator};
    }

    Iterator end() {
        return Iterator{_end, _end, _increment, _generator};
    }

   private:
    std::int32_t _start;
    std::int32_t _end;
    std::int32_t _increment;
    std::function<UIElement(std::int32_t)> _generator;
};

struct UIElement {
   public:
    // Element positioning
    OKAY_UI_ELEMENT_PROPERTY(ElementSize, width, size::Fit{});
    OKAY_UI_ELEMENT_PROPERTY(ElementSize, height, size::Fit{});
    OKAY_UI_ELEMENT_PROPERTY(ElementRealSize, minWidth, size::Percent{0.0f});
    OKAY_UI_ELEMENT_PROPERTY(ElementRealSize, minHeight, size::Percent{0.0f});
    OKAY_UI_ELEMENT_PROPERTY(UIPrimaryAxis, axis, UIPrimaryAxis::Parent);

    // Padding
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, leftPadding, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, rightPadding, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, topPadding, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, bottomPadding, size::Fixed{0});

    // Margin
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, leftMargin, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, rightMargin, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, topMargin, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, bottomMargin, size::Fixed{0});

    // Child spacing
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, childSpacing, size::Fixed{0});

    // Content
    OKAY_UI_ELEMENT_PROPERTY(Option<std::string>, text);
    OKAY_UI_ELEMENT_PROPERTY(TextStyle, textStyle);
    OKAY_UI_ELEMENT_PROPERTY(glm::vec3, textColor, 1.0f, 1.0f, 1.0f);

    // background
    OKAY_UI_ELEMENT_PROPERTY(Option<Texture>, backgroundImage);
    OKAY_UI_ELEMENT_PROPERTY(glm::vec4, backgroundColor, 0.0f, 0.0f, 0.0f, 0.0f);

    // border
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, borderWidth, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, borderRadius, size::Fixed{0});
    OKAY_UI_ELEMENT_PROPERTY(glm::vec4, borderColor, 0.0f, 0.0f, 0.0f, 1.0f);

    // rendering
    OKAY_UI_ELEMENT_PROPERTY(bool, doubleSided, false);
    OKAY_UI_ELEMENT_PROPERTY(MaterialHandle, backgroundMaterialOverride, MaterialHandle::none());
    OKAY_UI_ELEMENT_PROPERTY(MaterialHandle, textMaterialOverride, MaterialHandle::none());

    std::vector<UIElement> children;

    // Slate-style API

    template <typename... Children>
    UIElement&& operator()(Children&&... newChildren) && {
        (addChild(std::forward<Children>(newChildren)), ...);
        return std::move(*this);
    }

    template <typename... Children>
    UIElement&& operator()(Children&&... newChildren) & {
        (addChild(std::forward<Children>(newChildren)), ...);
        return std::move(*this);
    }

    void addChild(UIElement child) {
        children.push_back(child);
    }
    void addChild(UIElementGenerator generator) {
        for (UIElement element : generator) {
            children.push_back(element);
        }
    }

    ElementSize getSizeAlongAxis(UIPrimaryAxis axis) const {
        return axis == UIPrimaryAxis::Horizontal ? width : height;
    }

    UIElement& paddingSet(
        size::Fixed left, size::Fixed right, size::Fixed top, size::Fixed bottom) {
        leftPadding = left;
        rightPadding = right;
        topPadding = top;
        bottomPadding = bottom;
        return *this;
    }

    UIElement& paddingSet(size::Fixed padding) {
        leftPadding = rightPadding = topPadding = bottomPadding = padding;
        return *this;
    }

    UIElement& paddingSet(size::Fixed horizontal, size::Fixed vertical) {
        leftPadding = rightPadding = horizontal;
        topPadding = bottomPadding = vertical;
        return *this;
    }

    UIElement& marginSet(size::Fixed left, size::Fixed right, size::Fixed top, size::Fixed bottom) {
        leftMargin = left;
        rightMargin = right;
        topMargin = top;
        bottomMargin = bottom;
        return *this;
    }

    UIElement& marginSet(size::Fixed margin) {
        leftMargin = rightMargin = topMargin = bottomMargin = margin;
        return *this;
    }

    UIElement& marginSet(size::Fixed horizontal, size::Fixed vertical) {
        leftMargin = rightMargin = horizontal;
        topMargin = bottomMargin = vertical;
        return *this;
    }

    UIElement& widthGrow() {
        return widthSet(size::Grow{});
    }
    UIElement& heightGrow() {
        return heightSet(size::Grow{});
    }
    UIElement& grow() {
        return widthGrow().heightGrow();
    }

    UIElement& widthFit() {
        return widthSet(size::Fit{});
    }
    UIElement& heightFit() {
        return heightSet(size::Fit{});
    }
    UIElement& fit() {
        return widthFit().heightFit();
    }

    UIElement& widthPercent(float percent) {
        return widthSet(size::Percent{percent});
    }
    UIElement& heightPercent(float percent) {
        return heightSet(size::Percent{percent});
    }

    UIElement& alignTextHorizontal(TextStyle::HorizontalAlignment alignment) {
        textStyle.horizontalAlignment = alignment;
        return *this;
    }

    UIElement& alignTextVertical(TextStyle::VerticalAlignment alignment) {
        textStyle.verticalAlignment = alignment;
        return *this;
    }

    UIElement& alignment(
        TextStyle::HorizontalAlignment horizontal, TextStyle::VerticalAlignment vertical) {
        alignTextHorizontal(horizontal);
        alignTextVertical(vertical);
        return *this;
    }

    UIElement& alignTextLeft() {
        return alignTextHorizontal(TextStyle::HorizontalAlignment::Left);
    }
    UIElement& alignTextRight() {
        return alignTextHorizontal(TextStyle::HorizontalAlignment::Right);
    }
    UIElement& alignTextCenter() {
        return alignTextHorizontal(TextStyle::HorizontalAlignment::Center);
    }

    UIElement& alignTextTop() {
        return alignTextVertical(TextStyle::VerticalAlignment::Top);
    }
    UIElement& alignTextMiddle() {
        return alignTextVertical(TextStyle::VerticalAlignment::Middle);
    }
    UIElement& alignTextBottom() {
        return alignTextVertical(TextStyle::VerticalAlignment::Bottom);
    }

    UIElement& textSizeSet(float size) {
        textStyle.fontHeight = size;
        return *this;
    }
};

};  // namespace okay

#endif  // __ELEMENT_H__
