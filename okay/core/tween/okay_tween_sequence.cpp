#include "okay/core/tween/okay_tween_sequence.hpp"
#include "okay/core/tween/i_okay_tween.hpp"

void okay::OkayTweenSequence::append(std::shared_ptr<IOkayTween> tweenPtr) {
    _sequence.push_back(tweenPtr);
}

void okay::OkayTweenSequence::start() {
    for (std::shared_ptr<IOkayTween>& tween : _sequence) {
        tween.get()->start();
    }
}

void okay::OkayTweenSequence::pause() {
    for (std::shared_ptr<IOkayTween>& tween : _sequence) {
        tween.get()->pause();
    }
}

void okay::OkayTweenSequence::resume() {
    for (std::shared_ptr<IOkayTween>& tween : _sequence) {
        tween.get()->resume();
    }
}

void okay::OkayTweenSequence::kill() {
    for (std::shared_ptr<IOkayTween>& tween : _sequence) {
        tween.get()->kill();
    }
}
