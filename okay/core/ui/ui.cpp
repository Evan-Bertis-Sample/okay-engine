#include "ui.hpp"

#include "element.hpp"
#include "glm/fwd.hpp"
#include "render_resources.hpp"
#include "text_mesh_builder.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/util/format.hpp>
#include <okay/core/util/variant.hpp>

#include <variant>

namespace okay {

void UILayout::layout(const Context& context) {
    _layoutMap.clear();

    LayoutRect rootFrame{
        .pxPosition = glm::ivec2(0, 0),
        .pxSize = context.screenSize,
        .axis = UIAxis::Vertical,
        .resolvedSize = true,
    };

    LayoutRect& rootRect = getOrMakeRect(_root);
    rootRect = rootFrame;

    computeFixedSizes(_root, rootFrame);
    computeFitSizes(_root, rootFrame);
    computePositions(_root, rootFrame);
}

int UILayout::marginMainStart(const UIElement& element, UIAxis axis) const {
    return axis == UIAxis::Horizontal ? element.leftMargin.pixels : element.topMargin.pixels;
}

int UILayout::marginMainEnd(const UIElement& element, UIAxis axis) const {
    return axis == UIAxis::Horizontal ? element.rightMargin.pixels : element.bottomMargin.pixels;
}

int UILayout::marginCrossStart(const UIElement& element, UIAxis axis) const {
    return axis == UIAxis::Horizontal ? element.topMargin.pixels : element.leftMargin.pixels;
}

int UILayout::marginCrossEnd(const UIElement& element, UIAxis axis) const {
    return axis == UIAxis::Horizontal ? element.bottomMargin.pixels : element.rightMargin.pixels;
}

int UILayout::marginMainTotal(const UIElement& element, UIAxis axis) const {
    return marginMainStart(element, axis) + marginMainEnd(element, axis);
}

int UILayout::marginCrossTotal(const UIElement& element, UIAxis axis) const {
    return marginCrossStart(element, axis) + marginCrossEnd(element, axis);
}

void UILayout::computeFixedSizes(UINode& node, LayoutRect parent) {
    LayoutRect& rect = getOrMakeRect(node);
    const UIElement& element = node.element;

    UIAxis resolvedAxis = element.axis == UIAxis::Parent ? parent.axis : element.axis;
    rect.axis = resolvedAxis;

    // Root already has a frame; children get theirs from parent.
    if (node.id != _root.id) {
        rect.pxPosition = parent.pxPosition;
    }

    int minWidth = computeSize(element.minWidth, parent.pxSize.x);
    int minHeight = computeSize(element.minHeight, parent.pxSize.y);

    Option<int> width = computeElementSizeAlongAxis(node, UIAxis::Horizontal, parent);
    Option<int> height = computeElementSizeAlongAxis(node, UIAxis::Vertical, parent);

    if (width.isSome()) {
        rect.pxSize.x = std::max(width.value(), minWidth);
    }

    if (height.isSome()) {
        rect.pxSize.y = std::max(height.value(), minHeight);
    }

    // Root should keep full screen size unless explicitly overridden later.
    if (node.id == _root.id) {
        rect.pxSize = parent.pxSize;
    }

    for (UINode& child : node.children) {
        computeFixedSizes(child, rect);
    }
}

UI::UI() : _textMeshBuffer() {}

UI::UI(UIElement root) : _textMeshBuffer() {
    _root = createNodeFromElement(root, 0);
    _layout = UILayout(_root);
}

void UILayout::computeFitSizes(UINode& node, LayoutRect parent) {
    LayoutRect& rect = getOrMakeRect(node);
    const UIElement& element = node.element;

    UIAxis mainAxis = rect.axis;
    UIAxis crossAxis = (mainAxis == UIAxis::Horizontal) ? UIAxis::Vertical : UIAxis::Horizontal;

    int leftPadding = element.leftPadding.pixels;
    int rightPadding = element.rightPadding.pixels;
    int topPadding = element.topPadding.pixels;
    int bottomPadding = element.bottomPadding.pixels;
    int childSpacing = element.childSpacing.pixels;

    int horizontalPadding = leftPadding + rightPadding;
    int verticalPadding = topPadding + bottomPadding;

    int contentWidth = std::max(0, rect.pxSize.x - horizontalPadding);
    int contentHeight = std::max(0, rect.pxSize.y - verticalPadding);

    // First, resolve child cross-axis stretch before recursing.
    // This is the key fix: a child container may need its cross-axis size
    // in order to lay out its own children.
    for (UINode& child : node.children) {
        LayoutRect& childRect = getOrMakeRect(child);
        ElementSize childCrossDecl = child.element.getSizeAlongAxis(crossAxis);

        if (crossAxis == UIAxis::Horizontal) {
            if (childRect.pxSize.x == 0) {
                if (std::holds_alternative<size::Grow>(childCrossDecl)) {
                    int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
                    int childContentWidth =
                        std::max(0, contentWidth - marginCrossTotal(child.element, mainAxis));
                    childRect.pxSize.x = std::max(childContentWidth, minWidth);
                } else if (std::holds_alternative<size::Percent>(childCrossDecl)) {
                    float percent = std::get<size::Percent>(childCrossDecl).percent;
                    int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
                    int childContentWidth =
                        std::max(0, contentWidth - marginCrossTotal(child.element, mainAxis));
                    childRect.pxSize.x =
                        std::max(static_cast<int>(percent * static_cast<float>(childContentWidth)),
                            minWidth);
                }
            }
        } else {
            if (childRect.pxSize.y == 0) {
                if (std::holds_alternative<size::Grow>(childCrossDecl)) {
                    int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
                    int childContentHeight =
                        std::max(0, contentHeight - marginCrossTotal(child.element, mainAxis));
                    childRect.pxSize.y = std::max(childContentHeight, minHeight);
                } else if (std::holds_alternative<size::Percent>(childCrossDecl)) {
                    float percent = std::get<size::Percent>(childCrossDecl).percent;
                    int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
                    int childContentHeight =
                        std::max(0, contentHeight - marginCrossTotal(child.element, mainAxis));
                    childRect.pxSize.y =
                        std::max(static_cast<int>(percent * static_cast<float>(childContentHeight)),
                            minHeight);
                }
            }
        }
    }

    // Now recurse, once children have enough size context to solve themselves.
    for (UINode& child : node.children) {
        computeFitSizes(child, rect);
    }

    int childCount = static_cast<int>(node.children.size());
    int totalSpacing = childCount > 1 ? (childCount - 1) * childSpacing : 0;

    int summedMain = 0;
    int maxCross = 0;
    int growChildren = 0;
    int growMargins = 0;

    for (UINode& child : node.children) {
        LayoutRect& childRect = getOrMakeRect(child);

        int childMain = (mainAxis == UIAxis::Horizontal) ? childRect.pxSize.x : childRect.pxSize.y;
        int childCross =
            (crossAxis == UIAxis::Horizontal) ? childRect.pxSize.x : childRect.pxSize.y;

        childMain += marginMainTotal(child.element, mainAxis);
        childCross += marginCrossTotal(child.element, mainAxis);

        ElementSize childMainDecl = child.element.getSizeAlongAxis(mainAxis);
        if (std::holds_alternative<size::Grow>(childMainDecl)) {
            growChildren++;
            growMargins += marginMainTotal(child.element, mainAxis);
        } else {
            summedMain += childMain;
        }

        maxCross = std::max(maxCross, childCross);
    }

    // Resolve this node's own fit-from-children size.
    ElementSize widthDecl = element.width;
    ElementSize heightDecl = element.height;

    if (std::holds_alternative<size::Fit>(widthDecl)) {
        if (mainAxis == UIAxis::Horizontal) {
            rect.pxSize.x = std::max(rect.pxSize.x, summedMain + totalSpacing + horizontalPadding);
        } else {
            rect.pxSize.x = std::max(rect.pxSize.x, maxCross + horizontalPadding);
        }
    }

    if (std::holds_alternative<size::Fit>(heightDecl)) {
        if (mainAxis == UIAxis::Vertical) {
            rect.pxSize.y = std::max(rect.pxSize.y, summedMain + totalSpacing + verticalPadding);
        } else {
            rect.pxSize.y = std::max(rect.pxSize.y, maxCross + verticalPadding);
        }
    }

    // Recompute content size in case fit sizing changed this node.
    contentWidth = std::max(0, rect.pxSize.x - horizontalPadding);
    contentHeight = std::max(0, rect.pxSize.y - verticalPadding);

    // Distribute remaining space to grow children along the main axis.
    int contentMainSize = (mainAxis == UIAxis::Horizontal) ? contentWidth : contentHeight;
    int remainingMain = std::max(0, contentMainSize - summedMain - growMargins - totalSpacing);
    int growShare = (growChildren > 0) ? (remainingMain / growChildren) : 0;

    for (UINode& child : node.children) {
        LayoutRect& childRect = getOrMakeRect(child);
        ElementSize childMainDecl = child.element.getSizeAlongAxis(mainAxis);

        if (!std::holds_alternative<size::Grow>(childMainDecl)) {
            continue;
        }

        if (mainAxis == UIAxis::Horizontal) {
            int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
            childRect.pxSize.x = std::max(growShare, minWidth);
        } else {
            int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
            childRect.pxSize.y = std::max(growShare, minHeight);
        }
    }
}

void UILayout::computePositions(UINode& node, LayoutRect parent) {
    LayoutRect& rect = getOrMakeRect(node);
    const UIElement& element = node.element;

    UIAxis mainAxis = rect.axis;
    UIAxis crossAxis = (mainAxis == UIAxis::Horizontal) ? UIAxis::Vertical : UIAxis::Horizontal;

    int leftPadding = element.leftPadding.pixels;
    int rightPadding = element.rightPadding.pixels;
    int topPadding = element.topPadding.pixels;
    int bottomPadding = element.bottomPadding.pixels;
    int childSpacing = element.childSpacing.pixels;

    int contentX = rect.pxPosition.x + leftPadding;
    int contentY = rect.pxPosition.y + topPadding;

    int contentWidth = std::max(0, rect.pxSize.x - leftPadding - rightPadding);
    int contentHeight = std::max(0, rect.pxSize.y - topPadding - bottomPadding);

    int cursor = 0;

    for (UINode& child : node.children) {
        LayoutRect& childRect = getOrMakeRect(child);

        int mainStart = marginMainStart(child.element, mainAxis);
        int mainEnd = marginMainEnd(child.element, mainAxis);
        int crossStart = marginCrossStart(child.element, mainAxis);
        int crossEnd = marginCrossEnd(child.element, mainAxis);

        if (mainAxis == UIAxis::Horizontal) {
            childRect.pxPosition.x = contentX + cursor + mainStart;
            childRect.pxPosition.y = contentY + crossStart;

            // Stretch unresolved cross axis to content height.
            if (childRect.pxSize.y == 0) {
                int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
                childRect.pxSize.y =
                    std::max(std::max(0, contentHeight - crossStart - crossEnd), minHeight);
            }

            cursor += mainStart + childRect.pxSize.x + mainEnd + childSpacing;
        } else {
            childRect.pxPosition.x = contentX + crossStart;
            childRect.pxPosition.y = contentY + cursor + mainStart;

            // Stretch unresolved cross axis to content width.
            if (childRect.pxSize.x == 0) {
                int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
                childRect.pxSize.x =
                    std::max(std::max(0, contentWidth - crossStart - crossEnd), minWidth);
            }

            cursor += mainStart + childRect.pxSize.y + mainEnd + childSpacing;
        }

        computePositions(child, childRect);
    }
}

Option<int> UILayout::computeElementSizeAlongAxis(
    const UINode& node, UIAxis axis, LayoutRect frame) const {
    const UIElement& element = node.element;
    ElementSize size = element.getSizeAlongAxis(axis);

    int parentSize = (axis == UIAxis::Horizontal) ? frame.pxSize.x : frame.pxSize.y;

    if (std::holds_alternative<size::Fixed>(size)) {
        return std::get<size::Fixed>(size).pixels;
    }

    if (std::holds_alternative<size::Percent>(size)) {
        float percent = std::get<size::Percent>(size).percent;
        return static_cast<int>(percent * static_cast<float>(parentSize));
    }

    if (std::holds_alternative<size::Grow>(size)) {
        return Option<int>::none();
    }

    if (std::holds_alternative<size::Fit>(size)) {
        int resolved = 0;

        if (element.text.isSome()) {
            TextStyle style = element.textStyle;

            TextLayout layout(element.text.value(), style);
            const float glyphScale = style.fontHeight / layout.metrics().lineHeight;
            const float textWidth = layout.metrics().width() * glyphScale;
            const float textHeight = layout.metrics().height() * glyphScale;

            resolved = std::max(resolved,
                axis == UIAxis::Horizontal ? static_cast<int>(std::ceil(textWidth))
                                           : static_cast<int>(std::ceil(textHeight)));
        }

        if (element.backgroundImage.isSome()) {
            Texture texture = element.backgroundImage.value();
            resolved = std::max(resolved,
                axis == UIAxis::Horizontal ? (int)texture.getMeta().width
                                           : (int)texture.getMeta().height);
        }

        // Only intrinsic leaf fit is resolved here.
        // Container fit is resolved later from child rects.
        if (resolved > 0 || node.children.empty()) {
            return resolved;
        }

        return Option<int>::none();
    }

    return Option<int>::none();
}

void UILayout::toString(std::stringstream& ss) const {
    toString(ss, _root, 0);
}

void UILayout::toString(std::stringstream& ss, const UINode& node, int indent) const {
    ss << std::string(indent, ' ') << node.id << ':';

    LayoutRect rect = getRect(node);

    ss << " pos=" << rect.pxPosition.x << ',' << rect.pxPosition.y;
    ss << " size=" << rect.pxSize.x << ',' << rect.pxSize.y;
    if (node.element.text.isSome()) {
        ss << " text='" << node.element.text.value() << "'";
    }

    if (node.element.backgroundColor.a > 0.0f) {
        ss << " bg=" << node.element.backgroundColor.r << ',' << node.element.backgroundColor.g
           << ',' << node.element.backgroundColor.b << ',' << node.element.backgroundColor.a;
    }

    for (const UINode& child : node.children) {
        ss << '\n';
        toString(ss, child, indent + 2);
    }
}

// UI

void UI::render(
    glm::vec2 screenPosition, std::uint8_t uiLayer, SystemParameter<Renderer> renderer) {
    UILayout::Context layoutContext{
        .screenSize = glm::ivec2(renderer->width(), renderer->height()),
    };
    _layout.layout(layoutContext);
    renderNode(_root, *renderer, (0x1 << 7) + 1 + (uiLayer * 24));
}

void UI::update(UIElement newRoot) {
    // reset node ID
    // Engine.logger.debug("Updating UI!");
    _nextNodeID = 1;
    _root = createNodeFromElement(newRoot, 0);
    _layout.update(_root);
}

void UI::renderNode(const UINode& node, Renderer& renderer, int layerBase) {
    NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
    const UIElement& element = node.element;
    std::size_t elementContentHash = node.element.contentHash();

    if (!renderInfo.entityCreated) {
        renderInfo.contentHash = elementContentHash;
        createNodeRenderEnties(node, renderer);
    }

    if (renderInfo.contentHash != elementContentHash) {
        // Engine.logger.debug("Creating render entities for UI node {}", node.id);
        if (renderInfo.rectEntity.isValid()) {
            Engine.logger.debug("updateing render entity rect");
            renderer.world().removeRenderEntity(renderInfo.rectEntity);
            createRectRenderEntity(node, renderer);
        } else {
            createRectRenderEntity(node, renderer);
        }

        if (renderInfo.textEntity.isValid()) {
            // Replace the text mesh
            RenderEntity::Properties props = renderInfo.textEntity.prop();
            props.mesh = getTextMesh(node, renderer);
            Option<MaterialHandle> material = UIRenderResoruces::get().getTextMaterial(element);
            if (material.isSome()) {
                props.material = material.value();
            }

        } else {
            createTextRenderEntity(node, renderer);
        }

        renderInfo.contentHash = elementContentHash;
    }

    Option<LayoutRect> layout = _layout.getLayout(node);
    if (!layout.isSome()) {
        Engine.logger.warn("No layout found for UI node {}, skipping render", node.id);
        return;
    }
    const LayoutRect rect = layout.value();
    const glm::vec2 screenSize = glm::vec2(renderer.width(), renderer.height());

    Option<LayoutRect> parentLayout = _layout.getLayout(node.parentID);
    const LayoutRect parentRect =
        (parentLayout.isSome()) ? parentLayout.value()
                                : LayoutRect{.pxPosition = glm::ivec2{0, 0},
                                      .pxSize = glm::ivec2{renderer.width(), renderer.height()}};

    // Convert from top-left pixel coordinates to NDC.
    auto pixelToNdc = [&](const glm::vec2& px) -> glm::vec2 {
        return glm::vec2((px.x / screenSize.x) * 2.0f - 1.0f, 1.0f - (px.y / screenSize.y) * 2.0f);
    };

    // Number of NDC units per pixel.
    const glm::vec2 pxToNdcScale = glm::vec2(2.0f / screenSize.x, 2.0f / screenSize.y);

    if (renderInfo.rectEntity.isValid()) {
        // Background quad is centered at origin and has size 1x1 in mesh space.
        // So place it at the center of the layout rect and scale by rect size in NDC.
        const glm::vec2 rectCenterPx = glm::vec2(rect.pxPosition) + glm::vec2(rect.pxSize) * 0.5f;
        const glm::vec2 rectCenterNdc = pixelToNdc(rectCenterPx);
        const glm::vec2 rectSizeNdc = glm::vec2(rect.pxSize) * pxToNdcScale;

        RenderEntity::Properties props = renderInfo.rectEntity.prop();
        props.transform.position = glm::vec3(rectCenterNdc, 0.0f);
        props.transform.scale = glm::vec3(rectSizeNdc, 1.0f);
        props.renderLayer = layerBase + 0;

        // update the material properties
        const glm::vec2 pxToUV = glm::vec2(1.0f / rect.pxSize.x, 1.0f / rect.pxSize.y);

        if (UIRenderResoruces::RectMaterial* props = dynamic_cast<UIRenderResoruces::RectMaterial*>(
                renderInfo.rectEntity->material->properties().get())) {
            props->borderRadius = pxToUV * static_cast<float>(node.element.borderRadius.pixels);
            props->borderWidth = pxToUV * static_cast<float>(node.element.borderWidth.pixels);
            props->borderColor = node.element.borderColor;

            // TODO: Figure out a way to pass clipping bounds not based on material
            // UIElements share a material when possible, therefore this doesn't work

            // if (element.clippingMode == UIClippingMode::Clip_Overflow) {
            //     props->clipSpaceTL = parentRect.topLeft();
            //     props->clipSpaceBR = parentRect.bottomRight();

            // } else {
            //     props->clipSpaceBR = glm::vec2(1.0f, -1.0f);
            //     props->clipSpaceTL = glm::vec2(-1.0f, 1.0f);
            // }
        }
    }

    if (renderInfo.textEntity.isValid()) {
        // Text mesh units are already pixels.
        // So the transform scale should only convert pixels -> NDC.
        //
        // The origin of the text mesh
        // is dependent upon the vertical and horizontal alignment of the text.
        // For example, a top-left aligned text mesh has its origin at the top-left most vertex of
        // the mesh A bottom-right aligned text mesh has its origin at the bottom-right most vertex
        // of the mesh A center aligned text mesh has its origin at the center of the mesh

        glm::vec2 textOriginPx = glm::vec2(rect.pxPosition);

        switch (node.element.textStyle.verticalAlignment) {
            case TextStyle::VerticalAlignment::Top:
                break;
            case TextStyle::VerticalAlignment::Middle:
                textOriginPx.y += rect.pxSize.y * 0.5f;
                break;
            case TextStyle::VerticalAlignment::Bottom:
                textOriginPx.y += rect.pxSize.y;
                break;
        }

        switch (node.element.textStyle.horizontalAlignment) {
            case TextStyle::HorizontalAlignment::Left:
                break;
            case TextStyle::HorizontalAlignment::Center:
                textOriginPx.x += rect.pxSize.x * 0.5f;
                break;
            case TextStyle::HorizontalAlignment::Right:
                textOriginPx.x += rect.pxSize.x;
                break;
        }

        const glm::vec2 textOriginNdc = pixelToNdc(textOriginPx);

        RenderEntity::Properties props = renderInfo.textEntity.prop();
        props.transform.position = glm::vec3(textOriginNdc, 0.0f);
        props.transform.scale = glm::vec3(pxToNdcScale.x, pxToNdcScale.y, 1.0f);
        props.renderLayer = layerBase + 1;

        // TODO: Figure out a way to pass clipping bounds not based on material
        // UIElements share a material when possible, therefore this doesn't work

        // if (UIRenderResoruces::TextMaterial* props =
        // dynamic_cast<UIRenderResoruces::TextMaterial*>(
        //         renderInfo.textEntity->material->properties().get())) {
        //     if (element.clippingMode == UIClippingMode::Clip_Overflow) {
        //         props->clipSpaceTL = parentRect.topLeft();
        //         props->clipSpaceBR = parentRect.bottomRight();

        //     } else {
        //         props->clipSpaceBR = glm::vec2(1.0f, -1.0f);
        //         props->clipSpaceTL = glm::vec2(-1.0f, 1.0f);
        //     }
        // }
    }

    for (const UINode& child : node.children) {
        renderNode(child, renderer, layerBase + 2);
    }
}

void UI::createNodeRenderEnties(const UINode& node, Renderer& renderer) {
    NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
    createRectRenderEntity(node, renderer);
    createTextRenderEntity(node, renderer);
    renderInfo.entityCreated = true;
};

void UI::createRectRenderEntity(const UINode& node, Renderer& renderer) {
    NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
    const UIElement& element = node.element;

    if (element.backgroundImage.isSome() || element.backgroundColor.a > 0.0f) {
        // Engine.logger.debug("Creating render entity for UI element with background image");
        MaterialHandle handle = UIRenderResoruces::get().getRectMaterial(element).value();
        Mesh bgMesh = UIRenderResoruces::get().quadMesh();
        RenderEntity entity =
            renderer.world().addRenderEntity(glm::vec3(1.0f, 1.0f, 0.0f), handle, bgMesh);

        renderInfo.rectEntity = entity;
    }
}

void UI::createTextRenderEntity(const UINode& node, Renderer& renderer) {
    NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
    const UIElement& element = node.element;

    if (element.text.isSome()) {
        // Engine.logger.debug("Creating render entity for UI element with text
        // element.text.value()); render text
        MaterialHandle handle = UIRenderResoruces::get().getTextMaterial(element).value();
        Mesh textMesh = getTextMesh(node, renderer);

        // Create the render entity, using a default position
        RenderEntity entity =
            renderer.world().addRenderEntity(glm::vec3(1.0f, 1.0f, 0.0f), handle, textMesh);

        renderInfo.textEntity = entity;
    }
}

Mesh UI::getTextMesh(const UINode& node, Renderer& renderer) {
    const UIElement& element = node.element;
    if (element.text.isSome()) {
        TextStyle style = element.textStyle;
        if (!FontManager::instance().isLoadedFont(style.font)) {
            style.font = FontManager::instance().defaultFont();
        }

        NodeRenderInfo& renderInfo = getNodeRenderInfo(node);

        if (!_textMeshBuffer.isValidHandle(renderInfo.textMeshHandle)) {
            renderInfo.textMeshHandle = _textMeshBuffer.allocateHandle();
        }

        _textMeshBuffer.updateMesh(renderInfo.textMeshHandle, style, element.text.value());

        return _textMeshBuffer.getMesh(renderInfo.textMeshHandle);
    }

    return Mesh::none();
}

LayoutRect& UILayout::getOrMakeRect(const UINode& node) {
    if (!_layoutMap.contains(node.id)) {
        _layoutMap[node.id] = LayoutRect{};
    }
    return _layoutMap[node.id];
}

int UILayout::computeSize(ElementRealSize size, int parentSize) const {
    if (std::holds_alternative<size::Percent>(size)) {
        float percent = std::get<size::Percent>(size).percent;
        return static_cast<int>(percent * (float)parentSize);
    } else if (std::holds_alternative<size::Fixed>(size)) {
        return std::get<size::Fixed>(size).pixels;
    }
    return 0;
}

UI::NodeRenderInfo& UI::getNodeRenderInfo(UINode::ID nodeID) {
    if (!_nodeRenderInfo.contains(nodeID)) {
        _nodeRenderInfo[nodeID] = NodeRenderInfo{};
    }
    return _nodeRenderInfo[nodeID];
}

Option<LayoutRect> UILayout::getLayout(UINode::ID nodeID) const {
    if (!_layoutMap.contains(nodeID)) {
        return Option<LayoutRect>::none();
    }
    return _layoutMap.at(nodeID);
}

}  // namespace okay
