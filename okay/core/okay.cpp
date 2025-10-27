#include "okay.hpp"

#include <iostream>
#include <okay/core/level/okay_level_manager.hpp>

namespace okay {

OkayEngine Engine;

const std::vector<OkaySystemDescriptor> OkayGame::_REQUIRED_SYSTEMS = {
    OkaySystemDescriptor::create<OkayLevelManager>()
};

}  // namespace okay
