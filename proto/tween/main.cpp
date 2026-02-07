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

static float ref;

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



okay::TweenConfig<float> tween5Config {
    .start = -0.5,
    .end = 0.5,
    .ref = ref
};

std::shared_ptr<okay::OkayTween<float>> tween1;
std::shared_ptr<okay::OkayTween<float>> tween2;
std::shared_ptr<okay::OkayTween<float>> tween3;
std::shared_ptr<okay::OkayTween<float>> tween4;
std::shared_ptr<okay::OkayTween<float>> tween5;
std::shared_ptr<okay::OkayTweenSequence> seq { okay::OkayTweenSequence::create() };
std::shared_ptr<okay::OkayTweenCollection> col {okay::OkayTweenCollection::create() };

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");

    // DEMO
    
    // tween1 = okay::OkayTween<float>::create(0.5f, -0.5f, std::nullopt, 2000, okay::easing::linear, 
    //     0, true, 0);
    // tween2 = okay::OkayTween<float>::create(0.5f, -0.5f, std::nullopt, 2000, okay::easing::linear, 
    //     0, true, 0);
    // tween3 = okay::OkayTween<float>::create(-0.5f, 0.5f, std::nullopt, 2000, okay::easing::backInOut, 
    //     1, true, 0, [&]() {
    //         auto* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    //         renderer->setBoxPosition(glm::vec3(tween3->value(), tween3->value(), 0.0f));
    //     });
    // tween4 = okay::OkayTween<float>::create(-0.5f, 0.5f, std::nullopt, 2000, okay::easing::quadOut, 
    //     0, true, 0, [&]() {
    //         auto* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    //         renderer->setBoxPosition(glm::vec3(0.0f, tween4->value(), 0.0f));
    //     });

    tween5 = okay::OkayTween<float>::create(tween5Config);
    col->append(tween1);
    col->append(tween2);

    seq->append(tween3);
    seq->append(tween4);


    // int n1 {0};
    // int n2 {2};
    // col.append(okay::OkayTween<int>::create(n1, 200));
    // col.append(okay::OkayTween<int>::create(n1, 500, 2000));

    //col->start();
    
    tween5->start();
    // col.start();

    // okay::Engine.logger.debug("size: {}", sizeof(tween4));
}

static void __gameUpdate() {
    // auto* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    // renderer->setBoxPosition(glm::vec3(tween5->value(), 0.0f, 0.0f));

    // static int i {};
    
    // okay::Engine.logger.debug("tick: {}", i);
    // if (i == 300) {
    //     seq->kill();
    // }

    // if (i == 500) {
    //     seq->resume();
    // }

    // ++i;
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
