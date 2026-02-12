#ifndef __OKAY_TWEEN_SEQUENCE_H__
#define __OKAY_TWEEN_SEQUENCE_H__

#include <okay/core/tween/i_okay_tween.hpp>
#include <memory>
#include <vector>

/**
  * @brief Sequence of composed tweens; control lifetime of multiple tweens together.
  */
namespace okay {
    class OkayTweenSequence : public IOkayTween, public std::enable_shared_from_this<OkayTweenSequence> {
       public:
        OkayTweenSequence() = default;

        /**
          * @brief Return a shared_ptr to this sequence.
          */
        static std::shared_ptr<OkayTweenSequence> create() {
            auto sequencePtr { std::make_shared<OkayTweenSequence>() };

            return sequencePtr;
        }

        /**
          * @brief Push back a tween ptr into _sequence.
          * 
          * @param tweenPtr shared ptr to a tween
          */
        void append(std::shared_ptr<IOkayTween> tweenPtr);

        /**
          * @brief Call start() on the first tween in the sequence.
          */
        void start();
        
        /**
          * @brief Check if the current tween has ended. If so, move on to the next tween.
          * Else, tick the current tween.
          */
        void tick();

        /**
          * @brief Call pause() on the current tween.
          */
        void pause();

        /**
          * @brief Call resume() on the current tween.
          */
        void resume();

        /**
          * @brief Resets the index to 0.
          */
        void reset();

        /**
          * @brief Kills all tweens in the sequence.
          */
        void kill();

        /**
          * @brief Check if the sequence is finished.
          */
        bool isFinished();

        /**
          * @brief Set whether the current tween is ticking.
          */
        void setIsTweening(bool isTweening);
       
       private:
        std::vector<std::shared_ptr<IOkayTween>> _sequence;
        bool _started { false };
        std::uint32_t _index { 0 };
    };
} // namespace okay

#endif
