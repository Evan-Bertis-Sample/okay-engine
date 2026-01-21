#include "okay_tween_engine.hpp"


using namespace okay;

template <typename T>
void OkayTweenEngine::addTween(OkayTween<T> tween) {
    _activeTweens.push_back(tween);
}
