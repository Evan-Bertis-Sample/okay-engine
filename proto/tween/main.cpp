#include <functional>
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
#include "okay/core/tween/okay_tween_collection.hpp"

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static float ref0;
static float ref1;
static float ref2;
static float ref3;
static float ref4;

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

okay::TweenConfig<float> tweenConfig0 {
    .start = -0.5,
    .end = 0.5,
    .ref = ref0,
};
okay::TweenConfig<float> tweenConfig1 {
    .start = -0.5f,
    .end = 0.5f,
    .ref = ref1,
    .durationMs = 2000,
    .inOutBack = true,
};
okay::TweenConfig<float> tweenConfig2 {
    .start = -0.5f,
    .end = 0.5f,
    .ref = ref2,
    .durationMs = 2000,
    .inOutBack = true,
};
okay::TweenConfig<float> tweenConfig3 {
    .start = -0.5f,
    .end = 0.5f,
    .ref = ref3,
    .easingFn = okay::easing::cubicIn,
    .durationMs = 2000,
    .inOutBack = true,
};
okay::TweenConfig<float> tweenConfig4 {
    .start = -0.5f,
    .end = 0.5f,
    .ref = ref4,
    .easingFn = okay::easing::bounceInOut,
    .durationMs = 2000,
    .inOutBack = true,
};

std::shared_ptr<okay::OkayTween<float>> tween0;
std::shared_ptr<okay::OkayTween<float>> tween1;
std::shared_ptr<okay::OkayTween<float>> tween2;
std::shared_ptr<okay::OkayTween<float>> tween3;
std::shared_ptr<okay::OkayTween<float>> tween4;
std::shared_ptr<okay::OkayTweenSequence> seq { okay::OkayTweenSequence::create() };
std::shared_ptr<okay::OkayTweenCollection> col {okay::OkayTweenCollection::create() };

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");
    
    tween0 = okay::OkayTween<float>::create(tweenConfig0);
    // tween0->start();

    tween1 = okay::OkayTween<float>::create(tweenConfig1);
    tween2 = okay::OkayTween<float>::create(tweenConfig2);
    // col->append(tween1);
    // col->append(tween2);
    // col->start();

    tween3 = okay::OkayTween<float>::create(tweenConfig3);
    tween4 = okay::OkayTween<float>::create(tweenConfig4);
    // seq->append(tween3);
    // seq->append(tween4);
    // seq.start();
}

static void __gameUpdate() {
    // Game update logic
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
