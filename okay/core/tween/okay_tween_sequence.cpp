#include <functional>
#include "okay/core/tween/okay_tween_sequence.hpp"
#include "okay/core/okay.hpp"
#include "okay/core/tween/i_okay_tween.hpp"

void okay::OkayTweenSequence::append(IOkayTween& tween) {
    _sequence.push_back(std::reference_wrapper<IOkayTween>(tween));
}

void okay::OkayTweenSequence::start() {
    for (std::reference_wrapper<IOkayTween>& tween : _sequence) {
        okay::Engine.logger.debug("starting sequence tweens!");
        tween.get().start();
    }
}

void okay::OkayTweenSequence::pause() {
    for (std::reference_wrapper<IOkayTween>& tween : _sequence) {
        tween.get().pause();
    }
}

void okay::OkayTweenSequence::resume() {
    for (std::reference_wrapper<IOkayTween>& tween : _sequence) {
        tween.get().resume();
    }
}

void okay::OkayTweenSequence::kill() {
    for (std::reference_wrapper<IOkayTween>& tween : _sequence) {
        tween.get().kill();
    }
}
