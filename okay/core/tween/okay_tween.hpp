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

    OkayTween(T start, T end)
        : _start { start },
          _current { start },
          _end { end },
          _distance { end - start },
          _timeElapsed { 0 }
    {}
    
    void start() {
        _isTweenStarted = true;
    }

    void tick() { // linear tween
        if (_isTweenStarted) {
            if (_timeElapsed < _duration) {
                _timeElapsed += okay::Engine.time->deltaMs();
                _current = _start + static_cast<float>(_timeElapsed) / _duration * _distance;
            } else {
                _isTweenStarted = false;
                _start = _end;
                // dequeue
                // delte
            }

            // specific logger.debug for a vec3 Tween
            okay::Engine.logger.debug("\nCurrent val: ({}, {}, {})\nTime elapsed: {}\nDeltaMs: {}",
                _current.x, _current.y, _current.z, _timeElapsed, okay::Engine.time->deltaMs());
        }
    }

   private:
    T _start;
    T _current;
    T _end;
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

}; // namespace okay

#endif
