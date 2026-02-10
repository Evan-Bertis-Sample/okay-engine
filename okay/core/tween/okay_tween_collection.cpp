#include "okay/core/tween/okay_tween_collection.hpp"
#include "okay/core/okay.hpp"
#include "okay/core/tween/i_okay_tween.hpp"
#include "okay/core/tween/okay_tween_engine.hpp"

using namespace okay;

void OkayTweenCollection::append(std::shared_ptr<IOkayTween> tweenPtr) {
    _collection.push_back(tweenPtr);
}

void OkayTweenCollection::start() {
    if (_collection.size() > 0) {
        okay::Engine.systems.getSystemChecked<OkayTweenEngine>()->addTween(this->shared_from_this());
        for (auto& tween : _collection) {
            tween->reset();
            tween->setIsTweening(true);
        }
    } else {
        okay::Engine.logger.error("Collection is empty\n");
    }
}

void OkayTweenCollection::tick() {
    for (auto& tween : _collection) {
        tween->tick();
    }
}

void OkayTweenCollection::pause() {
    for (auto& tween : _collection) {
        tween->pause();
    }
}

void OkayTweenCollection::resume() {
    for (auto& tween : _collection) {
        tween->resume();
    }
}

void OkayTweenCollection::kill() {
    for (std::shared_ptr<IOkayTween> tween : _collection) {
        tween->kill();
    }

    _collection.clear();
}

bool OkayTweenCollection::isFinished() {
    bool allTweensFinished { true };

    for (auto& tween : _collection) {
        if (!tween->isFinished()) allTweensFinished = false;
    }
    
    return allTweensFinished;
}

void OkayTweenCollection::reset() {
    for (auto& tween : _collection) {
        tween->reset();
    }
}

void OkayTweenCollection::setIsTweening(bool isTweening) {
    for (auto& tween : _collection) {
        tween->setIsTweening(isTweening);
    }
}
