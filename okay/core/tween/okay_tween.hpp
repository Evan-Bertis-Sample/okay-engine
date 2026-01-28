#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <memory>
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
              std::int64_t numLoops = 0,
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
          _numLoops { numLoops },
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
        if (_isTweening) {
            if (_isReversing) {
                _timeElapsed = std::max(static_cast<int64_t>(0), static_cast<int64_t>(_timeElapsed) - okay::Engine.time->deltaTime());
            } else {
                _timeElapsed += okay::Engine.time->deltaTime();
            }

            std::float_t progress { _timeElapsed > _duration 
                                    ? 1 
                                    : static_cast<std::float_t>(_timeElapsed) / _duration };
            std::float_t step = { _easingFn(progress) };
            _current = START + step * DISTANCE;
        }

        okay::Engine.logger.debug("\nCurrent val: {}\nTime elapsed: {}\nDeltaMs: {}\nLoops: {}/{}",
                _current, _timeElapsed, okay::Engine.time->deltaTime(), _loopsCompleted, _numLoops);
        // specific logger.debug for a vec3 Tween
        // okay::Engine.logger.debug("\nCurrent val: ({}, {}, {})\nTime elapsed: {}\nDeltaMs: {}",
        // _current.x, _current.y, _current.z, _timeElapsed, okay::Engine.time->deltaTime());
    }

    void pause() {
        _isTweening = false;
        _onPause();
    }

    void resume() {
        _isTweening = true;
    }

    void kill() {
        _killTween = true;
        _onEnd();
    }

    bool isFinished() {
        if (_inOutBack && _isReversing && _timeElapsed == 0 
            || !_inOutBack && _timeElapsed >= static_cast<std::int64_t>(_duration))
        {
            if (_loopsCompleted == _numLoops) {
                return true;
            } else {
                _current = START;
                _timeElapsed = 0;
                ++_loopsCompleted;
            }
        }

        if (_inOutBack) {
            _isReversing = !_isReversing;
        }

        return false;
    }

    void endTween() {
        _current = END;
        _onEnd();
    }

   private:
    // core tween logic
    const T START;
    const T END;
    const T DISTANCE;
    T _current;
    bool _isTweening { true };
    std::uint32_t _duration;
    std::uint32_t _timeElapsed {};
    EasingFn _easingFn;

    // optional logical params
    bool _killTween { false };
    std::int64_t _numLoops;
    std::int64_t _loopsCompleted {};
    bool _inOutBack;
    bool _isReversing { false };
    std::uint32_t _buffer;
    std::function<void()> _onEnd;
    std::function<void()> _onPause;
};

}; // namespace okay

#endif
