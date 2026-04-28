#ifndef __UI_COMPONENT_H__
#define __UI_COMPONENT_H__

#include <okay/core/ui/builder.hpp>
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/ui.hpp>

namespace okay {

struct UIComponent {
    UI ui;

    UIComponent() {}
    // wrapped in a flexbox such that the UI is the whole screen
    UIComponent(UIElement root) : ui(ui::flexbox()(root)) {}
};

};  // namespace okay

#endif  // __UI_COMPONENT_H__
