#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <algorithm>
#include <compare>
#include <cstdint>
#include <cmath>
#include <iterator>
#include <memory>
#include <numbers>
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include "okay/core/tween/okay_tween_engine.hpp"
#include "okay/core/tween/i_okay_tween.hpp"

namespace okay {

enum OkayTweenEasingStyle : std::uint8_t { LINEAR, SINE, QUAD, CUBIC, QUART, QUINT, EXPO, CIRC, BACK, ELASTIC, BOUNCE };
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

    OkayTween(T& start,
              T& end, 
              std::uint32_t duration = 1000,
              OkayTweenEasingStyle easingStyle = LINEAR,
              OkayTweenEasingDirection easingDirection = IN,
              bool loops = false,
              bool inOutBack = false,
              std::uint32_t buffer = 0,
              std::function<void()> onEnd = [](){},
              std::function<void()> onPause = [](){})
        : _start { start },
          END { end },
          DISTANCE { end - start },
          _current { start },
          _duration { duration },
          _easingStyle { easingStyle },
          _easingDirection { easingDirection },
          _loops { loops },
          _inOutBack { inOutBack },
          _buffer { buffer },
          _onEnd { onEnd },
          _onPause { onPause }
    {}
    
    void start() {
        auto ptr = std::make_unique<OkayTween<T>>(*this);
        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(std::move(ptr));
    }

