#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>

#include <utility>
#include "okay/core/system/okay_system.hpp"
#include "okay/core/tween/okay_tween.hpp"

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
                    std::make_unique<okay::OkayAssetManager>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

// class TweenEngine : public okay::OkaySystem<okay::OkaySystemScope::ENGINE> {

okay::OkayTween<std::float_t> testTween;

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");
    testTween = okay::OkayTween(0.0f, 4.0f);
    testTween.start();
}

static void __gameUpdate() {
    // std::cout << "Game updated." << std::endl;
    // Game update logic
    testTween.tick();
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
