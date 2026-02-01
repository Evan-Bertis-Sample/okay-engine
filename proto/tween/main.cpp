#include <memory>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>

#include <utility>
#include <okay/core/tween/okay_tween.hpp>
#include <okay/core/tween/okay_tween_easing.hpp>
#include <okay/core/tween/okay_tween_engine.hpp>
#include <okay/core/tween/okay_tween_sequence.hpp>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

std::shared_ptr<okay::OkayTween<float>> tween1;
std::shared_ptr<okay::OkayTween<float>> tween2;

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
                    std::make_unique<okay::OkayTweenEngine>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

okay::OkayTweenSequence seq;

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");

    // DEMO
    
    tween1 = okay::OkayTween<float>::create(-0.5f, 0.5f, 2000, okay::easing::cubicOut, 
        0, true, 0, [&]() {
            auto* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
            renderer->setBoxPosition(glm::vec3(tween1->value(), 0.0f, 0.0f));
        });
    tween2 = okay::OkayTween<float>::create(-0.5f, 0.5f, 2000, okay::easing::bounceIn, 
        0, true, 0, [&]() {
            auto* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
            renderer->setBoxPosition(glm::vec3(tween2->value(), 0.0f, 0.0f));
        });
    seq.append(tween1);
    seq.append(tween2);

    seq.start();
}

static void __gameUpdate() {
    seq.tick();
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