    void tick() {
        std::double_t step {};

        _timeElapsed += okay::Engine.time->deltaTime();
        _prog = static_cast<std::float_t>(_timeElapsed) / _duration;
        
        switch (_easingStyle) {
           case LINEAR:
            step = _prog;
            break;
           case SINE:
            switch (_easingDirection) {
               case IN:
                step = this->sineIn();
                break;
               case OUT:
                step = this->sineOut();
                break;
               case INOUT:
                step = this->sineInOut();
                break;
            }
           case QUAD:
            switch (_easingDirection) {
               case IN:
                step = this->quadIn();
                break;
               case OUT:
                step = this->quadOut();
                break;
               case INOUT:
                step = this->quadInOut();
                break;
            }
           case CUBIC:
            switch (_easingDirection) {
               case IN:
                step = this->cubicIn();
                break;
               case OUT:
                step = this->cubicOut();
                break;
               case INOUT:
                step = this->cubicInOut();
                break;
            }
           case QUART:
            switch (_easingDirection) {
               case IN:
                step = this->quartIn();
                break;
               case OUT:
                step = this->quartOut();
                break;
               case INOUT:
                step = this->quartInOut();
                break;
            }
           case QUINT:
            switch (_easingDirection) {
               case IN:
                step = this->quintIn();
                break;
               case OUT:
                step = this->quintOut();
                break;
               case INOUT:
                step = this->quintInOut();
                break;
            }
           case CIRC:
            switch (_easingDirection) {
               case IN:
                step = this->circIn();
                break;
               case OUT:
                step = this->circOut();
                break;
               case INOUT:
                step = this->circInOut();
                break;
            }
           case ELASTIC:
            switch (_easingDirection) {
               case IN:
                step = this->elasticIn();
                break;
               case OUT:
                step = this->elasticOut();
                break;
               case INOUT:
                step = this->elasticInOut();
                break;
            }
           case EXPO:
            switch (_easingDirection) {
               case IN:
                step = this->expoIn();
                break;
               case OUT:
                step = this->expoOut();
                break;
               case INOUT:
                step = this->expoInOut();
                break;
            }
           case BACK:
            switch (_easingDirection) {
               case IN:
                step = this->backIn();
                break;
               case OUT:
                step = this->backOut();
                break;
               case INOUT:
                step = this->backInOut();
                break;
            }
           case BOUNCE:
            switch (_easingDirection) {
               case IN:
                step = this->bounceIn();
                break;
               case OUT:
                step = this->bounceOut();
                break;
               case INOUT:
                step = this->bounceInOut();
                break;
            }
        }

        
        // Engine.logger.debug("{}", step);

        _current = _start + step * DISTANCE;

        okay::Engine.logger.debug("\nCurrent val: {}\nTime elapsed: {}\nDeltaMs: {}",
                _current, _timeElapsed, okay::Engine.time->deltaTime());

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
    // core tween logic
    T _start;
    const T END;
    const T DISTANCE;
    T _current;
    bool _isTweening { false };
    std::uint32_t _duration;
    std::uint32_t _timeElapsed {};
    std::float_t _prog {};

    // optional logical params
    OkayTweenEasingStyle _easingStyle;
    OkayTweenEasingDirection _easingDirection;
    bool _loops;
    bool _inOutBack;
    std::uint32_t _buffer;
    std::function<void()> _onEnd;
    std::function<void()> _onPause;

    // easing functions
    std::float_t sineIn () {
        const std::float_t PI { std::numbers::pi };

        return 1 - std::cos(_prog * PI) / 2;
    }
    std::float_t sineOut() {
        const std::float_t PI { std::numbers::pi };

        return std::sin((_prog * PI)) / 2;
    }
    std::float_t sineInOut() {
        const std::float_t PI { std::numbers::pi };

        return -(std::cos(PI * _prog) - 1) / 2;
    }

    std::float_t quadIn() {
        return _prog * _prog;
    }
    std::float_t quadOut() {
        return 1 - (1 - _prog) * (1 - _prog);
    }
    std::float_t quadInOut() {
        return _prog < 0.5 ? 2 * _prog  * _prog : 1 - std::pow(-2 * _prog + 2, 2) / 2;
    }

    std::float_t cubicIn() {
        return _prog * _prog * _prog;
    }
    std::float_t cubicOut() {
        return 1 - std::pow(1 - _prog, 3);
    }
    std::float_t cubicInOut() {
        return _prog < 0.5
                ? 4 * _prog * _prog * _prog
                : 1 - std::pow(-2 * _prog + 2, 3) / 2;
    }

    std::float_t quartIn() {
        return _prog * _prog * _prog * _prog;
    }
    std::float_t quartOut() {
        return 1 - std::pow(1 - _prog, 4);
    }
    std::float_t quartInOut() {
        return _prog < 0.5
                ? 8 * _prog * _prog * _prog * _prog
                : 1 - std::pow(-2 * _prog + 2, 4) / 2;
    }

    std::float_t quintIn() {
        return _prog * _prog * _prog * _prog * _prog;
    }
    std::float_t quintOut() {
        return 1 - std::pow(1 - _prog, 5);
    }
    std::float_t quintInOut() {
        return _prog < 0.5
                ? 16 * _prog * _prog * _prog * _prog * _prog
                : (1 - std::pow(-2 * _prog + 2, 5)) / 2;
    }

    std::float_t expoIn() {
        return _prog == 0 ? 0 : std::pow(2, 10 * _prog - 10);
    }
    std::float_t expoOut() {
        return _prog == 1 ? 1 : 1 - std::pow(2, -10 * _prog);
    }
    std::float_t expoInOut() {
        return _prog == 0
                ? 0
                : _prog == 1
                ? 1
                : _prog < 0.5 ? std::pow(2, 20 * _prog - 10) / 2
                : (2 - std::pow(2, -20 * _prog + 10)) / 2;
    }

    std::float_t circIn() {
        return 1 - std::sqrt(1 - _prog * _prog);
    }
    std::float_t circOut() {
        return std::sqrt(1 - std::pow(_prog - 1, 2));
    }
    std::float_t circInOut() {
        return _prog < 0.5
          ? (1 - std::sqrt(1 - std::pow(2 * _prog, 2))) / 2
          : (std::sqrt(1 - std::pow(-2 * _prog + 2, 2)) + 1) / 2;
    }
    
    std::float_t elasticIn() {
        const std::float_t PI { std::numbers::pi };
        const std::float_t C4 { (2 * PI) / 3 };

        return _prog == 0
          ? 0
          : _prog == 1
          ? 1
          : -std::pow(2, 10 * _prog - 10) * std::sin((_prog * 10 - 10.75) * C4);
    }
    std::float_t elasticOut() {
        const std::float_t PI { std::numbers::pi };
        const std::float_t C4 { (2 * PI) / 3 };
        
        return _prog == 0
          ? 0
          : _prog == 1
          ? 1
          : std::pow(2, -10 * _prog) * std::sin((_prog * 10 - 0.75) * C4 + 1);
    }
    std::float_t elasticInOut() {
        const std::float_t PI { std::numbers::pi };
        const std::float_t C5 { (2 * PI) / 4.5f };

        return _prog == 0
          ? 0
          : _prog == 1
          ? 1
          : _prog < 0.5
          ? -(std::pow(2, 20 * _prog - 10) * std::sin((20 * _prog - 11.125) * C5)) / 2
          : (std::pow(2, -20 * _prog + 10) * std::sin((20 * _prog - 11.125) * C5)) / 2 + 1;;
    }
    
    std::float_t backIn() {
        const std::float_t C1 { 1.70158 };
        const std::float_t C3 { C1 + 1 };

        return C3 * _prog * _prog * _prog - C1 * _prog * _prog;
    }
    std::float_t backOut() {
        const std::float_t C1 { 1.70158 };
        const std::float_t C3 { C1 + 1 };
        
        return 1 + C3 * std::pow(_prog - 1, 3) + C1 * std::pow(_prog - 1, 2);
    }
    std::float_t backInOut() {
        const std::float_t C1 { 1.70158 };
        const std::float_t C2 { C1 * 1.525f };
        
        return _prog < 0.5
            ? (std::pow(2 * _prog, 2) * ((C2 + 1) * 2 * _prog - C2)) / 2
            : (std::pow(2 * _prog - 2, 2) * ((C2 + 1) * (_prog * 2 - 2) + C2) + 2) / 2;
    }

    std::float_t bounceIn() {
        _prog = 1 - _prog, 0;
        return 1 - this->bounceOut();
    }
    std::float_t bounceOut() {
        const std::float_t N1 = 7.5625;
        const std::float_t D1 = 2.75;

        if (_prog < 1 / D1) {
            return N1 * _prog * _prog;
        } else if (_prog < 2 / D1) {
            return N1 * (_prog -= 1.5 / D1) * _prog + 0.75;
        } else if (_prog < 2.5 / D1) {
            return N1 * (_prog -= 2.25 / D1) * _prog + 0.9375;
        } else {
            return N1 * (_prog -= 2.625 / D1) * _prog + 0.984375;
        }
    }
    std::float_t bounceInOut() {
        if (_prog < 0.5) {
            _prog = 1 - 2 * _prog;
            return 1 - this->bounceOut() / 2;
        } else {
            _prog = 2 * _prog - 1;
            return 1 + this->bounceOut() / 2;
        }
    }
};

}; // namespace okay

#endif
