#include "okay/core/tween/okay_tween_sequence.hpp"
#include "okay/core/tween/i_okay_tween.hpp"

#include <cstdint>

using namespace okay;

void OkayTweenSequence::append(std::shared_ptr<IOkayTween> tweenPtr) {
    _sequence.push_back(tweenPtr);
}

void OkayTweenSequence::start() {
    for (std::shared_ptr<IOkayTween> tween : _sequence) {
        tween->start();
    }
}

void OkayTweenSequence::pause() {
    std::vector<std::uint64_t> tweenIndicesToErase;

    for (std::uint64_t i {}; i < _sequence.size(); ++i) {
        auto tween { _sequence[i] };

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

void OkayTweenSequence::resume() {
    std::vector<std::uint64_t> tweenIndicesToErase;

    for (std::uint64_t i {}; i < _sequence.size(); ++i) {
        auto tween { _sequence[i] };

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

void OkayTweenSequence::kill() {
    for (std::shared_ptr<IOkayTween> tween : _sequence) {
        tween->kill();
    }

    _sequence.clear();

    // okay::Engine.logger.debug("Size: {}", _sequence.size());
}

void OkayTweenSequence::removeTween(std::uint64_t index) {
    _sequence.erase(_sequence.begin() + static_cast<std::uint32_t>(index));
}
