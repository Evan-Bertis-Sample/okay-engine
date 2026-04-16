#ifndef __TEXT_COMPONENT_H__
#define __TEXT_COMPONENT_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/generic/font_loader.hpp>
#include <okay/core/ecs/ecs.hpp>
#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/ui/text.hpp>

#include <glm/glm.hpp>

namespace okay {

class TextRendererSystem;

struct TextRendererComponentConfig {};

// struct TextRendererComponent {
//    public:
//     using LoadT = AssetManager::Load<FontManager::FontHandle>;

//     TextRendererComponent(LoadT fontLoad, SystemParameter<AssetManager> assetManager = nullptr) {
//         Result<Asset<FontManager::FontHandle>> font = assetManager->loadAssetSync(fontLoad);
//         if (font.isError()) {
//             Engine.logger.error("Failed to load font: {}", font.error());
//         } else {
//             font = font.value().asset;
//         }
//     }

//     FontManager::FontHandle font{};
//     Mesh mesh{Mesh::none()};
//     MaterialHandle material{MaterialHandle::none()};
//     RenderEntity renderEntity;
// };

};  // namespace okay

#endif  // __TEXT_COMPONENT_H__