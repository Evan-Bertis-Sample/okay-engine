#include "okay.hpp"

#include <iostream>
#include <okay/core/level/okay_level_manager.hpp>

namespace okay {

OkayEngine Engine;

const std::vector<std::size_t> OkayGame::_REQUIRED_SYSTEMS = {
    OkayLevelManager::hash()
};

}  // namespace okay
