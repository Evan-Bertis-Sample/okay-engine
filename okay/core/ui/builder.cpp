#include "builder.hpp"

#include <array>

namespace okay {

static glm::vec4 scaleMaintainA(glm::vec4 vec, float s) {
    glm::vec4 res = vec * s;
    res.a = vec.a;
    return res;
}

UIStyle::UIStyle() {
    const glm::vec4 black = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    text = UIElement().textColorSet(scaleMaintainA(black, 0.8)).textSizeSet(12.0f);

    h1 = UIElement().textColorSet(scaleMaintainA(black, 0.95)).textSizeSet(24.0f);

    h2 = UIElement().textColorSet(scaleMaintainA(black, 0.9)).textSizeSet(18.0f);

    h3 = UIElement().textColorSet(scaleMaintainA(black, 0.9)).textSizeSet(14.0f);
};

void UIStyle::setMainFont(const FontManager::FontHandle& font) {
    text.textStyle.font = font;
    h1.textStyle.font = font;
    h2.textStyle.font = font;
    h3.textStyle.font = font;
};

}  // namespace okay