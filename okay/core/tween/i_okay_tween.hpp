#ifndef __I_OKAY_TWEEN_ENGINE_H__
#define __I_OKAY_TWEEN_ENGINE_H__

namespace okay {

/**
 * @brief Interface for OkayTween dependency injection
 */
class IOkayTween {
   public:
    virtual void start() = 0;
    virtual void tick() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void reset() = 0;
    virtual void kill() = 0;
    virtual bool isFinished() = 0;
    virtual void setIsTweening(bool isTweening) = 0;
    virtual ~IOkayTween() = default;
};

} // namespace okay

#endif
