#ifndef __STRING_LOADER_H__
#define __STRING_LOADER_H__

#include <filesystem>
#include <istream>
#include <string>

#include <okay/core/util/result.hpp>

using namespace okay;

template <typename T>
struct OkayAssetLoader {
    static Result<T> loadAsset(const std::filesystem::path& path, std::istream& file) {
        static_assert(sizeof(T) == 0,
                      "No OkayAssetLoader<T> specialization found for this asset type.");
        return Result<T>::errorResult("No loader");
    }
};

template <>
struct OkayAssetLoader<std::string> {
    static Result<std::string> loadAsset(const std::filesystem::path& path, std::istream& file);
};

#endif  // __STRING_LOADER_H__