#ifndef __UI_COMPONENT_H__
#define __UI_COMPONENT_H__

#include <okay/core/ui/builder.hpp>
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/ui.hpp>

#include <functional>

namespace okay {

struct UIComponent {
    UI ui;
    std::function<UIElement()> uiBuilder;

    UIComponent() {}
    // wrapped in a flexbox such that the UI is the whole screen
    UIComponent(UIElement root)
        : uiBuilder([root]() {
              return ui::flexbox()(root);
          }) {}

    UIComponent(std::function<UIElement()> builder) : uiBuilder(builder) {}
};

};  // namespace okay

#endif  // __UI_COMPONENT_H__
