#ifndef __I_OKAY_TWEEN_ENGINE_H__
#define __I_OKAY_TWEEN_ENGINE_H__

#include <cstdint>

class IOkayTween {
   public:
    virtual void start() = 0;
    virtual void tick() = 0;
    virtual bool isFinished() = 0;
    virtual void endTween() = 0;
    virtual ~IOkayTween() = default;
};

#endif
