#ifndef __TWEEN_H__
#define __TWEEN_H__

#include <cstdint>
#include <cmath>
#include <memory>
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include "okay/core/tween/okay_tween_engine.hpp"
#include "okay/core/tween/i_okay_tween.hpp"
#include "okay/core/tween/okay_tween_easing.hpp"
#include "okay_tween_engine.hpp"

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
              std::function<void()> onPause = [](){},
              std::function<void()> onResume = [](){})
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

    static std::shared_ptr<OkayTween<T>> create(T& start,
                                                T& end,
                                                std::uint32_t duration = 1000,
                                                EasingFn easingFn = okay::easing::linear,
                                                std::int64_t numLoops = 0,
                                                bool inOutBack = false,
                                                std::uint32_t buffer = 0,
                                                std::function<void()> onEnd = [](){},
                                                std::function<void()> onPause = [](){},
                                                std::function<void()> onResume = [](){})
    {
        auto tweenPtr { std::make_shared<OkayTween<T>>(start, end, duration, easingFn, numLoops, inOutBack, buffer, onEnd, onPause, onResume) };

        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(tweenPtr);

        return tweenPtr;
    }
    
    void start() {
        _isTweening = true;
    }

    void tick() {
        if (_isTweening) {
            _timeElapsed += okay::Engine.time->deltaTime();
            
            if (_timeElapsed > _duration) {
                _timeElapsed = _duration;
            }
            
            std::float_t progress = static_cast<std::float_t>(_timeElapsed) / _duration;
            
            if (_isReversing) {
                progress = 1.0f - progress;
            }
            
            std::float_t step = _easingFn(progress);
            _current = START + step * DISTANCE;

            // visualize tween in console
            std::cout << '\n';
            for (T i { START }; i < _current; i += DISTANCE / 150)
            {
                std::cout << ':';
            }
            std::cout << "O";
            for (T i { _current }; i <= END; i += DISTANCE / 150)
            {
                std::cout << '-';
            }
        }
    }

    void pause() {
        _isTweening = false;
        _onPause();
    }

    void resume() {
        _isTweening = true;
        _onResume();
    }

    void kill() {
        _killTween = true;
        _onEnd();
    }

    bool isFinished() {
        if (_killTween) {
            return true;
        }

        if (_timeElapsed < _duration) {
            return false;
        }

        if (_inOutBack && !_isReversing) {
            _isReversing = true;
            _timeElapsed = 0;
            return false;
        }

        if (_numLoops == _loopsCompleted) {
            return true;
        } else {
            _timeElapsed = 0;
            _current = START;
            ++_loopsCompleted;
        }

        _isReversing = false;

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
    bool _isTweening { false };
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
    std::function<void()> _onResume;
};

}; // namespace okay

#endif
