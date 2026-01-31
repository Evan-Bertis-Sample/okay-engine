#ifndef __OKAY_TWEEN_SEQUENCE_H__
#define __OKAY_TWEEN_SEQUENCE_H__

#include <memory>
#include <vector>
#include "okay/core/tween/i_okay_tween.hpp"

namespace okay {
    class OkayTweenSequence {
       public:
        OkayTweenSequence() = default;

        void append(std::shared_ptr<IOkayTween> tweenPtr);

        void start();

        void pause();

        void resume();

        void kill();
       
       private:
        std::vector<std::shared_ptr<IOkayTween>> _sequence;

        void removeTween(std::uint64_t index);
    };
} // namespace okay

#endif
