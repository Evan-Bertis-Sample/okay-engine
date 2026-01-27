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
#include "okay/core/tween/okay_tween_easing.hpp"

namespace okay {

template <typename T>
concept Tweenable = requires(T t) {
    t + t;
    1.0f * t;
};

template <Tweenable T>
class OkayTween : public IOkayTween {
   public:
    OkayTween() = default;

    OkayTween(T& start,
              T& end,
              std::uint32_t duration = 1000,
              EasingFn easingFn = okay::easing::linear,
              bool loops = false,
              bool inOutBack = false,
              std::uint32_t buffer = 0,
              std::function<void()> onEnd = [](){},
              std::function<void()> onPause = [](){})
        : START { start },
          END { end },
          DISTANCE { end - start },
          _current { start },
          _duration { duration },
          _easingFn { easingFn },
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
        std::float_t step {};
        _timeElapsed += okay::Engine.time->deltaTime();
        std::float_t progress { _timeElapsed > _duration ? 1 : static_cast<std::float_t>(_timeElapsed) / _duration };
        step = _easingFn(progress);
        _current = START + step * DISTANCE;

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
        _current = END;
    };

   private:
    // core tween logic
    const T START;
    const T END;
    const T DISTANCE;
    T _current;
    bool _isTweening { false };
    std::uint32_t _duration;
    std::uint32_t _timeElapsed {};

    // optional logical params
    EasingFn _easingFn;
    bool _loops;
    bool _inOutBack;
    std::uint32_t _buffer;
    std::function<void()> _onEnd;
    std::function<void()> _onPause;
};

}; // namespace okay

#endif
