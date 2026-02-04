#include "okay/core/tween/okay_tween_sequence.hpp"
#include "okay/core/tween/i_okay_tween.hpp"
#include <okay/core/okay.hpp>

using namespace okay;

void OkayTweenSequence::append(std::shared_ptr<IOkayTween> tweenPtr) {
    _sequence.push_back(tweenPtr);
}

void OkayTweenSequence::start() {
    if (_sequence.size() == 0) {
        okay::Engine.logger.error("Sequence is empty\n");
        return;
    }

    _sequence[0]->start();
}

void OkayTweenSequence::tick() {
    if (_sequence.size() == 0) return;

    if (_sequence[0]->isFinished()) {
        _sequence[0]->kill();
        _sequence.erase(_sequence.begin());
        
        if (_sequence.size() > 0) {
            _sequence[0]->start();
        }
    }
}

void OkayTweenSequence::pause() {
    if (_sequence.size() == 0) {
        okay::Engine.logger.error("Sequence is empty\n");
        return;
    }

    _sequence[0]->pause();
}

void OkayTweenSequence::resume() {
    if (_sequence.size() == 0) {
        okay::Engine.logger.error("Sequence is empty\n");
        return;
    }
    
    _sequence[0]->resume();
}

void OkayTweenSequence::kill() {
    if (_sequence.size() == 0) {
        okay::Engine.logger.error("Sequence is empty\n");
        return;
    }

    for (auto& tween : _sequence) {
        tween->kill();
    }

    _sequence.clear();
}
