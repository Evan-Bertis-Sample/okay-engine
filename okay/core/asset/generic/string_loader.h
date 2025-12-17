#ifndef __OKAY_ASSET_LOADER_STRING_HPP__
#define __OKAY_ASSET_LOADER_STRING_HPP__

#include <filesystem>
#include <istream>
#include <string>

#include <okay/core/asset/okay_asset.h>
#include <okay/core/util/result.hpp>

namespace okay {

template <>
struct OkayAssetLoader<std::string> {
    static Result<std::string> loadAsset(const std::filesystem::path& path, std::istream& file);
};

}  // namespace okay

#endif  // __OKAY_ASSET_LOADER_STRING_HPP__
