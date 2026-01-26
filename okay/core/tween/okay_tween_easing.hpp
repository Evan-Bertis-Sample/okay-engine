#ifndef __OKAY_TWEEN_EASING_H__
#define __OKAY_TWEEN_EASING_H__

#include <cmath>
#include <functional>

namespace okay {

using EasingFn = std::function<std::float_t(std::float_t& progress)>;

namespace easing {
    std::float_t linear(std::float_t& progress);

    std::float_t sineIn(std::float_t& progress);
    std::float_t sineOut(std::float_t& progress);
    std::float_t sineInOut(std::float_t& progress);

    std::float_t quadIn(std::float_t& progress);
    std::float_t quadOut(std::float_t& progress);
    std::float_t quadInOut(std::float_t& progress);

    std::float_t cubicIn(std::float_t& progress);
    std::float_t cubicOut(std::float_t& progress);
    std::float_t cubicInOut(std::float_t& progress);

    std::float_t quartIn(std::float_t& progress);
    std::float_t quartOut(std::float_t& progress);
    std::float_t quartInOut(std::float_t& progress);

    std::float_t quintIn(std::float_t& progress);
    std::float_t quintOut(std::float_t& progress);
    std::float_t quintInOut(std::float_t& progress);

    std::float_t expoIn(std::float_t& progress);
    std::float_t expoOut(std::float_t& progress);
    std::float_t expoInOut(std::float_t& progress);

    std::float_t circIn(std::float_t& progress);
    std::float_t circOut(std::float_t& progress);
    std::float_t circInOut(std::float_t& progress);

    std::float_t elasticIn(std::float_t& progress);
    std::float_t elasticOut(std::float_t& progress);
    std::float_t elasticInOut(std::float_t& progress);

    std::float_t backIn(std::float_t& progress);
    std::float_t backOut(std::float_t& progress);
    std::float_t backInOut(std::float_t& progress);

    std::float_t bounceIn(std::float_t& progress);
    std::float_t bounceOut(std::float_t& progress);
    std::float_t bounceInOut(std::float_t& progress);

}; // namespace easing

}; // namespace okay

#endif
