#ifndef __TWEEN_EASING_H__
#define __TWEEN_EASING_H__

#include <cmath>
#include <functional>

namespace okay {

/**
 * @brief Easing functions adapted from https://easings.net/ for use in OkayTween.
 *
 * @param progress interpolation percentage completed
 */
using EasingFn = std::function<float(float progress)>;

namespace easing {
float linear(float progress);

float sineIn(float progress);
float sineOut(float progress);
float sineInOut(float progress);

float quadIn(float progress);
float quadOut(float progress);
float quadInOut(float progress);

float cubicIn(float progress);
float cubicOut(float progress);
float cubicInOut(float progress);

float quartIn(float progress);
float quartOut(float progress);
float quartInOut(float progress);

float quintIn(float progress);
float quintOut(float progress);
float quintInOut(float progress);

float expoIn(float progress);
float expoOut(float progress);
float expoInOut(float progress);

float circIn(float progress);
float circOut(float progress);
float circInOut(float progress);

float elasticIn(float progress);
float elasticOut(float progress);
float elasticInOut(float progress);

float backIn(float progress);
float backOut(float progress);
float backInOut(float progress);

float bounceIn(float progress);
float bounceOut(float progress);
float bounceInOut(float progress);

};  // namespace easing

};  // namespace okay

#endif
