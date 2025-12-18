#ifndef __OKAY_ASSET_H__
#define __OKAY_ASSET_H__

#include <filesystem>
#include <fstream>
#include <functional>
#include <istream>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/util/result.hpp>

#ifndef OKAY_ENGINE_ASSET_ROOT
#define OKAY_ENGINE_ASSET_ROOT ".okay/engine/assets"
#endif

#ifndef OKAY_GAME_ASSET_ROOT
#define OKAY_GAME_ASSET_ROOT ".okay/game/assets"
#endif

namespace okay {

template <typename T>
struct OkayAsset {
    T asset;
    // meta information about the asset
    std::size_t assetSize;
};

class OkayAssetIO {
   public:
    virtual ~OkayAssetIO() = default;

    virtual Result<std::unique_ptr<std::istream>> open(const std::filesystem::path& path) = 0;
    virtual Result<std::size_t> fileSize(const std::filesystem::path& path) = 0;
};

class FilesystemAssetIO final : public OkayAssetIO {
   public:
    Result<std::unique_ptr<std::istream>> open(const std::filesystem::path& path) override {
        auto f = std::make_unique<std::ifstream>(path, std::ios::in | std::ios::binary);
        if (!f->is_open()) {
            return Result<std::unique_ptr<std::istream>>::errorResult("Failed to open asset: " +
                                                                      path.string());
        }
        return Result<std::unique_ptr<std::istream>>::ok(std::move(f));
    }

    Result<std::size_t> fileSize(const std::filesystem::path& path) override {
        std::error_code ec;
        auto s = std::filesystem::file_size(path, ec);
        if (ec) {
            return Result<std::size_t>::errorResult("file_size failed: " + path.string());
        }
        return Result<std::size_t>::ok(static_cast<std::size_t>(s));
    }
};

template <typename T>
struct OkayAssetLoader {
    static Result<T> loadAsset(const std::filesystem::path& path, OkayAssetIO &assetIO) {
        static_assert(sizeof(T) != 0,
                      "No OkayAssetLoader<T> specialization found for this asset type.");
        return Result<T>::errorResult("No loader");
    }
};

template <typename T>
using OnAssetSucceedCB = std::function<void(const OkayAsset<T>&)>;

template <typename T>
using OnAssetFailedCB = std::function<void(const std::string&)>;

class OkayAssetManager : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    using DefaultAssetIO = FilesystemAssetIO;

    template <typename T, typename AssetIO = DefaultAssetIO>
    struct Load {
       public:
        OnAssetSucceedCB<T> onCompleteCB;
        OnAssetFailedCB<T> onFailCB;
        std::filesystem::path assetPath;

        AssetIO assetIO;

        static Load<T> EngineAsset(const std::filesystem::path& path) {
            return Load(std::filesystem::path(OKAY_ENGINE_ASSET_ROOT) / path);
        };

        static Load<T> GameAsset(const std::filesystem::path& path) {
            return Load(std::filesystem::path(OKAY_GAME_ASSET_ROOT) / path);
        };

        Load<T>& onComplete(OnAssetSucceedCB<T> cb) {
            onCompleteCB = cb;
            return *this;
        }
        Load<T>& onFail(OnAssetFailedCB<T> cb) {
            onFailCB = cb;
            return *this;
        }

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

    template <typename T, typename AssetIO = DefaultAssetIO>
    LoadHandle<T> loadAssetAsync(const Load<T, AssetIO>& load) {
        Result<T> res = OkayAssetLoader<T>::loadAsset(load.assetPath, load.assetIO);

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

    template <typename T, typename AssetIO = DefaultAssetIO>
    Result<OkayAsset<T>> loadAssetSync(const Load<T, AssetIO>& load) {
        Result<T> res = OkayAssetLoader<T>::loadAsset(load.assetPath, load.assetIO);

        if (res.isError()) {
            return Result<OkayAsset<T>>::errorResult(res.error());
        } else {
            return Result<OkayAsset<T>>::ok(createAsset(load.assetPath, res.value()));
        };
    };

    template <typename T, typename AssetIO = DefaultAssetIO>
    Result<OkayAsset<T>> loadEngineAssetSync(const std::filesystem::path& path) {
        return loadAssetSync(Load<T, AssetIO>::EngineAsset(path));
    }

    template <typename T, typename AssetIO = DefaultAssetIO>
    Result<OkayAsset<T>> loadGameAssetSync(const std::filesystem::path& path) {
        return loadAssetSync(Load<T, AssetIO>::GameAsset(path));
    }

   private:
    template <typename T>
    inline OkayAsset<T> createAsset(const std::filesystem::path& path, T loaded, const OkayAssetIO& assetIO) {
        return OkayAsset<T>{.asset = loaded, .assetSize = assetIO.fileSize(path).value()};
    }
};

}  // namespace okay
#endif  // __OKAY_ASSET_H__