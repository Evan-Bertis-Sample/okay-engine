#ifndef __ELEMENT_H__
#define __ELEMENT_H__

#include <okay/core/renderer/texture.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/util/option.hpp>

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

enum class UIPrimaryAxis { Horizontal, Vertical };

struct UIElement {
    // Element positioning
    ElementAxisSize width{size::Fit{}};
    ElementAxisSize height{size::Fit{}};
    ElementMinMaxSize minWidth{size::Percent{0.0f}};
    ElementMinMaxSize minHeight{size::Percent{0.0f}};
    UIPrimaryAxis axis{UIPrimaryAxis::Vertical};

    // Content
    Option<std::string_view> text;
    Option<FontManager::FontHandle> font;
    glm::vec3 textColor{1.0f, 1.0f, 1.0f};

    // background
    Option<Texture> backgroundImage;
    glm::vec3 backgroundColor{0.0f, 0.0f, 0.0f};
};

struct UINode {};

};  // namespace okay

#endif  // __ELEMENT_H__