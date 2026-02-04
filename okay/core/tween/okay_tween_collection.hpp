#ifndef __OKAY_TWEEN_COLLECTION_H__
#define __OKAY_TWEEN_COLLECTION_H__

#include <memory>
#include <vector>
#include "okay/core/tween/i_okay_tween.hpp"

/**
  * @brief Collection of tweens; control lifetime of multiple tweens together.
  */
namespace okay {
    class OkayTweenCollection {
       public:
        OkayTweenCollection() = default;

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
          * @brief Call pause() on all tweens in _collection; filter out ended tweens.
          */
        void pause();

        /**
          * @brief Call resume() on all tweens in _collection; filter out ended tweens.
          */
        void resume();

        /**
          * @brief Call kill() on all tweens in _collection; clear _collection.
          */
        void kill();
       
       private:
        std::vector<std::shared_ptr<IOkayTween>> _collection;

        /**
          * @brief Erase tween ptr from _activeTweens at given index.
          * 
          * @param index index of tween ptr
          */
        void removeTween(std::uint64_t index);
    };
} // namespace okay

#endif
