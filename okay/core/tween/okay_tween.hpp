#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include "okay/core/tween/okay_tween_engine.hpp"

namespace okay {

enum OkayTweenEasingStyle : std::uint8_t { LINEAR, SINE, CUBIC, QUAD, QUINT, CIRC, ELASTIC, QUART, EXPO, BACK, BOUNCE };
enum OkayTweenEasingDirection : std::uint8_t { IN, OUT, INOUT };

template <typename T>
concept Tweenable = requires(T t) {
    t + t;
    1.0f * t;
};

class IOkayTween {
   public:
    virtual void start();
    virtual void tick();
    virtual std::uint32_t timeRemaining();
    virtual void endTween();
};

template <Tweenable T>
class OkayTween : IOkayTween {
   public:
    OkayTween() = default;

    // Tween(T object, long long target, std::float_t duration = 1, EasingStyle.LINEAR, EasignDirection.IN, std::int8_t loops = false, bool isOutAndBack = false, std::float_t buffer = 0)
    // {
    //     this.
    // }

    OkayTween(T& start, T& end)
        : START { start },
          END { end },
          DISTANCE { end - start },
          _current { start },
          _timeElapsed { 0 }
    {}
    
    void start() {
        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(this);
    }

    void tick() {
        _timeElapsed += okay::Engine.time->deltaTime();
        _current = START + static_cast<float>(_timeElapsed) / _duration * DISTANCE;

        // specific logger.debug for a vec3 Tween
        okay::Engine.logger.debug("\nCurrent val: ({}, {}, {})\nTime elapsed: {}\nDeltaMs: {}",
        _current.x, _current.y, _current.z, _timeElapsed, okay::Engine.time->deltaTime());
    }

    std::uint32_t timeRemaining() {
        return _duration - _timeElapsed;
    }

    void endTween() {
        START = END;
    };

   private:
    const T START;
    const T END;
    const T DISTANCE;
    T _current;
    bool _isTweenStarted { false };
    std::uint32_t _duration { 2000 };
    std::uint32_t _timeElapsed;
    // EasingStyle.LINEAR;
    // EasignDirection.IN;
    // std::int8_t loops;
    // bool isOutAndBack;
    // std::float_t buffer;
};

}; // namespace okay

#endif
