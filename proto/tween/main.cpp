#include <cmath>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>

#include <utility>
#include "okay/core/system/okay_system.hpp"



template <typename T>
class Tween {
   public:
    // Tween(T object, long long target, std::float_t duration = 1, EasingStyle.LINEAR, EasignDirection.IN, std::int8_t loops = false, bool isOutAndBack = false, std::float_t buffer = 0)
    // {
    //     this.
    // }

    Tween(T object, T target)
        : _object { object },
          _target { target },
          _distance { target - object }
        //   _dT { static_cast<std::float_t>(okay::Engine.time->deltaTime()) / 1000 }
    {
        this->_dT = static_cast<std::float_t>(okay::Engine.time->deltaTime()) / 1000;
    }
    
    void start() {
        for (std::float_t timeElapsed {}; timeElapsed <= this->_duration; timeElapsed += this->_dT ) {
            this->_dT = static_cast<std::float_t>(okay::Engine.time->deltaTime()) / 1000;
            this->_object = static_cast<T>(timeElapsed / this->_duration * this->_distance);
            okay::Engine.logger.debug("\nCurrent val: {}{}{}{}{}", this->_object, "\nTime elapsed: ", timeElapsed, "\nDeltaTime: ", this->_dT);
            // std::this_thread::sleep_for(std::chrono::seconds(static_cast<long long>(this->_dT)));
        }

        this->_object = this->_target;
    }

   private:
    T _object;
    T _target;
    std::float_t _duration { 2 };
    std::float_t _dT;
    T _distance;
    // EasingStyle.LINEAR;
    // EasignDirection.IN;
    // std::int8_t loops;
    // bool isOutAndBack;
    // std::float_t buffer;
};

// Tween<float> tween;

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

// class TweenEngine : public okay::OkaySystem<okay::OkaySystemScope::ENGINE> {}

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");

    
}

static void __gameUpdate() {
    // std::cout << "Game updated." << std::endl;
    // Game update logic

    static int count {};

    
    if (count == 50) {
        Tween testTween(0.0f, 4.0f);
        testTween.start();
    }
    okay::Engine.time->deltaTime();

    ++count;
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
