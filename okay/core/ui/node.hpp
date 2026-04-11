#ifndef __NODE_H__
#define __NODE_H__

#include "render_resources.hpp"
#include "text_mesh_builder.hpp"

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
    UI() = default;
    UI(UIElement root) : _root{createNodeFromElement(root)} {}

   public:
    void render(glm::vec2 screenPosition, SystemParameter<Renderer> renderer) {
        renderNode(_root, screenPosition, glm::vec2(1.0f, 1.0f), renderer);
    }

    void renderNode(const UINode& node,
                    glm::vec2 screenPosition,
                    glm::vec2 size,
                    SystemParameter<Renderer> renderer) {
        NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
        if (!renderInfo.entityCreated) {
            Engine.logger.debug("Creating render entities for UI node {}", node.id);
            createNodeRenderEnties(node, screenPosition, size, renderer);
        }

        if (renderInfo.textureEntity.isValid()) {
            RenderEntity::Properties props = renderInfo.textureEntity.prop();
            props.transform.position = glm::vec3(screenPosition, props.transform.position.z);
        }

        if (renderInfo.textEntity.isValid()) {
            RenderEntity::Properties props = renderInfo.textEntity.prop();
            props.transform.position = glm::vec3(screenPosition, props.transform.position.z);
        }

        for (const UINode& child : node.children) {
            renderNode(child, screenPosition, size, renderer);
        }
    }

    void createNodeRenderEnties(const UINode& node,
                                glm::vec2 screenPosition,
                                glm::vec2 size,
                                SystemParameter<Renderer> renderer) {
        float z = 0.0f;
        const float zIncrement = 0.001f;  // small increment to ensure correct layering

        NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
        const UIElement& element = node.element;

        if (element.backgroundImage.isSome()) {
            Engine.logger.debug("Creating render entity for UI element with background image");
            // render background image
            Texture bgTexture = element.backgroundImage.value();
            MaterialHandle handle = UIRenderResoruces::get().getMaterial(element).value();
            Mesh bgMesh = UIRenderResoruces::get().quadMesh();
            RenderEntity entity =
                renderer->world().addRenderEntity(glm::vec3(screenPosition, z), handle, bgMesh);

            renderInfo.textureEntity = entity;

            z += zIncrement;
        }

        if (element.text.isSome()) {
            Engine.logger.debug("Creating render entity for UI element with text {}",
                                element.text.value());
            // render text
            MaterialHandle handle = UIRenderResoruces::get().getMaterial(element).value();
            TextStyle style;
            if (element.textStyle.isSome()) {
                style = element.textStyle.value();
            } else {
                style = UIRenderResoruces::get().defaultTextStyle();
            }

            MeshData textMeshData =
                TextMeshBuilder::build(element.text.value(), style, element.doubleSided);
            Mesh textMesh = renderer->meshBuffer().addMesh(textMeshData);
            RenderEntity entity =
                renderer->world().addRenderEntity(glm::vec3(screenPosition, z), handle, textMesh);

            renderInfo.textEntity = entity;

            z += zIncrement;
        }

        renderInfo.entityCreated = true;
    };

   private:
    UINode _root;
    UINode::ID _nextNodeID{1};

    struct NodeRenderInfo {
        RenderEntity textureEntity;
        RenderEntity textEntity;
        bool entityCreated = false;
    };

    UINode::ID createNodeIDFromElement(const UIElement& element) { return _nextNodeID++; }

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

    std::unordered_map<UINode::ID, NodeRenderInfo> _nodeRenderInfo;
};

};  // namespace okay

#endif  // __NODE_H__