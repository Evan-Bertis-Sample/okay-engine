#ifndef __OKAY_TWEEN_ENGINE_H__
#define __OKAY_TWEEN_ENGINE_H__

#include <memory>
#include <okay/core/system/okay_system.hpp>
#include "okay_tween.hpp"

namespace okay {

    class OkayTweenEngine : public OkaySystem<OkaySystemScope::ENGINE> {
       public:
        OkayTweenEngine()
        {}
        
        void addTween(IOkayTween& tween) {
            _activeTweens.push_back(std::make_unique<IOkayTween>(tween));
        }
        
        void removeTween(std::uint64_t index) {
            _activeTweens.erase(_activeTweens.begin() + static_cast<std::uint32_t>(index));
        }

        void initialize() override;

        void postInitialize() override;

        void tick() override {
            std::vector<std::uint64_t> tweenIndicesToErase;
            for (std::uint64_t i {}; i < _activeTweens.size(); ++i) {
                std::unique_ptr<IOkayTween>& tween { _activeTweens[i] };

                if (tween->timeRemaining() <= 0) {
                    tween->tick();
                } else {
                    tween->endTween();
                    tweenIndicesToErase.push_back(i);
                }
            }

            for (std::uint64_t i { tweenIndicesToErase.size() }; i >= 0; --i) {
                removeTween(i);
            }
        }

        void postTick() override;

        void shutdown() override;

       private:
        std::vector<std::unique_ptr<IOkayTween>> _activeTweens;
    };
    
}; // namespace okay

#endif
