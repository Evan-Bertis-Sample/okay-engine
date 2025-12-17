#ifndef __OKAY_ASSET_H__
#define __OKAY_ASSET_H__

#include <filesystem>
#include <fstream>
#include <functional>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/util/result.hpp>

namespace okay {

template <typename T>
struct OkayAsset {
    T asset;
    // meta information about the asset
    std::size_t assetSize;
};

template <typename T>
struct OkayAssetLoader {
    static Result<T> loadAsset(const std::istream& file);
};

template <typename T>
using OnAssetSucceedCB = std::function<void(const OkayAsset<T>&)>;

template <typename T>
using OnAssetFailedCB = std::function<void(const std::string&)>;

class OkayAssetManager : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static constexpr const char* ENGINE_ASSET_ROOT = ".okay/engine/assets";
    static constexpr const char* GAME_ASSET_ROOT = ".okay/user/assets";

    template <typename T>
    struct Load {
       public:
        OnAssetSucceedCB<T> onCompleteCB;
        OnAssetFailedCB<T> onFailCB;
        const std::filesystem::path& assetPath;

        static Load<T> EngineAsset(const std::filesystem::path& path) {
            return Load(std::filesystem::path(ENGINE_ASSET_ROOT) / path);
        };

        static Load<T> GameAsset(const std::filesystem::path& path) {
            return Load(std::filesystem::path(GAME_ASSET_ROOT) / path);
        };

        Load<T>& onComplete(OnAssetSucceedCB<T> cb) {
            onCompleteCB = cb;
            return *this;
        }
        Load<T>& onFail(OnAssetFailedCB<T> cb) {
            onFailCB = cb;
            return *this;
        }

        std::istream data() const { return std::ifstream(assetPath); }

       private:
        Load(const std::filesystem::path& assetPath) : assetPath(assetPath) {}
    };

    template <typename T>
    struct LoadHandle {
       public:
        enum State { LOADING, COMPLETE, FAILED };
        State state;
        Result<OkayAsset<T>> asset;
    };

    template <typename T>
    LoadHandle<T> loadAssetAsync(const Load<T>& load) {
        std::istream assetFile = load.data();
        Result<T> res = OkayAssetLoader<T>::loadAsset(assetFile);

        if (res.isError()) {
            auto handleAsset = Result<OkayAsset<T>>::errorResult(res.error());
            if (load.onFailCB) load.onFailCB(res.error());

            return LoadHandle<T>{.state = LoadHandle<T>::State::FAILED, .asset = handleAsset};
        } else {
            OkayAsset<T> asset = createAsset(load.assetPath, res.value());
            auto handleAsset = Result<OkayAsset<T>>::ok(asset);
            if (load.onCompleteCB) load.onCompleteCB(asset);

            return LoadHandle<T>{.state = LoadHandle<T>::State::COMPLETE, .asset = handleAsset};
        };
    };

    template <typename T>
    Result<OkayAsset<T>> loadAssetSync(const Load<T>& load) {
        std::istream assetFile = load.data();
        Result<T> res = OkayAssetLoader<T>::loadAsset(assetFile);

        if (res.isError()) {
            return Result<OkayAsset<T>>::errorResult(res.error());
        } else {
            return Result<OkayAsset<T>>::ok(createAsset(load.assetPath, res.value()));
        };
    };

    template <typename T>
    Result<OkayAsset<T>> loadEngineAssetSync(const std::filesystem::path& path) {
        return loadAssetSync(Load<T>::EngineAsset(path));
    }

    template <typename T>
    Result<OkayAsset<T>> loadGameAssetSync(const std::filesystem::path& path) {
        return loadAssetSync(Load<T>::GameAsset(path));
    }

   private:
    template <typename T>
    inline OkayAsset<T> createAsset(const std::filesystem::path& path, T loaded) {
        return OkayAsset<T>{.asset = loaded, .assetSize = std::filesystem::file_size(path)};
    }
};

}  // namespace okay
#endif  // __OKAY_ASSET_H__