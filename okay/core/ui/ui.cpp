#include "ui.hpp"

#include "element.hpp"
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
        .axis = UIPrimaryAxis::Vertical,
        .resolvedSize = true,
    };

    LayoutRect& rootRect = getOrMakeRect(_root);
    rootRect = rootFrame;

    computeFixedSizes(_root, rootFrame);
    computeFitSizes(_root, rootFrame);
    computePositions(_root, rootFrame);
}

void UILayout::computeFixedSizes(UINode& node, LayoutRect parent) {
    LayoutRect& rect = getOrMakeRect(node);
    const UIElement& element = node.element;

    UIPrimaryAxis resolvedAxis = element.axis == UIPrimaryAxis::Parent ? parent.axis : element.axis;
    rect.axis = resolvedAxis;

    // Root already has a frame; children get theirs from parent.
    if (node.id != _root.id) {
        rect.pxPosition = parent.pxPosition;
    }

    int minWidth = computeSize(element.minWidth, parent.pxSize.x);
    int minHeight = computeSize(element.minHeight, parent.pxSize.y);

    Option<int> width = computeElementSizeAlongAxis(node, UIPrimaryAxis::Horizontal, parent);
    Option<int> height = computeElementSizeAlongAxis(node, UIPrimaryAxis::Vertical, parent);

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

UI::UI(UIElement root) {
    _root = createNodeFromElement(root);
    _layout = UILayout(_root);
}

void UILayout::computeFitSizes(UINode& node, LayoutRect parent) {
    LayoutRect& rect = getOrMakeRect(node);
    const UIElement& element = node.element;

    UIPrimaryAxis mainAxis = rect.axis;
    UIPrimaryAxis crossAxis = (mainAxis == UIPrimaryAxis::Horizontal) ? UIPrimaryAxis::Vertical
                                                                      : UIPrimaryAxis::Horizontal;

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

        if (crossAxis == UIPrimaryAxis::Horizontal) {
            if (childRect.pxSize.x == 0) {
                if (std::holds_alternative<size::Grow>(childCrossDecl)) {
                    int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
                    childRect.pxSize.x = std::max(contentWidth, minWidth);
                } else if (std::holds_alternative<size::Percent>(childCrossDecl)) {
                    float percent = std::get<size::Percent>(childCrossDecl).percent;
                    int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
                    childRect.pxSize.x = std::max(
                        static_cast<int>(percent * static_cast<float>(contentWidth)), minWidth);
                }
            }
        } else {
            if (childRect.pxSize.y == 0) {
                if (std::holds_alternative<size::Grow>(childCrossDecl)) {
                    int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
                    childRect.pxSize.y = std::max(contentHeight, minHeight);
                } else if (std::holds_alternative<size::Percent>(childCrossDecl)) {
                    float percent = std::get<size::Percent>(childCrossDecl).percent;
                    int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
                    childRect.pxSize.y = std::max(
                        static_cast<int>(percent * static_cast<float>(contentHeight)), minHeight);
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

    for (UINode& child : node.children) {
        LayoutRect& childRect = getOrMakeRect(child);

        int childMain =
            (mainAxis == UIPrimaryAxis::Horizontal) ? childRect.pxSize.x : childRect.pxSize.y;
        int childCross =
            (crossAxis == UIPrimaryAxis::Horizontal) ? childRect.pxSize.x : childRect.pxSize.y;

        ElementSize childMainDecl = child.element.getSizeAlongAxis(mainAxis);
        if (std::holds_alternative<size::Grow>(childMainDecl)) {
            growChildren++;
        } else {
            summedMain += childMain;
        }

        maxCross = std::max(maxCross, childCross);
    }

    // Resolve this node's own fit-from-children size.
    ElementSize widthDecl = element.width;
    ElementSize heightDecl = element.height;

    if (std::holds_alternative<size::Fit>(widthDecl)) {
        if (mainAxis == UIPrimaryAxis::Horizontal) {
            rect.pxSize.x = std::max(rect.pxSize.x, summedMain + totalSpacing + horizontalPadding);
        } else {
            rect.pxSize.x = std::max(rect.pxSize.x, maxCross + horizontalPadding);
        }
    }

    if (std::holds_alternative<size::Fit>(heightDecl)) {
        if (mainAxis == UIPrimaryAxis::Vertical) {
            rect.pxSize.y = std::max(rect.pxSize.y, summedMain + totalSpacing + verticalPadding);
        } else {
            rect.pxSize.y = std::max(rect.pxSize.y, maxCross + verticalPadding);
        }
    }

    // Recompute content size in case fit sizing changed this node.
    contentWidth = std::max(0, rect.pxSize.x - horizontalPadding);
    contentHeight = std::max(0, rect.pxSize.y - verticalPadding);

    // Distribute remaining space to grow children along the main axis.
    int contentMainSize = (mainAxis == UIPrimaryAxis::Horizontal) ? contentWidth : contentHeight;
    int remainingMain = std::max(0, contentMainSize - summedMain - totalSpacing);
    int growShare = (growChildren > 0) ? (remainingMain / growChildren) : 0;

    for (UINode& child : node.children) {
        LayoutRect& childRect = getOrMakeRect(child);
        ElementSize childMainDecl = child.element.getSizeAlongAxis(mainAxis);

        if (!std::holds_alternative<size::Grow>(childMainDecl)) {
            continue;
        }

        if (mainAxis == UIPrimaryAxis::Horizontal) {
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

    UIPrimaryAxis mainAxis = rect.axis;
    UIPrimaryAxis crossAxis = (mainAxis == UIPrimaryAxis::Horizontal) ? UIPrimaryAxis::Vertical
                                                                      : UIPrimaryAxis::Horizontal;

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

        if (mainAxis == UIPrimaryAxis::Horizontal) {
            childRect.pxPosition.x = contentX + cursor;
            childRect.pxPosition.y = contentY;

            // Stretch unresolved cross axis to content height.
            if (childRect.pxSize.y == 0) {
                int minHeight = computeSize(child.element.minHeight, rect.pxSize.y);
                childRect.pxSize.y = std::max(contentHeight, minHeight);
            }

            cursor += childRect.pxSize.x + childSpacing;
        } else {
            childRect.pxPosition.x = contentX;
            childRect.pxPosition.y = contentY + cursor;

            // Stretch unresolved cross axis to content width.
            if (childRect.pxSize.x == 0) {
                int minWidth = computeSize(child.element.minWidth, rect.pxSize.x);
                childRect.pxSize.x = std::max(contentWidth, minWidth);
            }

            cursor += childRect.pxSize.y + childSpacing;
        }

        computePositions(child, childRect);
    }
}

Option<int> UILayout::computeElementSizeAlongAxis(const UINode& node,
                                                  UIPrimaryAxis axis,
                                                  LayoutRect frame) const {
    const UIElement& element = node.element;
    ElementSize size = element.getSizeAlongAxis(axis);

    int parentSize = (axis == UIPrimaryAxis::Horizontal) ? frame.pxSize.x : frame.pxSize.y;

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
            resolved = std::max(resolved,
                                axis == UIPrimaryAxis::Horizontal ? (int)layout.metrics().width()
                                                                  : (int)layout.metrics().height());
        }

        if (element.backgroundImage.isSome()) {
            Texture texture = element.backgroundImage.value();
            resolved = std::max(resolved,
                                axis == UIPrimaryAxis::Horizontal ? (int)texture.getMeta().width
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

    for (const UINode& child : node.children) {
        ss << '\n';
        toString(ss, child, indent + 2);
    }
}

// UI

void UI::render(glm::vec2 screenPosition, SystemParameter<Renderer> renderer) {
    UILayout::Context layoutContext{
        .screenSize = glm::ivec2(renderer->width(), renderer->height()),
    };
    _layout.layout(layoutContext);
    renderNode(_root, *renderer);
}

void UI::renderNode(const UINode& node, Renderer& renderer) {
    NodeRenderInfo& renderInfo = getNodeRenderInfo(node);
    if (!renderInfo.entityCreated) {
        Engine.logger.debug("Creating render entities for UI node {}", node.id);
        createNodeRenderEnties(node, renderer);
    }

    Option<LayoutRect> layout = _layout.getLayout(node);
    if (!layout.isSome()) {
        Engine.logger.warn("No layout found for UI node {}, skipping render", node.id);
        return;
    }

    const LayoutRect rect = layout.value();
    const glm::vec2 screenSize = glm::vec2(renderer.width(), renderer.height());

    // Convert from top-left pixel coordinates to NDC.
    auto pixelToNdc = [&](const glm::vec2& px) -> glm::vec2 {
        return glm::vec2((px.x / screenSize.x) * 2.0f - 1.0f, 1.0f - (px.y / screenSize.y) * 2.0f);
    };

    // Number of NDC units per pixel.
    const glm::vec2 pxToNdcScale = glm::vec2(2.0f / screenSize.x, 2.0f / screenSize.y);

    if (renderInfo.textureEntity.isValid()) {
        // Background quad is centered at origin and has size 1x1 in mesh space.
        // So place it at the center of the layout rect and scale by rect size in NDC.
        const glm::vec2 rectCenterPx = glm::vec2(rect.pxPosition) + glm::vec2(rect.pxSize) * 0.5f;
        const glm::vec2 rectCenterNdc = pixelToNdc(rectCenterPx);
        const glm::vec2 rectSizeNdc = glm::vec2(rect.pxSize) * pxToNdcScale;

        RenderEntity::Properties props = renderInfo.textureEntity.prop();
        props.transform.position = glm::vec3(rectCenterNdc, props.transform.position.z);
        props.transform.scale = glm::vec3(rectSizeNdc, 1.0f);

        // Engine.logger.debug("Rendering texture with transform {}", props.transform);
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
        props.transform.position = glm::vec3(textOriginNdc, props.transform.position.z);
        props.transform.scale = glm::vec3(pxToNdcScale.x, pxToNdcScale.y, 1.0f);

        // Engine.logger.debug("Rendering text with transform {}", props.transform);
    }

    for (const UINode& child : node.children) {
        renderNode(child, renderer);
    }
}

void UI::createNodeRenderEnties(const UINode& node, Renderer& renderer) {
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

        // Create the render entity
        RenderEntity entity =
            renderer.world().addRenderEntity(glm::vec3(1.0f, 1.0f, z), handle, bgMesh);

        renderInfo.textureEntity = entity;

        z += zIncrement;
    }

    if (element.text.isSome()) {
        Engine.logger.debug("Creating render entity for UI element with text {}",
                            element.text.value());
        // render text
        MaterialHandle handle = UIRenderResoruces::get().getMaterial(element).value();
        TextStyle style = element.textStyle;

        if (!FontManager::instance().isLoadedFont(style.font)) {
            style.font = FontManager::instance().defaultFont();
        }

        MeshData textMeshData =
            TextMeshBuilder::build(element.text.value(), style, element.doubleSided);
        Mesh textMesh = renderer.meshBuffer().addMesh(textMeshData);

        // Create the render entity, using a default position
        RenderEntity entity =
            renderer.world().addRenderEntity(glm::vec3(1.0f, 1.0f, z), handle, textMesh);

        renderInfo.textEntity = entity;

        z += zIncrement;
    }

    renderInfo.entityCreated = true;
};

}  // namespace okay