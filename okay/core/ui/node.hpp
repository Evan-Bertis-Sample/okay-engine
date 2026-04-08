#ifndef __NODE_H__
#define __NODE_H__

#include <okay/core/ui/element.hpp>

#include <array>
#include <span>
#include <unordered_map>

namespace okay {

struct UINode {
   public:
    using ID = std::uint32_t;
    ID id;

    UIElement element;
    std::span<const UINode> children;
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

#endif  // __NODE_H__