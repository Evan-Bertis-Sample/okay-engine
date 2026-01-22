#ifndef __I_OKAY_TWEEN_ENGINE_H__
#define __I_OKAY_TWEEN_ENGINE_H__

#include <cstdint>

class IOkayTween {
   public:
    virtual void start();
    virtual void tick();
    virtual std::uint32_t timeRemaining();
    virtual void endTween();
};

#endif
