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
    Option<TextStyle> textStyle;
    glm::vec3 textColor{1.0f, 1.0f, 1.0f};

    // background
    Option<Texture> backgroundImage;
    glm::vec3 backgroundColor{0.0f, 0.0f, 0.0f};
};

struct UINode {
   public:
    using ID = std::uint32_t;
    ID id;

    UIElement element;
    std::span<const UINode> children;
};

struct UINodeHash {
    std::size_t operator()(const UINode& node) const noexcept { return node.hash(); }
};

namespace children {

template <size_t N>
struct NodeArray {
    std::array<UINode, N> nodes;

    operator std::span<const UINode>() const {
        return std::span<const UINode>(nodes.data(), nodes.size());
    }
};

template <typename... Ts>
auto multiple(Ts&&... nodes) {
    return NodeArray<sizeof...(Ts)>{.nodes = {std::forward<Ts>(nodes)...}};
}

}  // namespace children

class UI {
   public:
    UI(UINode root) : root{root} {}

   private:
    UINode root;

    struct NodeRenderInfo {
        RenderEntity renderEntity;
        size::Fixed calculatedWidth;
        size::Fixed calculatedHeight;
        size::Fixed calculatedX;
        size::Fixed calculatedY;
    };

    std::unordered_map<UINode::ID, NodeRenderInfo> _nodeRenderInfo;
};

};  // namespace okay

#endif  // __ELEMENT_H__