#ifndef __UTIL_H__
#define __UTIL_H__

#include "okay/core/ui/font.hpp"

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/generic/font_loader.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>
#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/mesh.hpp>

#include <filesystem>
#include <typeinfo>

namespace okay {

template <typename T>
inline T unwrapAssetResult(Result<Asset<T>> res) {
    if (res.isError()) {
        Engine.logger.error("Failed to load asset of type {}: {}", typeid(T).name(), res.error());
        return T{};
    }
    return res.value().asset;
}

template <typename T>
using Load = AssetManager::Load<T>;

namespace detail {

template <typename AssetT, bool engineAsset, typename... LoadOptions>
inline AssetT loadAsset(const std::filesystem::path& path,
                        SystemParameter<AssetManager> assetManager,
                        LoadOptions&&... options) {
    if constexpr (engineAsset) {
        return unwrapAssetResult(
            assetManager->loadEngineAssetSync<AssetT>(path, std::forward<LoadOptions>(options)...));
    } else {
        return unwrapAssetResult(
            assetManager->loadGameAssetSync<AssetT>(path, std::forward<LoadOptions>(options)...));
    }
}

}  // namespace detail

namespace load {

// game assets
template <typename AssetT, typename... LoadOptions>
inline AssetT asset(const std::filesystem::path& path,
                    SystemParameter<AssetManager> assetManager = nullptr,
                    LoadOptions&&... options) {
    return detail::loadAsset<AssetT, false>(
        path, assetManager, std::forward<LoadOptions>(options)...);
}

// engine assets
template <typename AssetT, typename... LoadOptions>
inline AssetT engine(const std::filesystem::path& path,
                     SystemParameter<AssetManager> assetManager = nullptr,
                     LoadOptions&&... options) {
    return detail::loadAsset<AssetT, true>(
        path, assetManager, std::forward<LoadOptions>(options)...);
}

// optional convenience wrappers
inline FontManager::FontHandle font(const std::filesystem::path& path,
                                    SystemParameter<AssetManager> assetManager = nullptr,
                                    FontLoadOptions options = {}) {
    return asset<FontManager::FontHandle>(path, assetManager, options);
}

inline Shader shader(const std::filesystem::path& path,
                     SystemParameter<AssetManager> assetManager = nullptr) {
    return asset<Shader>(path, assetManager);
}

inline Texture texture(const std::filesystem::path& path,
                       SystemParameter<AssetManager> assetManager = nullptr,
                       TextureLoadSettings settings = {}) {
    return asset<Texture>(path, assetManager, settings);
}

inline MeshData meshData(const std::filesystem::path& path,
                         SystemParameter<AssetManager> assetManager = nullptr) {
    return asset<MeshData>(path, assetManager);
}

inline FontManager::FontHandle engineFont(const std::filesystem::path& path,
                                          SystemParameter<AssetManager> assetManager = nullptr,
                                          FontLoadOptions options = {}) {
    return engine<FontManager::FontHandle>(path, assetManager, options);
}

inline Shader engineShader(const std::filesystem::path& path,
                           SystemParameter<AssetManager> assetManager = nullptr) {
    return engine<Shader>(path, assetManager);
}

inline Texture engineTexture(const std::filesystem::path& path,
                             SystemParameter<AssetManager> assetManager = nullptr,
                             TextureLoadSettings settings = {}) {
    return engine<Texture>(path, assetManager, settings);
}

inline MeshData engineMeshData(const std::filesystem::path& path,
                               SystemParameter<AssetManager> assetManager = nullptr) {
    return engine<MeshData>(path, assetManager);
}

}  // namespace load

}  // namespace okay

#endif  // __UTIL_H__