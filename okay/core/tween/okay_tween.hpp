#ifndef __OKAY_TWEEN_H__
#define __OKAY_TWEEN_H__

#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/tween/okay_tween_engine.hpp>
#include <okay/core/tween/i_okay_tween.hpp>
#include <okay/core/tween/okay_tween_easing.hpp>

#include <cstdint>
#include <cmath>
#include <memory>

namespace okay {

/**
  * Ensure an OkayTween can only be created if 
  * addition and scalar multiplication is possible on its type.
  */
template <typename T>
concept Tweenable = requires(T t) {
    t + t;
    1.0f * t;
};

/**
  * @brief A tween
  *
  * @param start Starting value to tween to
  * @param end End value to tween to
  * @param easingFn Easing function to interpolate with
  * @param numLoops Number of times the tween should loop, excluding the initial run (-1 if infinite, default 0)
  * @param inOutBack Whether the tween should in-out-back (yoyo)
  * @param buffer Time to wait before tween beins ticking
  * @param onTick Callback for every time tween is updated
  * @param onEnd Callback for when the tween ends
  * @param onPause Callback for when the tween pauses
  * @param onResume Callback for when the tween resumes
  * @param onLoop Callback for when the tween completes a loop
  */
template <Tweenable T>
class OkayTween : public IOkayTween, public std::enable_shared_from_this<OkayTween<T>> {
   public:
    OkayTween() = default;

    OkayTween(T start,
              T end,
              std::uint32_t durationMs = 1000,
              EasingFn easingFn = okay::easing::linear,
              std::int64_t numLoops = 0,
              bool inOutBack = false,
              std::int32_t prefixMs = 0,
              std::function<void()> onTick = [](){},
              std::function<void()> onEnd = [](){},
              std::function<void()> onPause = [](){},
              std::function<void()> onResume = [](){},
              std::function<void()> onLoop = [](){})
        : START { start },
          END { end },
          DISPLACEMENT { end - start },
          _current { start },
          _durationMs { durationMs },
          _easingFn { easingFn },
          _numLoops { numLoops },
          _inOutBack { inOutBack },
          _prefixMs { prefixMs },
          _remainingPrefixMs { prefixMs },
          _onTick { onTick },
          _onReset { onEnd },
          _onPause { onPause },
          _onResume { onResume },
          _onLoop { onLoop }
    {}

    static std::shared_ptr<OkayTween<T>> create(T start,
                                                T end,
                                                std::uint32_t durationMs = 1000,
                                                EasingFn easingFn = okay::easing::linear,
                                                std::int64_t numLoops = 0,
                                                bool inOutBack = false,
                                                std::int32_t prefixMs = 0,
                                                std::function<void()> onTick = [](){},
                                                std::function<void()> onEnd = [](){},
                                                std::function<void()> onPause = [](){},
                                                std::function<void()> onResume = [](){},
                                                std::function<void()> onLoop = [](){})
    {
        auto tweenPtr { std::make_shared<OkayTween<T>>(start, end, durationMs, easingFn, numLoops, inOutBack, prefixMs, onTick, onEnd, onPause, onResume, onLoop) };

        return tweenPtr;
    }
    
    void start() {
        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(this->shared_from_this());
        setIsTweening(true);
    }

    void tick() {
        if (!_isTweening) return;

        if (_remainingPrefixMs > 0) {
            _remainingPrefixMs -= okay::Engine.time->deltaTime();
        } else {
            _timeElapsed += okay::Engine.time->deltaTime();
        
            if (_timeElapsed > _durationMs) {
                _timeElapsed = _durationMs;
            }
            
            std::float_t progress = static_cast<std::float_t>(_timeElapsed) / _durationMs;
            
            if (_isReversing) {
                progress = 1.0f - progress;
            }
            
            std::float_t step = _easingFn(progress);
            _current = START + step * DISPLACEMENT;
            _onTick();
        }
    }

    void pause() {
        setIsTweening(false);
        _onPause();
    }

    void resume() {
        setIsTweening(true);
        _onResume();
    }
    
    void reset() {
        _current = START;
        _loopsCompleted = 0;
        _isReversing = false;
        _remainingPrefixMs = _prefixMs;
        _timeElapsed = 0;
        setIsTweening(false);
        _onReset();
    }

    void kill() {
        _killTween = true;
    }

    bool isFinished() {
        if (_killTween) {
            return true;
        }

        if (_timeElapsed < _durationMs) {
            return false;
        }

        if (_inOutBack && !_isReversing) {
            _isReversing = true;
            _timeElapsed = 0;
            return false;
        }

        if (_numLoops == _loopsCompleted && _numLoops != -1) {
            return true;
        } else {
            _timeElapsed = 0;
            _current = START;
            ++_loopsCompleted;
            _onLoop();
        }

        _isReversing = false;

        return false;
    }


    void setIsTweening(bool isTweening) {
        _isTweening = isTweening;
    }

    const T& value() const { 
        return _current; 
    }

   private:
    // core tween logic
    const T START;
    const T END;
    const T DISPLACEMENT;
    T _current;
    bool _isTweening { false };
    std::uint32_t _durationMs;
    std::uint32_t _timeElapsed {};
    EasingFn _easingFn;

    // optional logical params
    bool _killTween { false };
    std::int64_t _numLoops;
    std::int64_t _loopsCompleted {};
    bool _inOutBack;
    bool _isReversing { false };
    std::int32_t _prefixMs;
    std::int32_t _remainingPrefixMs;
    std::function<void()> _onTick;
    std::function<void()> _onReset;
    std::function<void()> _onPause;
    std::function<void()> _onResume;
    std::function<void()> _onLoop;
};

} // namespace okay

#endif
