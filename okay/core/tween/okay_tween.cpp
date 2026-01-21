#include "okay_tween.hpp"
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include "okay/core/tween/okay_tween_engine.hpp"

using namespace okay;

void OkayTween::start() {
    okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(this);
}

void OkayTween::tick() {
    _timeElapsed += okay::Engine.time->deltaTime();
    _current = START + static_cast<float>(_timeElapsed) / _duration * DISTANCE;

    // specific logger.debug for a vec3 Tween
    okay::Engine.logger.debug("\nCurrent val: ({}, {}, {})\nTime elapsed: {}\nDeltaMs: {}",
    _current.x, _current.y, _current.z, _timeElapsed, okay::Engine.time->deltaTime());
}

std::uint32_t OkayTween::timeRemaining() {
    return _duration - _timeElapsed;
}

void OkayTween::endTween() {
    START = END;
};
