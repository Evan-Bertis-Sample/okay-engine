#include "element.hpp"

#include <okay/core/ui/render_resources.hpp>

namespace okay {

UIElement UIElementGenerator::Iterator::operator*() const {
    return generator(current);
}

};  // namespace okay