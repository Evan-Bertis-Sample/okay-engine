#include "okay_tween_engine.hpp"
#include "okay_tween.hpp"

using namespace okay;

void OkayTweenEngine::addTween(std::unique_ptr<IOkayTween> tween) {
    _activeTweens.push_back(std::move(tween));
}

void OkayTweenEngine::removeTween(std::uint64_t index) {
    _activeTweens.erase(_activeTweens.begin() + static_cast<std::uint32_t>(index));
}

void OkayTweenEngine::tick() {
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

    for (auto it = tweenIndicesToErase.rbegin(); it != tweenIndicesToErase.rend(); ++it) {
        removeTween(*it);
    }
}
