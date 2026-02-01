#include <concepts>
#include <memory>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>

#include <utility>
#include "okay/core/tween/okay_tween.hpp"
#include "okay/core/tween/okay_tween_easing.hpp"
#include "okay/core/tween/okay_tween_engine.hpp"
#include "okay/core/tween/okay_tween_sequence.hpp"

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

std::shared_ptr<okay::OkayTween<float>> tween1;

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
    // glm::vec3 vec1 {glm::vec3(1.0f, 2.0f, 3.0f)};
    // glm::vec3 vec2 {glm::vec3 (2.0f, 3.0f, 4.0f)};
    // std::float_t f1 { 0.0f };
    // std::float_t f2 { 1.0f };
    // okay::OkayTween(f1, f2, 1000, okay::easing::bounceInOut, 2, true).start();
    // okay::OkayTweenSequence seq;
    // auto tween1 { okay::OkayTween<std::float_t>::create(f1, f2, 100, okay::easing::bounceInOut) };
    // auto tween2 { okay::OkayTween<glm::vec3>::create(vec1, vec2, 100, okay::easing::backIn) };
    // seq.append(tween1);
    // seq.append(tween2);
    // seq.start();
    // int n1 { 30 };
    // int n2 { 30 };
    // int n3 { 30 };
    // okay::Engine.logger.debug("TweenEngine size init: {}", sizeof(okay::OkayTweenEngine));
    
    tween1 = okay::OkayTween<float>::create(-0.5f, 0.5f, 2000, okay::easing::cubicIn, 2, true);
    seq.append(tween1);
    // seq.append(okay::OkayTween<int>::create(n1, 2000, 2000));
    // seq.append(okay::OkayTween<int>::create(n2, 1000));
    // seq.append(okay::OkayTween<int>::create(n3, 400, 4000));
    seq.start();
    // okay::Engine.logger.debug("TweenEngine size started: {}", sizeof(okay::OkayTweenEngine));
    // okay::Engine.logger.debug("Tween size started: {}", sizeof(tween))
}

static void __gameUpdate() {
    // std::cout << "Game updated." << std::endl;
    // Game update logic
    // okay::Engine.logger.debug("TweenEngine size killed: {}", sizeof(okay::OkayTweenEngine));
    // static int aaa { 0 };

    // if (aaa == 50)
    // {
    //     okay::Engine.logger.debug("pausing!");
    //     seq.pause();
    // }
    
    // if (aaa == 100)
    // {
    //     okay::Engine.logger.debug("resuming!");
    //     seq.resume();
    // }

    // ++aaa;
    auto* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    if (renderer) {
        renderer->setBoxPosition(glm::vec3(tween1->value(), 0.0f, 0.0f));
    }
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
