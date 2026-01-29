#include "okay_tween_engine.hpp"

using namespace okay;

void OkayTweenEngine::addTween(std::shared_ptr<IOkayTween> tween) {
    _activeTweens.push_back(std::move(tween));
}

void OkayTweenEngine::removeTween(std::uint64_t index) {
    _activeTweens.erase(_activeTweens.begin() + static_cast<std::uint32_t>(index));
}

void OkayTweenEngine::tick() {
    std::vector<std::uint64_t> tweenIndicesToErase;
    for (std::uint64_t i {}; i < _activeTweens.size(); ++i) {
        std::shared_ptr<IOkayTween>& tween { _activeTweens[i] };
        
        if (!tween->isFinished()) {
            // Engine.logger.debug("{}", tween->timeRemaining());
            tween->tick();
        } else {
            tween->endTween();
            tweenIndicesToErase.push_back(i);
        }
    }

    for (auto it = tweenIndicesToErase.rbegin(); it != tweenIndicesToErase.rend(); ++it) {
        removeTween(*it);
    }
}
