#ifndef __UI_COMPONENT_H__
#define __UI_COMPONENT_H__

#include <okay/core/ui/element.hpp>
#include <okay/core/ui/ui.hpp>

namespace okay {

struct UIComponent {
    UI ui;

    UIComponent() {}
    UIComponent(UIElement root) : ui(root) {}
};

};  // namespace okay

#endif  // __UI_COMPONENT_H__