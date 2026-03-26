#include "tween_easing.hpp"

#include <numbers>

namespace okay {

const float PI{std::numbers::pi};

namespace easing {

float linear(float progress) {
    return progress;
}

float sineIn(float progress) {
    return 1 - std::cos((progress * PI) / 2);
}

float sineOut(float progress) {
    return std::sin((progress * PI) / 2);
}

float sineInOut(float progress) {
    return -(std::cos(PI * progress) - 1) / 2;
}

float quadIn(float progress) {
    return progress * progress;
}

float quadOut(float progress) {
    return 1 - (1 - progress) * (1 - progress);
}

float quadInOut(float progress) {
    return progress < 0.5 ? 2 * progress * progress : 1 - std::pow(-2 * progress + 2, 2) / 2;
}

float cubicIn(float progress) {
    return progress * progress * progress;
}

float cubicOut(float progress) {
    return 1 - std::pow(1 - progress, 3);
}

float cubicInOut(float progress) {
    return progress < 0.5 ? 4 * progress * progress * progress
                          : 1 - std::pow(-2 * progress + 2, 3) / 2;
}

float quartIn(float progress) {
    return progress * progress * progress * progress;
}

float quartOut(float progress) {
    return 1 - std::pow(1 - progress, 4);
}

float quartInOut(float progress) {
    return progress < 0.5 ? 8 * progress * progress * progress * progress
                          : 1 - std::pow(-2 * progress + 2, 4) / 2;
}

float quintIn(float progress) {
    return progress * progress * progress * progress * progress;
}

float quintOut(float progress) {
    return 1 - std::pow(1 - progress, 5);
}

float quintInOut(float progress) {
    return progress < 0.5 ? 16 * progress * progress * progress * progress * progress
                          : 1 - std::pow(-2 * progress + 2, 5) / 2;
}

float expoIn(float progress) {
    return progress == 0 ? 0 : std::pow(2, 10 * progress - 10);
}

float expoOut(float progress) {
    return progress == 1 ? 1 : 1 - std::pow(2, -10 * progress);
}

float expoInOut(float progress) {
    return progress == 0    ? 0
           : progress == 1  ? 1
           : progress < 0.5 ? std::pow(2, 20 * progress - 10) / 2
                            : (2 - std::pow(2, -20 * progress + 10)) / 2;
}

float circIn(float progress) {
    return 1 - std::sqrt(1 - progress * progress);
}

float circOut(float progress) {
    return std::sqrt(1 - std::pow(progress - 1, 2));
}

float circInOut(float progress) {
    return progress < 0.5 ? (1 - std::sqrt(1 - std::pow(2 * progress, 2))) / 2
                          : (std::sqrt(1 - std::pow(-2 * progress + 2, 2)) + 1) / 2;
}

float elasticIn(float progress) {
    const float C4{(2 * PI) / 3};

    return progress == 0 ? 0
           : progress == 1
               ? 1
               : -std::pow(2, 10 * progress - 10) * std::sin((progress * 10 - 10.75) * C4);
}

float elasticOut(float progress) {
    const float C4{(2 * PI) / 3};

    return progress == 0 ? 0
           : progress == 1
               ? 1
               : std::pow(2, -10 * progress) * std::sin((progress * 10 - 0.75) * C4) + 1;
}

float elasticInOut(float progress) {
    const float C5{(2 * PI) / 4.5f};

    return progress == 0   ? 0
           : progress == 1 ? 1
           : progress < 0.5
               ? -(std::pow(2, 20 * progress - 10) * std::sin((20 * progress - 11.125) * C5)) / 2
               : (std::pow(2, -20 * progress + 10) * std::sin((20 * progress - 11.125) * C5)) / 2 +
                     1;
}

float backIn(float progress) {
    const float C1{1.70158};
    const float C3{C1 + 1};

    return C3 * progress * progress * progress - C1 * progress * progress;
}

float backOut(float progress) {
    const float C1{1.70158};
    const float C3{C1 + 1};

    return 1 + C3 * std::pow(progress - 1, 3) + C1 * std::pow(progress - 1, 2);
}

float backInOut(float progress) {
    const float C1{1.70158};
    const float C2{C1 * 1.525f};

    return progress < 0.5
               ? (std::pow(2 * progress, 2) * ((C2 + 1) * 2 * progress - C2)) / 2
               : (std::pow(2 * progress - 2, 2) * ((C2 + 1) * (progress * 2 - 2) + C2) + 2) / 2;
}

float bounceIn(float progress) {
    progress = 1 - progress;
    return 1 - bounceOut(progress);
}

float bounceOut(float progress) {
    const float N1 = 7.5625f;
    const float D1 = 2.75f;

    if (progress < 1.0f / D1) {
        return N1 * progress * progress;
    } else if (progress < 2.0f / D1) {
        progress -= 1.5f / D1;
        return N1 * progress * progress + 0.75f;
    } else if (progress < 2.5f / D1) {
        progress -= 2.25f / D1;
        return N1 * progress * progress + 0.9375f;
    } else {
        progress -= 2.625f / D1;
        return N1 * progress * progress + 0.984375f;
    }
}

float bounceInOut(float progress) {
    if (progress < 0.5) {
        progress = 1 - 2 * progress;
        return (1 - bounceOut(progress)) / 2;
    } else {
        progress = 2 * progress - 1;
        return (1 + bounceOut(progress)) / 2;
    }
}

}  // namespace easing

}  // namespace okay
