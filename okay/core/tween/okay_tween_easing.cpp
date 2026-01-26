#include "okay_tween_easing.hpp"
#include <numbers>

namespace okay {

const std::float_t PI { std::numbers::pi };

namespace easing {

std::float_t linear(std::float_t& progress) {
    return progress;
}

std::float_t sineIn(std::float_t& progress) {
    return 1 - std::cos((progress * PI) / 2);
}

std::float_t sineOut(std::float_t& progress) {
    return std::sin((progress * PI) / 2);
}

std::float_t sineInOut(std::float_t& progress) {
    return -(std::cos(PI * progress) - 1) / 2;
}

std::float_t quadIn(std::float_t& progress) {
    return progress * progress;
}

std::float_t quadOut(std::float_t& progress) {
    return 1 - (1 - progress) * (1 - progress);
}

std::float_t quadInOut(std::float_t& progress) {
    return progress < 0.5 ? 2 * progress * progress : 1 - std::pow(-2 * progress + 2, 2) / 2;
}

std::float_t cubicIn(std::float_t& progress) {
    return progress * progress * progress;
}

std::float_t cubicOut(std::float_t& progress) {
    return 1 - std::pow(1 - progress, 3);
}

std::float_t cubicInOut(std::float_t& progress) {
    return progress < 0.5
            ? 4 * progress * progress * progress
            : 1 - std::pow(-2 * progress + 2, 3) / 2;
}

std::float_t quartIn(std::float_t& progress) {
    return progress * progress * progress * progress;
}

std::float_t quartOut(std::float_t& progress) {
    return 1 - std::pow(1 - progress, 4);
}

std::float_t quartInOut(std::float_t& progress) {
    return progress < 0.5
            ? 8 * progress * progress * progress * progress
            : 1 - std::pow(-2 * progress + 2, 4) / 2;
}

std::float_t quintIn(std::float_t& progress) {
    return progress * progress * progress * progress * progress;
}

std::float_t quintOut(std::float_t& progress) {
    return 1 - std::pow(1 - progress, 5);
}

std::float_t quintInOut(std::float_t& progress) {
    return progress < 0.5
            ? 16 * progress * progress * progress * progress * progress
            : 1 - std::pow(-2 * progress + 2, 5) / 2;
}

std::float_t expoIn(std::float_t& progress) {
    return progress == 0 ? 0 : std::pow(2, 10 * progress - 10);
}

std::float_t expoOut(std::float_t& progress) {
    return progress == 1 ? 1 : 1 - std::pow(2, -10 * progress);
}

std::float_t expoInOut(std::float_t& progress) {
    return progress == 0
            ? 0
            : progress == 1
            ? 1
            : progress < 0.5 ? std::pow(2, 20 * progress - 10) / 2
            : (2 - std::pow(2, -20 * progress + 10)) / 2;
}

std::float_t circIn(std::float_t& progress) {
    return 1 - std::sqrt(1 - progress * progress);
}

std::float_t circOut(std::float_t& progress) {
    return std::sqrt(1 - std::pow(progress - 1, 2));
}

std::float_t circInOut(std::float_t& progress) {
    return progress < 0.5
            ? (1 - std::sqrt(1 - std::pow(2 * progress, 2))) / 2
            : (std::sqrt(1 - std::pow(-2 * progress + 2, 2)) + 1) / 2;
}

std::float_t elasticIn(std::float_t& progress) {
    const std::float_t C4 { (2 * PI) / 3 };

    return progress == 0
            ? 0
            : progress == 1
            ? 1
            : -std::pow(2, 10 * progress - 10) * std::sin((progress * 10 - 10.75) * C4);
}

std::float_t elasticOut(std::float_t& progress) {
    const std::float_t C4 { (2 * PI) / 3 };

    return progress == 0
            ? 0
            : progress == 1
            ? 1
            : std::pow(2, -10 * progress) * std::sin((progress * 10 - 0.75) * C4) + 1;
}

std::float_t elasticInOut(std::float_t& progress) {
    const std::float_t C5 { (2 * PI) / 4.5f };

    return progress == 0
            ? 0
            : progress == 1
            ? 1
            : progress < 0.5
            ? -(std::pow(2, 20 * progress - 10) * std::sin((20 * progress - 11.125) * C5)) / 2
            : (std::pow(2, -20 * progress + 10) * std::sin((20 * progress - 11.125) * C5)) / 2 + 1;
}

std::float_t backIn(std::float_t& progress) {
    const std::float_t C1 { 1.70158 };
    const std::float_t C3 { C1 + 1 };

    return C3 * progress * progress * progress - C1 * progress * progress;
}

std::float_t backOut(std::float_t& progress) {
    const std::float_t C1 { 1.70158 };
    const std::float_t C3 { C1 + 1 };

    return 1 + C3 * std::pow(progress - 1, 3) + C1 * std::pow(progress - 1, 2);
}

std::float_t backInOut(std::float_t& progress) {
    const std::float_t C1 { 1.70158 };
    const std::float_t C2 { C1 * 1.525f };

    return progress < 0.5
            ? (std::pow(2 * progress, 2) * ((C2 + 1) * 2 * progress - C2)) / 2
            : (std::pow(2 * progress - 2, 2) * ((C2 + 1) * (progress * 2 - 2) + C2) + 2) / 2;
}

std::float_t bounceIn(std::float_t& progress) {
    progress = 1 - progress;
    return 1 - bounceOut(progress);
}

std::float_t bounceOut(std::float_t& progress) {
    const std::float_t N1 = 7.5625f;
    const std::float_t D1 = 2.75f;

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

std::float_t bounceInOut(std::float_t& progress) {
    if (progress < 0.5) {
        progress = 1 - 2 * progress;
        return (1 - bounceOut(progress)) / 2;
    } else {
        progress = 2 * progress - 1;
        return (1 + bounceOut(progress)) / 2;
    }
}

} // namespace easing

} // namespace okay
