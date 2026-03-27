#ifndef __TWEEN_ENGINE_H__
#define __TWEEN_ENGINE_H__

#include <okay/core/engine/system.hpp>
#include <okay/core/tween/i_okay_tween.hpp>

#include <memory>
#include <vector>

/**
 * @brief OkayTween manager.
 */
namespace okay {
class TweenEngine : public System<SystemScope::ENGINE> {
   public:
    TweenEngine() {}

    /**
     * @brief Push back tween ptr into _activeTweens.
     *
     * @param tween shared_ptr to a tween
     */
    void addTween(std::shared_ptr<ITween> tween);

    /**
     * @brief Tick and remove finished tweens.
     */
    void tick() override;

   private:
    std::vector<std::shared_ptr<ITween>> _activeTweens;

    /**
     * @brief Erase tween ptr from _activeTweens at given index.
     *
     * @param index index of tween ptr
     */
    void removeTween(std::size_t index);
};

};  // namespace okay

#endif
