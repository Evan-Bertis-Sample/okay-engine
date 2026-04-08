#ifndef __UI_COMPONENT_H__
#define __UI_COMPONENT_H__

#include <okay/core/ui/element.hpp>

namespace okay {

struct UIComponent {
    UIElement root;

    UIComponent() {}
    UIComponent(UIElement root) : root{root} {}
};

};  // namespace okay

#endif  // __UI_COMPONENT_H__