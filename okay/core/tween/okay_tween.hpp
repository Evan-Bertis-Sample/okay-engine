#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>

namespace okay {

template <typename T>
concept Tweenable = requires(T t) {
    t + t;
    1.0f * t;
};

template <Tweenable T>
class OkayTween {
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
        _isTweenStarted = true;
    }

    void tick() { // linear tween
        if (_isTweenStarted) {
            if (_timeElapsed <= _duration) {
                _timeElapsed += okay::Engine.time->deltaTime();
                _current = START + static_cast<float>(_timeElapsed) / _duration * DISTANCE;
            } else {
                _isTweenStarted = false;
                START = END;
                // dequeue
                // delte
            }

            // specific logger.debug for a vec3 Tween
            okay::Engine.logger.debug("\nCurrent val: ({}, {}, {})\nTime elapsed: {}\nDeltaMs: {}",
                _current.x, _current.y, _current.z, _timeElapsed, okay::Engine.time->deltaTime());
        }
    }

   private:
    const T START;
    const T END;
    const T DISTANCE;
    T _current;
    bool _isTweenStarted { false };
    long long _duration { 2000 };
    long long _timeElapsed;
    // EasingStyle.LINEAR;
    // EasignDirection.IN;
    // std::int8_t loops;
    // bool isOutAndBack;
    // std::float_t buffer;
};

}; // namespace okay

#endif
