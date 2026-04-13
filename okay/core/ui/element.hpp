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

    // Child spacing
    OKAY_UI_ELEMENT_PROPERTY(size::Fixed, childSpacing, size::Fixed{0});

    // Content
    OKAY_UI_ELEMENT_PROPERTY(Option<std::string_view>, text);
    OKAY_UI_ELEMENT_PROPERTY(TextStyle, textStyle);
    OKAY_UI_ELEMENT_PROPERTY(glm::vec3, textColor, 1.0f, 1.0f, 1.0f);

    // background
    OKAY_UI_ELEMENT_PROPERTY(Option<Texture>, backgroundImage);
    OKAY_UI_ELEMENT_PROPERTY(glm::vec3, backgroundColor, 0.0f, 0.0f, 0.0f);

    OKAY_UI_ELEMENT_PROPERTY(bool, doubleSided, false);

    std::vector<UIElement> children;

    // Slate-style API
    template <typename... Children>
    UIElement& operator()(Children&&... newChildren) & {
        (children.push_back(std::forward<Children>(newChildren)), ...);
        return *this;
    }

    template <typename... Children>
    UIElement&& operator()(Children&&... newChildren) && {
        (children.push_back(std::forward<Children>(newChildren)), ...);
        return std::move(*this);
    }

    ElementSize getSizeAlongAxis(UIPrimaryAxis axis) const {
        return axis == UIPrimaryAxis::Horizontal ? width : height;
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

    UIElement& widthGrow() { return widthSet(size::Grow{}); }
    UIElement& heightGrow() { return heightSet(size::Grow{}); }

    UIElement& widthFit() { return widthSet(size::Fit{}); }
    UIElement& heightFit() { return heightSet(size::Fit{}); }

    UIElement& widthPercent(float percent) { return widthSet(size::Percent{percent}); }
    UIElement& heightPercent(float percent) { return heightSet(size::Percent{percent}); }

    UIElement& alignTextHorizontal(TextStyle::HorizontalAlignment alignment) {
        textStyle.horizontalAlignment = alignment;
        return *this;
    }

    UIElement& alignTextVertical(TextStyle::VerticalAlignment alignment) {
        textStyle.verticalAlignment = alignment;
        return *this;
    }
};

namespace ui {

inline UIElement flexbox() {
    return UIElement{};
}

inline UIElement text(std::string_view text, const TextStyle& style = TextStyle{}) {
    return UIElement{.text = text};
}

inline UIElement image(const Texture& texture) {
    return UIElement{.backgroundImage = texture, .backgroundColor = glm::vec3(1.0f, 1.0f, 1.0f)};
}

inline UIElement slot(UIPrimaryAxis axis = UIPrimaryAxis::Parent) {
    return UIElement{.axis = axis};
}

};  // namespace ui

};  // namespace okay

#endif  // __ELEMENT_H__