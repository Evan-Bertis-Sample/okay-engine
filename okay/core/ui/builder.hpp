#ifndef __BUILDER_H__
#define __BUILDER_H__

#include "okay/core/engine/engine.hpp"

#include <okay/core/ui/element.hpp>
#include <okay/core/util/option.hpp>

namespace okay {

struct UIStyle {
   public:
    static UIStyle& main() {
        static UIStyle instance;
        return instance;
    }

    UIStyle();

    // Text styles
    UIElement text;
    UIElement h1;
    UIElement h2;
    UIElement h3;

    // Image styles
    UIElement image;

    static UIElement applyStyle(UIElement src, UIElement base) {
        Engine.logger.debug("Applying style!");
        src.width = base.width;
        src.height = base.height;
        src.minWidth = base.minWidth;
        src.minHeight = base.minHeight;
        src.leftPadding = base.leftPadding;
        src.rightPadding = base.rightPadding;
        src.topPadding = base.topPadding;
        src.bottomPadding = base.bottomPadding;
        src.leftMargin = base.leftMargin;
        src.rightMargin = base.rightMargin;
        src.topMargin = base.topMargin;
        src.bottomMargin = base.bottomMargin;
        src.childSpacing = base.childSpacing;
        src.textStyle = base.textStyle;
        src.textColor = base.textColor;
        src.backgroundColor = base.backgroundColor;
        src.borderWidth = base.borderWidth;
        src.borderRadius = base.borderRadius;
        src.borderColor = base.borderColor;
        src.doubleSided = base.doubleSided;
        return src;
    }

    void setMainFont(const FontManager::FontHandle& font);
};

struct UIStyleParam {
    UIStyleParam(UIStyle* style) : _style(style) {}

    UIStyle& operator*() const {
        if (_style == nullptr)
            return UIStyle::main();
        else
            return *_style;
    }

    UIStyle* operator->() const {
        UIStyle& s = **this;
        return &s;
    }

   private:
    UIStyle* _style{nullptr};
};

namespace ui {

inline UIElement flexbox() {
    return UIElement{};
}

inline UIElement text(std::string text, UIStyleParam style = nullptr) {
    return UIStyle::applyStyle(
        (UIElement){
            .text = text,
        },
        style->text);
}

inline UIElement h1(std::string text, UIStyleParam style = nullptr) {
    return UIStyle::applyStyle(
        (UIElement){
            .text = text,
        },
        style->h1);
}

inline UIElement h2(std::string text, UIStyleParam style = nullptr) {
    return UIStyle::applyStyle(
        (UIElement){
            .text = text,
        },
        style->h2);
}

inline UIElement h3(std::string text, UIStyleParam style = nullptr) {
    return UIStyle::applyStyle(
        (UIElement){
            .text = text,
        },
        style->h3);
}

inline UIElement image(const Texture& texture, UIStyleParam style = nullptr) {
    return UIStyle::applyStyle(
        (UIElement){
            .backgroundImage = texture,
            .backgroundColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        },
        style->image);
}

inline UIElement slot(UIPrimaryAxis axis = UIPrimaryAxis::Parent) {
    return UIElement{.axis = axis};
}

inline UIElementGenerator range(
    std::int32_t count, std::function<UIElement(std::int32_t)> generator) {
    return UIElementGenerator{count, generator};
}

inline UIElementGenerator range(std::int32_t start,
    std::int32_t end,
    std::int32_t increment,
    std::function<UIElement(std::int32_t)> generator) {
    return UIElementGenerator{start, end, increment, generator};
}

inline UIElement growbox(UIPrimaryAxis axis = UIPrimaryAxis::Parent) {
    return UIElement{.width = size::Grow{}, .height = size::Grow{}, .axis = axis};
}

inline UIElement row() {
    return UIElement{.axis = UIPrimaryAxis::Horizontal};
}

inline UIElement column() {
    return UIElement{.axis = UIPrimaryAxis::Vertical};
}

inline UIElement stack() {
    return UIElement{.axis = UIPrimaryAxis::Parent};
}

inline UIElement spacer() {
    return UIElement{.width = size::Grow{}, .height = size::Grow{}};
}

inline UIElement hspacer() {
    return UIElement{.width = size::Grow{}};
}

inline UIElement vspacer() {
    return UIElement{.height = size::Grow{}};
}

inline UIElement box() {
    return UIElement{};
}

inline UIElement frame(size::Fixed x,
    size::Fixed y,
    size::Fixed width,
    size::Fixed height,
    SystemParameter<Renderer> renderer = nullptr) {
    const float screenW = renderer->width();
    const float screenH = renderer->height();

    return UIElement{
        .width = width,
        .height = height,

        .leftMargin = x,
        .rightMargin = screenW - x.pixels - width.pixels,

        .topMargin = y,
        .bottomMargin = screenH - y.pixels - height.pixels,
    };
}

inline UIElement relFrame(size::Percent x,
    size::Percent y,
    size::Percent width,
    size::Percent height,
    SystemParameter<Renderer> renderer = nullptr) {
    const float screenW = renderer->width();
    const float screenH = renderer->height();

    return UIElement{
        .width = width,
        .height = height,

        .leftMargin = x.percent * screenW,
        .rightMargin = (1.0f - x.percent - width.percent) * screenW,

        .topMargin = y.percent * screenH,
        .bottomMargin = (1.0f - y.percent - height.percent) * screenH,
    };
}

inline UIElement centerFrame(size::Fixed x,
    size::Fixed y,
    size::Fixed width,
    size::Fixed height,
    SystemParameter<Renderer> renderer = nullptr) {
    const float screenW = renderer->width();
    const float screenH = renderer->height();

    const float halfW = width.pixels * 0.5f;
    const float halfH = height.pixels * 0.5f;

    return UIElement{
        .width = width,
        .height = height,

        .leftMargin = x.pixels - halfW,
        .rightMargin = screenW - (x.pixels + halfW),

        .topMargin = y.pixels - halfH,
        .bottomMargin = screenH - (y.pixels + halfH),
    };
}

inline UIElement relCenterFrame(size::Percent x,
    size::Percent y,
    size::Percent width,
    size::Percent height,
    SystemParameter<Renderer> renderer = nullptr) {
    const float screenW = renderer->width();
    const float screenH = renderer->height();

    const float halfW = width.percent * 0.5f;
    const float halfH = height.percent * 0.5f;

    return UIElement{
        .width = width,
        .height = height,

        .leftMargin = (x.percent - halfW) * screenW,
        .rightMargin = (1.0f - (x.percent + halfW)) * screenW,

        .topMargin = (y.percent - halfH) * screenH,
        .bottomMargin = (1.0f - (y.percent + halfH)) * screenH,
    };
}

};  // namespace ui

};  // namespace okay

#endif  // __BUILDER_H__
