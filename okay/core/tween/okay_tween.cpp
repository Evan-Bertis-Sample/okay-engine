#include "okay_tween.hpp"
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>

using namespace okay;

namespace okay {

enum OkayTweenEasingStyle : std::uint8_t { LINEAR, SINE, CUBIC, QUAD, QUINT, CIRC, ELASTIC, QUART, EXPO, BACK, BOUNCE };
enum OkayTweenEasingDirection : std::uint8_t { IN, OUT, INOUT };

}; // namespace okay


