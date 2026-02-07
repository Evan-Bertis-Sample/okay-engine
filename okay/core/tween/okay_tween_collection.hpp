#ifndef __OKAY_TWEEN_COLLECTION_H__
#define __OKAY_TWEEN_COLLECTION_H__

#include <memory>
#include <vector>
#include "okay/core/tween/i_okay_tween.hpp"

/**
  * @brief Collection of tweens; control lifetime of multiple tweens together.
  */
namespace okay {
    class OkayTweenCollection : public IOkayTween, public std::enable_shared_from_this<OkayTweenCollection> {
       public:
        OkayTweenCollection() = default;

        /**
          * @brief Return a shared_ptr to this collection.
          */
        static std::shared_ptr<OkayTweenCollection> create() {
            auto collectionPtr { std::make_shared<OkayTweenCollection>() };

            return collectionPtr;
        }

        /**
          * @brief Push back a tween ptr into _collection.
          * 
          * @param tweenPtr shared ptr to a tween
          */
        void append(std::shared_ptr<IOkayTween> tweenPtr);

        /**
          * @brief Call start() on all tweens in _collection.
          */
        void start();

        /**
          * @brief Tick all tweens in collection.
          */
        void tick();

        /**
          * @brief Call pause() on all tweens in _collection
          */
        void pause();

        /**
          * @brief Call resume() on all tweens in _collection
          */
        void resume();

        /**
          * @brief Calls reset() on all tweens.
          */
        void reset();

        /**
          * @brief Call kill() on all tweens in _collection; clear _collection.
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
        std::vector<std::shared_ptr<IOkayTween>> _collection;
    };
} // namespace okay

#endif
