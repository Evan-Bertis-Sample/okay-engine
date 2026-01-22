#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <cstdint>
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include "okay/core/tween/okay_tween_engine.hpp"
#include "okay/core/tween/i_okay_tween.hpp"

namespace okay {

enum OkayTweenEasingStyle : std::uint8_t { LINEAR, SINE, CUBIC, QUAD, QUINT, CIRC, ELASTIC, QUART, EXPO, BACK, BOUNCE };
enum OkayTweenEasingDirection : std::uint8_t { IN, OUT, INOUT };

template <typename T>
concept Tweenable = requires(T t) {
    t + t;
    1.0f * t;
};

template <Tweenable T>
class OkayTween : public IOkayTween {
   public:
    OkayTween() = default;

    // Tween(T object, long long target, std::float_t duration = 1, EasingStyle.LINEAR, EasignDirection.IN, std::int8_t loops = false, bool isOutAndBack = false, std::float_t buffer = 0)
    // {
    //     this.
    // }

    OkayTween(T& start, T& end)
        : _start { start },
          END { end },
          DISTANCE { end - start },
          _current { start }
    {}
    
    void start() {
        auto ptr = std::make_unique<OkayTween<T>>(*this);
        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(std::move(ptr));
    }

    void tick() {
        _timeElapsed += okay::Engine.time->deltaTime();
        _current = _start + static_cast<float>(_timeElapsed) / _duration * DISTANCE;

        // specific logger.debug for a vec3 Tween
        // okay::Engine.logger.debug("\nCurrent val: ({}, {}, {})\nTime elapsed: {}\nDeltaMs: {}",
        // _current.x, _current.y, _current.z, _timeElapsed, okay::Engine.time->deltaTime());
    }

    std::int64_t timeRemaining() {
        return static_cast<std::int64_t>(_duration) - _timeElapsed;
    }

    void endTween() {
        _start = END;
    };

   private:
    T _start;
    const T END;
    const T DISTANCE;
    T _current;
    bool _isTweenStarted { false };
    std::uint32_t _duration { 2000 };
    std::uint32_t _timeElapsed {};
    // EasingStyle.LINEAR;
    // EasignDirection.IN;
    // std::int8_t loops;
    // bool isOutAndBack;
    // std::float_t buffer;
};

}; // namespace okay

#endif
