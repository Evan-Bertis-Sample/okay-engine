#ifndef __OKAY_TWEEN_ENGINE_H__
#define __OKAY_TWEEN_ENGINE_H__

#include <okay/core/system/okay_system.hpp>
#include "okay_tween.hpp"

namespace okay {

    template <typename T>
    class OkayTweenEngine : public okay::OkaySystem<okay::OkaySystemScope::ENGINE> {
       public:
        OkayTweenEngine()
        {}
        
        void addTween(okay::OkayTween<T> tween);

        void initialize() override;

        void postInitialize() override;

        void tick() override;

        void postTick() override;

        void shutdown() override;

       private:
        std::vector<okay::OkayTween<T>> _activeTweens;
    };
    
}; // namespace okay

#endif
