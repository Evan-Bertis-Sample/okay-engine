#include "engine.hpp"

#include <okay/core/asset/asset.hpp>

namespace okay {

OkayEngine Engine;

const std::vector<OkaySystemDescriptor> Game::REQUIRED_SYSTEMS = {
    OkaySystemDescriptor::create<AssetManager>(),
};

}  // namespace okay