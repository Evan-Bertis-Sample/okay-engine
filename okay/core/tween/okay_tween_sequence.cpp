#include "okay/core/tween/okay_tween_sequence.hpp"
#include "okay/core/tween/i_okay_tween.hpp"
#include "okay/core/tween/okay_tween_engine.hpp"
#include <okay/core/okay.hpp>

using namespace okay;

void OkayTweenSequence::append(std::shared_ptr<IOkayTween> tweenPtr) {
    if (_started) {
        okay::Engine.logger.error("Could not append tween. Sequence has already started.\n");
        return;
    }
    
    _sequence.push_back(tweenPtr);
}

void OkayTweenSequence::start() {
    if (_sequence.size() > 0) {
        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(this->shared_from_this());
        _sequence[_index]->setIsTweening(true);
        _started = true;
    } else {
        okay::Engine.logger.error("Sequence is empty\n");
    }
}

void OkayTweenSequence::tick() {
    if (_sequence[_index]->isFinished()) {
        _sequence[_index]->reset();
        ++_index;
        
        if (!isFinished()) {
            _sequence[_index]->setIsTweening(true);
        }
    } else {
        _sequence[_index]->tick();
    }
}

void OkayTweenSequence::pause() {
    if (!_started) {
        okay::Engine.logger.error("Could not pause. Sequence has not started.\n");
        return;
    }

    _sequence[_index]->pause();
}

void OkayTweenSequence::resume() {
    if (!_started) {
        okay::Engine.logger.error("Could not resume. Sequence has not started.\n");
        return;
    }

    _sequence[_index]->resume();
}

void OkayTweenSequence::kill() {
    for (auto& tween : _sequence) {
        tween->kill();
    }
    
    _sequence.clear();
    _index = 0;
}

bool OkayTweenSequence::isFinished() {
    return _index >= _sequence.size();
}

void OkayTweenSequence::reset() {
    _index = 0;
    _started = false;
    setIsTweening(false);
}

void OkayTweenSequence::setIsTweening(bool isTweening) {
    if (_sequence.size() > 0) {
        _sequence[_index]->setIsTweening(isTweening);
    }
}
