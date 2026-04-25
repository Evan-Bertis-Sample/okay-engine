#ifndef __UI_RECT_H__
#define __UI_RECT_H__

#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/uniform.hpp>

namespace okay {

struct UIRectMaterial : public UnlitMaterial {
    UniformProperty<glm::vec2, FixedString("u_borderRadius")> borderRadius{glm::vec2(0.0f)};
    UniformProperty<glm::vec2, FixedString("u_borderWidth")> borderWidth{glm::vec2(0.0f)};
    UniformProperty<glm::vec4, FixedString("u_borderColor")> borderColor{glm::vec4(0.0f)};

    UniformProperty<glm::vec2, FixedString("u_clipSpaceTL")> clipSpaceTL{glm::vec2(-1.0f)};
    UniformProperty<glm::vec2, FixedString("u_clipSpaceBR")> clipSpaceBR{glm::vec2(1.0f)};
};

}  // namespace okay

#endif  // __UI_RECT_H__