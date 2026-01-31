#ifndef __OKAY_TWEEN_ENGINE_H__
#define __OKAY_TWEEN_ENGINE_H__

#include <memory>
#include <okay/core/system/okay_system.hpp>
#include <vector>
#include "okay/core/tween/i_okay_tween.hpp"

/**
  * @brief OkayTween manager.
  */
namespace okay {
    class OkayTweenEngine : public OkaySystem<OkaySystemScope::ENGINE> {
       public:
        OkayTweenEngine()
        {}
        
        /**
          * @brief Push back tween ptr into _activeTweens.
          * 
          * @param tween shared_ptr to a tween
          */
        void addTween(std::shared_ptr<IOkayTween> tween);
        
        /**
          * @brief Tick and remove finished tweens.
          */
        void tick() override;
        
       private:
        std::vector<std::shared_ptr<IOkayTween>> _activeTweens;

        /**
          * @brief Erase tween ptr from _activeTweens at given index.
          * 
          * @param index index of tween ptr
          */
        void removeTween(std::uint64_t index);
    };
    
}; // namespace okay

#endif
