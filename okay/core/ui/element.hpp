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
};

};  // namespace size

struct ElementAxisSize {
    std::variant<size::Fit, size::Grow, size::Percent, size::Fixed> value{size::Fit{}};
};

struct ElementMinMaxSize {
    std::variant<size::Percent, size::Fixed> value{size::Percent{0.0f}};
};

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
    OKAY_UI_ELEMENT_PROPERTY(ElementAxisSize, width, size::Fit{});
    OKAY_UI_ELEMENT_PROPERTY(ElementAxisSize, height, size::Fit{});
    OKAY_UI_ELEMENT_PROPERTY(ElementMinMaxSize, minWidth, size::Percent{1.0f});
    OKAY_UI_ELEMENT_PROPERTY(ElementMinMaxSize, minHeight, size::Percent{1.0f});
    OKAY_UI_ELEMENT_PROPERTY(UIPrimaryAxis, axis, UIPrimaryAxis::Parent);

    // Content
    OKAY_UI_ELEMENT_PROPERTY(Option<std::string_view>, text);
    OKAY_UI_ELEMENT_PROPERTY(Option<TextStyle>, textStyle);
    OKAY_UI_ELEMENT_PROPERTY(glm::vec3, textColor, 1.0f, 1.0f, 1.0f);

    // background
    OKAY_UI_ELEMENT_PROPERTY(Option<Texture>, backgroundImage);
    OKAY_UI_ELEMENT_PROPERTY(glm::vec3, backgroundColor, 0.0f, 0.0f, 0.0f);

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