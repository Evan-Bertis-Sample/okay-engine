#include "tween_engine.hpp"

using namespace okay;

void TweenEngine::addTween(std::shared_ptr<ITween> tween) {
    _activeTweens.push_back(std::move(tween));
}

void TweenEngine::tick() {
    std::vector<std::uint64_t> tweenIndicesToErase;
    for (std::uint64_t i{}; i < _activeTweens.size(); ++i) {
        std::shared_ptr<ITween>& tween{_activeTweens[i]};

        if (!tween->isFinished()) {
            tween->tick();
        } else {
            tween->reset();
            tweenIndicesToErase.push_back(i);
        }
    }

    for (auto it{tweenIndicesToErase.rbegin()}; it != tweenIndicesToErase.rend(); ++it) {
        removeTween(*it);
    }
}

void TweenEngine::removeTween(std::size_t index) {
    _activeTweens.erase(_activeTweens.begin() + static_cast<std::uint32_t>(index));
}
