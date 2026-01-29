#ifndef __I_OKAY_TWEEN_ENGINE_H__
#define __I_OKAY_TWEEN_ENGINE_H__

class IOkayTween {
   public:
    virtual void start() = 0;
    virtual void tick() = 0;
    virtual bool isFinished() = 0;
    virtual void endTween() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void kill() = 0;
    virtual ~IOkayTween() = default;
};

#endif
