#include "okay.hpp"
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>

namespace okay {

OkayEngine Engine;

const std::vector<OkaySystemDescriptor> OkayGame::REQUIRED_SYSTEMS = {
    OkaySystemDescriptor::create<OkayLevelManager>(),
    OkaySystemDescriptor::create<OkayAssetManager>(),
};

}  // namespace okay
