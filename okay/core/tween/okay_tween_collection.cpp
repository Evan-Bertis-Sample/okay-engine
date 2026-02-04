#include "okay/core/tween/okay_tween_collection.hpp"
#include <cstdint>
#include "okay/core/okay.hpp"
#include "okay/core/tween/i_okay_tween.hpp"

using namespace okay;

void OkayTweenCollection::append(std::shared_ptr<IOkayTween> tweenPtr) {
    _collection.push_back(tweenPtr);
}

void OkayTweenCollection::start() {
    for (std::shared_ptr<IOkayTween> tween : _collection) {
        tween->start();
    }
}

void OkayTweenCollection::pause() {
    std::vector<std::uint64_t> tweenIndicesToErase;

    for (std::uint64_t i {}; i < _collection.size(); ++i) {
        auto tween { _collection[i] };

        if (tween->isFinished()) {
            tweenIndicesToErase.push_back(i);
        } else {
            tween->pause();
        }
    }

    for (auto it { tweenIndicesToErase.rbegin() }; it != tweenIndicesToErase.rend(); ++it) {
        removeTween(*it);
    }
}

void OkayTweenCollection::resume() {
    std::vector<std::uint64_t> tweenIndicesToErase;

    for (std::uint64_t i {}; i < _collection.size(); ++i) {
        auto tween { _collection[i] };

        if (tween->isFinished()) {
            tweenIndicesToErase.push_back(i);
        } else {
            tween->resume();
        }
    }

    for (auto it { tweenIndicesToErase.rbegin() }; it != tweenIndicesToErase.rend(); ++it) {
        removeTween(*it);
    }
}

void OkayTweenCollection::kill() {
    for (std::shared_ptr<IOkayTween> tween : _collection) {
        tween->kill();
    }

    _collection.clear();

    okay::Engine.logger.debug("Size: {}", _collection.size());
}

void OkayTweenCollection::removeTween(std::uint64_t index) {
    _collection.erase(_collection.begin() + static_cast<std::uint32_t>(index));
}
