#ifndef __OKAY_ASSET_LOADER_STRING_HPP__
#define __OKAY_ASSET_LOADER_STRING_HPP__

#include <filesystem>
#include <istream>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/util/result.hpp>
#include <string>

namespace okay {

template <>
struct OkayAssetLoader<std::string> {
    using StreamElementType = char;
    
    static Result<std::string> loadAsset(const std::filesystem::path& path,
                                         std::basic_istream<StreamElementType>& file) {
        std::ostringstream ss;
        ss << file.rdbuf();
        if (file.bad()) {
            return Result<std::string>::errorResult(
                "String load failed: stream became bad while reading.");
        }
        return Result<std::string>::ok(ss.str());
    }
};

}  // namespace okay

#endif  // __OKAY_ASSET_LOADER_STRING_HPP__
