#ifndef __OKAY_TWEEN_SEQUENCE_H__
#define __OKAY_TWEEN_SEQUENCE_H__

#include <functional>
#include <vector>
#include "okay/core/tween/i_okay_tween.hpp"

namespace okay {
    class OkayTweenSequence {
       public:
        OkayTweenSequence() = default;

        void append(IOkayTween& tween);

        void start();

        void pause();

        void resume();

        void kill();
       
       private:
        std::vector<std::reference_wrapper<IOkayTween>> _sequence;
    };
} // namespace okay

#endif
