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
    Tween() = default;

    // Tween(T object, long long target, std::float_t duration = 1, EasingStyle.LINEAR, EasignDirection.IN, std::int8_t loops = false, bool isOutAndBack = false, std::float_t buffer = 0)
    // {
    //     this.
    // }

    Tween(T object, T target)
        : _object { object },
          _target { target },
          _distance { target - object },
          _timeElapsed { 0 }
        //   _dT { static_cast<std::float_t>(okay::Engine.time->deltaTime()) / 1000 }
    {}
    
    void start() {
        this->_isTweenStarted = true;
    }

    void tick() {
        if (this->_isTweenStarted) {
            if (this->_timeElapsed < this->_duration) {
                this->_object = static_cast<T>(this->_timeElapsed) / this->_duration * this->_distance;
                this->_timeElapsed += okay::Engine.time->deltaTime();
            } else {
                this->_isTweenStarted = false;
                this->_object = this->_target;
                // dequeue
                // delte
            }
            okay::Engine.logger.debug("\nCurrent val: {}{}{}{}{}", this->_object, "\nTime elapsed: ", this->_timeElapsed, "\nDeltaTime: ", okay::Engine.time->deltaTime());
        }
    }

    T distance() {
        return this->_distance;
    }

   private:
    T _object;
    T _target;
    T _distance;
    bool _isTweenStarted { false };
    long long _duration { 2000 };
    long long _timeElapsed;
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

// class TweenEngine : public okay::OkaySystem<okay::OkaySystemScope::ENGINE> {

Tween<std::float_t> testTween;

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");
    testTween = Tween(0.0f, 4.0f);
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
