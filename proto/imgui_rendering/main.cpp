#include <cstddef>
#include <functional>
#include <memory>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/renderer/imgui/okay_imgui.hpp>

#include <utility>
#include <okay/core/tween/okay_tween.hpp>
#include <okay/core/tween/okay_tween_easing.hpp>
#include <okay/core/tween/okay_tween_engine.hpp>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

int main() {
    okay::SurfaceConfig surfaceConfig;
    okay::Surface surface(surfaceConfig);
    
    okay::OkayRendererSettings rendererSettings{surfaceConfig};
    auto renderer = okay::OkayRenderer::create(rendererSettings);

    okay::OkayLevelManagerSettings levelManagerSettings;
    auto levelManager = okay::OkayLevelManager::create(levelManagerSettings);

    okay::OkayGame::create()
        .addSystems(std::move(renderer), std::move(levelManager),
                    std::make_unique<okay::OkayAssetManager>(),
                    std::make_unique<okay::OkayTweenEngine>(),
                    std::make_unique<okay::OkayIMGUI>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");
}

static void __gameUpdate() {
    // Game update logic
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
