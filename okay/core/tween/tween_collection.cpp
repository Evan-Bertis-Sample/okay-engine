#include "tween_collection.hpp"

#include "i_okay_tween.hpp"
#include "tween_engine.hpp"

#include <okay/core/engine/engine.hpp>

using namespace okay;

void TweenCollection::append(std::shared_ptr<ITween> tweenPtr) {
    _collection.push_back(tweenPtr);
}

void TweenCollection::start() {
    if (_collection.size() > 0) {
        okay::Engine.systems.getSystemChecked<TweenEngine>()->addTween(this->shared_from_this());
        for (auto& tween : _collection) {
            tween->reset();
            tween->setIsTweening(true);
        }
    } else {
        okay::Engine.logger.error("Collection is empty\n");
    }
}

void TweenCollection::tick() {
    for (auto& tween : _collection) {
        tween->tick();
    }
}

void TweenCollection::pause() {
    for (auto& tween : _collection) {
        tween->pause();
    }
}

void TweenCollection::resume() {
    for (auto& tween : _collection) {
        tween->resume();
    }
}

void TweenCollection::kill() {
    for (std::shared_ptr<ITween> tween : _collection) {
        tween->kill();
    }

    _collection.clear();
}

bool TweenCollection::isFinished() {
    bool allTweensFinished{true};

    for (auto& tween : _collection) {
        if (!tween->isFinished())
            allTweensFinished = false;
    }

    return allTweensFinished;
}

void TweenCollection::reset() {
    for (auto& tween : _collection) {
        tween->reset();
    }
}

void TweenCollection::setIsTweening(bool isTweening) {
    for (auto& tween : _collection) {
        tween->setIsTweening(isTweening);
    }
}
