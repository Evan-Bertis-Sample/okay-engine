#ifndef __OKAY_TWEEN_ENGINE_H__
#define __OKAY_TWEEN_ENGINE_H__

#include <memory>
#include <okay/core/system/okay_system.hpp>
#include <vector>
#include "okay/core/tween/i_okay_tween.hpp"

namespace okay {
    class OkayTweenEngine : public OkaySystem<OkaySystemScope::ENGINE> {
       public:
        OkayTweenEngine()
        {}
        
        void addTween(std::shared_ptr<IOkayTween> tween);
        
        void tick() override;
        
       private:
        std::vector<std::shared_ptr<IOkayTween>> _activeTweens;

        void removeTween(std::uint64_t index);
    };
    
}; // namespace okay

#endif
