// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// ui_component.hpp - Okay Engine

#pragma once

#include <okay/core/ui/builder.hpp>
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/ui.hpp>

#include <functional>

namespace okay {

struct UIComponent {
    UI ui;
    std::function<UIElement()> uiBuilder;
    std::uint8_t uiLayer{0};

    UIComponent() {}
    // wrapped in a flexbox such that the UI is the whole screen
    UIComponent(UIElement root, std::uint8_t uiLayer = 0)
        : uiBuilder([root]() {
              return ui::flexbox()(root);
          }),
          uiLayer(uiLayer) {}

    UIComponent(std::function<UIElement()> builder, std::uint8_t uiLayer = 0)
        : uiBuilder(builder), uiLayer(uiLayer) {}
};

};  // namespace okay
