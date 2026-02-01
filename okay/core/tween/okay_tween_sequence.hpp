#ifndef __OKAY_TWEEN_SEQUENCE_H__
#define __OKAY_TWEEN_SEQUENCE_H__

#include <okay/core/tween/i_okay_tween.hpp>
#include <memory>
#include <vector>

/**
  * @brief Sequence of composed tweens; control lifetime of multiple tweens together.
  */
namespace okay {
    class OkayTweenSequence {
       public:
        OkayTweenSequence() = default;

        /**
          * @brief Push back a tween ptr into _sequence.
          * 
          * @param tweenPtr shared ptr to a tween
          */
        void append(std::shared_ptr<IOkayTween> tweenPtr);

        /**
          * @brief Call start() on all tweens in _sequence.
          */
        void start();

        /**
          * @brief Call pause() on all tweens in _sequence; filter out ended tweens.
          */
        void pause();

        /**
          * @brief Call resume() on all tweens in _sequence; filter out ended tweens.
          */
        void resume();

        /**
          * @brief Call kill() on all tweens in _sequence; clear _sequence.
          */
        void kill();
       
       private:
        std::vector<std::shared_ptr<IOkayTween>> _sequence;

        /**
          * @brief Erase tween ptr from _activeTweens at given index.
          * 
          * @param index index of tween ptr
          */
        void removeTween(std::uint64_t index);
    };
} // namespace okay

#endif
