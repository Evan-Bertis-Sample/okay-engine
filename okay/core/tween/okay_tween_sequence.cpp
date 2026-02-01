#include "okay/core/tween/okay_tween_sequence.hpp"
#include "okay/core/tween/i_okay_tween.hpp"
#include <okay/core/okay.hpp> // for erroring

#include <cstdint>

using namespace okay;

void OkayTweenSequence::append(std::shared_ptr<IOkayTween> tweenPtr) {
    _sequence.push_back(tweenPtr);
}

void OkayTweenSequence::start() {
    if (_sequence.size() > 0) {
        _sequence[_index]->start();
    } else {
        okay::Engine.logger.error("Sequence is empty\n");
    }
}

void OkayTweenSequence::tick() {
    if (_index >= _sequence.size()) return;

    if (_sequence[_index]->isFinished()) {
        _sequence[_index]->kill();
        ++_index;
        if (_index < _sequence.size()) {
            _sequence[_index]->start();
        }
    }
}


void OkayTweenSequence::pause() {
    _sequence[_index]->pause();
}

void OkayTweenSequence::resume() {
    _sequence[_index]->resume();
}

void OkayTweenSequence::kill() {
    for (std::shared_ptr<IOkayTween> tween : _sequence) {
        tween->kill();
    }

    _sequence.clear();
}
