#ifndef __OKAY_TWEEN_H__
#define __OKAY_TWEEN_H__

#include <functional>
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/tween/okay_tween_engine.hpp>
#include <okay/core/tween/i_okay_tween.hpp>
#include <okay/core/tween/okay_tween_easing.hpp>

#include <cstdint>
#include <cmath>
#include <memory>
#include <optional>

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
struct TweenConfig {
    T start;
    T end;
    std::optional<std::reference_wrapper<T>> ref = std::nullopt;
    std::uint32_t durationMs = 1000;
    EasingFn easingFn = okay::easing::linear;
    std::int64_t numLoops = 0;
    bool inOutBack = false;
    std::int32_t prefixMs = 0;
    std::function<void()> onTick = [](){};
    std::function<void()> onEnd = [](){};
    std::function<void()> onPause = [](){};
    std::function<void()> onResume = [](){};
    std::function<void()> onLoop = [](){};
};

template <Tweenable T>
class OkayTween : public IOkayTween, public std::enable_shared_from_this<OkayTween<T>> {
   public:
    OkayTween() = default;

    explicit OkayTween(const TweenConfig<T>& cfg)
        : START { cfg.start },
          END { cfg.end },
          DISPLACEMENT { cfg.end - cfg.start },
          _reference { cfg.ref },
          _current { cfg.start },
          _durationMs { cfg.durationMs },
          _easingFn { cfg.easingFn },
          _numLoops { cfg.numLoops },
          _inOutBack { cfg.inOutBack },
          _prefixMs { cfg.prefixMs },
          _remainingPrefixMs { cfg.prefixMs },
          _onTick { cfg.onTick },
          _onReset { cfg.onEnd },
          _onPause { cfg.onPause },
          _onResume { cfg.onResume },
          _onLoop { cfg.onLoop }
    {}

    OkayTween(T start,
              T end,
              std::optional<T&> ref = std::nullopt,
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
                                                std::optional<std::reference_wrapper<T>> ref = std::nullopt,
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
        auto tweenPtr { std::make_shared<OkayTween<T>>(start, end, ref, durationMs, easingFn, numLoops, inOutBack, prefixMs, onTick, onEnd, onPause, onResume, onLoop) };

        return tweenPtr;
    }

    static std::shared_ptr<OkayTween<T>> create(const TweenConfig<T>& cfg)
    {
        auto tweenPtr { std::make_shared<OkayTween<T>>(cfg.start, cfg.end, cfg.ref, cfg.durationMs, cfg.easingFn, cfg.numLoops, cfg.inOutBack, cfg.prefixMs, cfg.onTick, cfg.onEnd, cfg.onPause, cfg.onResume, cfg.onLoop) };

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

    // std::optionalal logical params
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

    std::optional<std::reference_wrapper<T>> _reference;
};

} // namespace okay

#endif
