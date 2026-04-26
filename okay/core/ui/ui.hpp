#ifndef __UI_H__
#define __UI_H__

#include "element.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/ui/element.hpp>

#include <array>
#include <glm/glm.hpp>
#include <span>
#include <unordered_map>

namespace okay {

struct UINode {
   public:
    using ID = std::uint32_t;
    ID id;

    UIElement element;
    std::vector<UINode> children;
};

struct LayoutRect {
    glm::ivec2 pxPosition{0, 0};
    glm::ivec2 pxSize{0, 0};
    UIPrimaryAxis axis{UIPrimaryAxis::Parent};
    bool resolvedSize{false};
};

class UILayout {
   public:
    UILayout() = default;
    UILayout(UINode root) : _root(root) {}

    struct Context {
        glm::ivec2 screenSize;
    };

    void layout(const Context& context);

    Option<LayoutRect> getLayout(const UINode& node) const {
        if (!_layoutMap.contains(node.id)) {
            return Option<LayoutRect>::none();
        }
        return _layoutMap.at(node.id);
    };

    void toString(std::stringstream& ss) const;

   private:
    UINode _root;
    std::unordered_map<UINode::ID, LayoutRect> _layoutMap;

    void computeFixedSizes(UINode& node, LayoutRect frame);
    void computeFitSizes(UINode& node, LayoutRect frame);
    void computePositions(UINode& node, LayoutRect frame);

    LayoutRect& getOrMakeRect(const UINode& node) {
        if (!_layoutMap.contains(node.id)) {
            _layoutMap[node.id] = LayoutRect{};
        }
        return _layoutMap[node.id];
    }

    LayoutRect getRect(const UINode& node) const {
        return _layoutMap.at(node.id);
    }

    int computeSize(ElementRealSize size, int parentSize) const {
        if (std::holds_alternative<size::Percent>(size)) {
            float percent = std::get<size::Percent>(size).percent;
            return static_cast<int>(percent * (float)parentSize);
        } else if (std::holds_alternative<size::Fixed>(size)) {
            return std::get<size::Fixed>(size).pixels;
        }
        return 0;
    }

    int marginMainStart(const UIElement& element, UIPrimaryAxis axis) const;
    int marginMainEnd(const UIElement& element, UIPrimaryAxis axis) const;
    int marginCrossStart(const UIElement& element, UIPrimaryAxis axis) const;
    int marginCrossEnd(const UIElement& element, UIPrimaryAxis axis) const;
    int marginMainTotal(const UIElement& element, UIPrimaryAxis axis) const;
    int marginCrossTotal(const UIElement& element, UIPrimaryAxis axis) const;

    Option<int> computeElementSizeAlongAxis(
        const UINode& node, UIPrimaryAxis axis, LayoutRect frame) const;

    void toString(std::stringstream& ss, const UINode& node, int indent = 0) const;
};

class UI {
   public:
    UI() = default;
    UI(UIElement root);

   public:
    void render(glm::vec2 screenPosition, SystemParameter<Renderer> renderer = nullptr);

   private:
    UINode _root;
    UINode::ID _nextNodeID{1};

    struct NodeRenderInfo {
        RenderEntity rectEntity;
        RenderEntity textEntity;
        bool entityCreated = false;
    };

    UINode::ID createNodeIDFromElement(const UIElement& element) {
        return _nextNodeID++;
    }
    UINode createNodeFromElement(const UIElement& element) {
        UINode node;
        node.id = createNodeIDFromElement(element);
        node.element = element;
        for (const UIElement& childElement : element.children) {
            node.children.push_back(createNodeFromElement(childElement));
        }
        return node;
    }

    NodeRenderInfo& getNodeRenderInfo(const UINode& node) {
        if (!_nodeRenderInfo.contains(node.id)) {
            _nodeRenderInfo[node.id] = NodeRenderInfo{};
        }
        return _nodeRenderInfo[node.id];
    }

    void renderNode(const UINode& node, Renderer& renderer, int depth = 0);
    void createNodeRenderEnties(const UINode& node, Renderer& renderer);

    UILayout _layout;
    std::unordered_map<UINode::ID, NodeRenderInfo> _nodeRenderInfo;
};

};  // namespace okay

#endif  // __UI_H__