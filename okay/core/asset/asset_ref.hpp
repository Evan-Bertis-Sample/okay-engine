#ifndef __ASSET_REF_H__
#define __ASSET_REF_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/asset_util.hpp>
#include <okay/core/asset/generic/font_loader.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>
#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/engine/system.hpp>

#include <filesystem>
#include <tuple>

namespace okay {

template <typename T, auto engineAsset = false, typename LoadOptions = std::tuple<>>
class AssetRef {
   public:
    T asset{};
    bool loaded{false};
    SystemParameter<AssetManager> assetManager;
    std::filesystem::path path;
    LoadOptions loadOptions;

    AssetRef(const std::filesystem::path& path,
        SystemParameter<AssetManager> am = nullptr,
        LoadOptions loadOptions = LoadOptions{})
        : assetManager(am), path(path), loadOptions(loadOptions) {}

    T& operator*() {
        if (!loaded)
            load();

        return asset;
    }

    T* operator->() {
        if (!loaded)
            load();

        return &asset;
    }

    ~AssetRef() {
        // TODO: Unload this asset!
        // Increment/decrement asset
    }

   private:
    void load() {
        if constexpr (engineAsset) {
            asset = unwrapAssetResult(
                assetManager->loadEngineAssetSync<T, AssetManager::DefaultAssetIO, LoadOptions>(
                    path, loadOptions));
        } else {
            asset = unwrapAssetResult(
                assetManager->loadGameAssetSync<T, AssetManager::DefaultAssetIO, LoadOptions>(
                    path, loadOptions));
        }
        loaded = true;
    }
};

template <typename T, typename LoadOptions = std::tuple<>>
using GameAssetRef = AssetRef<T, false, LoadOptions>;

template <typename T, typename LoadOptions = std::tuple<>>
using EngineAssetRef = AssetRef<T, true, LoadOptions>;

}  // namespace okay

#endif  // __ASSET_REF_H__
