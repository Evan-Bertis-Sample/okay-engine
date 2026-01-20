#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>

namespace okay {

template <typename T>
class OkayTween {
   public:
    OkayTween() = default;

    // Tween(T object, long long target, std::float_t duration = 1, EasingStyle.LINEAR, EasignDirection.IN, std::int8_t loops = false, bool isOutAndBack = false, std::float_t buffer = 0)
    // {
    //     this.
    // }

    OkayTween(T object, T target)
        : _object { object },
          _target { target },
          _distance { target - object },
          _timeElapsed { 0 }
    {}
    
    void start() {
        _isTweenStarted = true;
    }

    void tick() {
        if (_isTweenStarted) {
            if (_timeElapsed < _duration) {
                _object = static_cast<T>(_timeElapsed) / _duration * _distance;
                _timeElapsed += okay::Engine.time->deltaTime();
            } else {
                _isTweenStarted = false;
                _object = _target;
                // dequeue
                // delte
            }
            okay::Engine.logger.debug("\nCurrent val: {}{}{}{}{}", _object, "\nTime elapsed: ", _timeElapsed, "\nDeltaTime: ", okay::Engine.time->deltaTime());
        }
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

}; // namespace okay

#endif
