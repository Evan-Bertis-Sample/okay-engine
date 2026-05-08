#ifndef __UI_H__
#define __UI_H__

#include "element.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/ui/element.hpp>
#include <okay/core/ui/text_mesh_buffer.hpp>

#include <array>
#include <glm/glm.hpp>
#include <span>
#include <unordered_map>

namespace okay {

struct UINode {
   public:
    using ID = std::uint32_t;
    ID id;
    ID parentID;
    UIElement element;
    std::vector<UINode> children;
};

struct LayoutRect {
    glm::ivec2 pxPosition{0, 0};
    glm::ivec2 pxSize{0, 0};
    UIAxis axis{UIAxis::Parent};
    bool resolvedSize{false};

    glm::vec2 bottomRight() const {
        return glm::vec2(pxPosition.x + pxSize.x, pxPosition.y + pxSize.y);
    }

    glm::vec2 topLeft() const {
        return pxPosition;
    }
};

class UILayout {
   public:
    UILayout() = default;
    UILayout(UINode root) : _root(root) {}

    struct Context {
        glm::ivec2 screenSize;
    };

    void update(UINode root) {
        _root = root;
    }

    void layout(const Context& context);

    Option<LayoutRect> getLayout(UINode::ID nodeID) const;
    Option<LayoutRect> getLayout(const UINode& node) const {
        return getLayout(node.id);
    };

    void toString(std::stringstream& ss) const;

   private:
    UINode _root;
    std::unordered_map<UINode::ID, LayoutRect> _layoutMap;

    void computeFixedSizes(UINode& node, LayoutRect frame);
    void computeFitSizes(UINode& node, LayoutRect frame);
    void computePositions(UINode& node, LayoutRect frame);

    LayoutRect& getOrMakeRect(const UINode& node);
    LayoutRect getRect(const UINode& node) const {
        return _layoutMap.at(node.id);
    }

    int computeSize(ElementRealSize size, int parentSize) const;

    int marginMainStart(const UIElement& element, UIAxis axis) const;
    int marginMainEnd(const UIElement& element, UIAxis axis) const;
    int marginCrossStart(const UIElement& element, UIAxis axis) const;
    int marginCrossEnd(const UIElement& element, UIAxis axis) const;
    int marginMainTotal(const UIElement& element, UIAxis axis) const;
    int marginCrossTotal(const UIElement& element, UIAxis axis) const;

    Option<int> computeElementSizeAlongAxis(
        const UINode& node, UIAxis axis, LayoutRect frame) const;

    void toString(std::stringstream& ss, const UINode& node, int indent = 0) const;
};

class UI {
   public:
    UI();
    UI(UIElement root);

   public:
    void render(glm::vec2 screenPosition, SystemParameter<Renderer> renderer = nullptr);
    void update(UIElement newRoot);

   private:
    UINode _root;
    UINode::ID _nextNodeID{1};

    struct NodeRenderInfo {
        RenderEntity rectEntity;
        RenderEntity textEntity;
        std::size_t contentHash;
        bool entityCreated{false};
        TextMeshBuffer::TextMeshHandle textMeshHandle{
            TextMeshBuffer::TextMeshHandle::invalidHandle()};
    };

    UINode createNodeFromElement(const UIElement& element, UINode::ID parentID) {
        UINode node;
        node.id = _nextNodeID++;
        node.element = element;
        for (const UIElement& childElement : element.children) {
            node.children.push_back(createNodeFromElement(childElement, node.parentID));
        }
        return node;
    }

    NodeRenderInfo& getNodeRenderInfo(const UINode& node) {
        return getNodeRenderInfo(node.id);
    }

    NodeRenderInfo& getNodeRenderInfo(UINode::ID nodeID);

    void renderNode(const UINode& node, Renderer& renderer, int depth = 0);
    void createNodeRenderEnties(const UINode& node, Renderer& renderer);
    void createRectRenderEntity(const UINode& node, Renderer& renderer);
    void createTextRenderEntity(const UINode& node, Renderer& renderer);

    Mesh getTextMesh(const UINode& node, Renderer& renderer);

    UILayout _layout;
    std::unordered_map<UINode::ID, NodeRenderInfo> _nodeRenderInfo;
    TextMeshBuffer _textMeshBuffer;
};

};  // namespace okay

#endif  // __UI_H__
