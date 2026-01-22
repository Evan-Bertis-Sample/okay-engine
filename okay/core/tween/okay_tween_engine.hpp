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
        
        void addTween(std::unique_ptr<IOkayTween> tween);
        
        void removeTween(std::uint64_t index);

        // void initialize() override;

        // void postInitialize() override;

        void tick() override;

        // void postTick() override;

        // void shutdown() override;

       private:
        std::vector<std::unique_ptr<IOkayTween>> _activeTweens;
    };
    
}; // namespace okay

#endif
