#include "engine.hpp"

#include <okay/core/asset/asset.hpp>

namespace okay {

OkayEngine Engine;

const std::vector<OkaySystemDescriptor> OkayGame::REQUIRED_SYSTEMS = {
    OkaySystemDescriptor::create<OkayAssetManager>(),
};

}  // namespace okay