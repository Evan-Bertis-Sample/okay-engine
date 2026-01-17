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
