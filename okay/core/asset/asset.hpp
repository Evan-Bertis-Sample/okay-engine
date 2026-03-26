#ifndef _ASSET_H__
#define _ASSET_H__

#include <okay/core/engine/system.hpp>
#include <okay/core/util/option.hpp>
#include <okay/core/util/result.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <istream>

#ifdef _WIN32
#define SEP "\\"
#else
#define SEP "/"
#endif

#ifndef OKAY_ENGINE_ASSET_ROOT
#define OKAY_ENGINE_ASSET_ROOT "engine" SEP "assets"
#endif

#ifndef OKAY_GAME_ASSET_ROOT
#define OKAY_GAME_ASSET_ROOT "game" SEP "assets"
#endif

namespace okay {

template <typename T>
struct Asset {
    T asset;
    // meta information about the asset
    std::size_t assetSize;
};

class AssetIO {
   public:
    virtual ~AssetIO() = default;

    virtual Result<std::unique_ptr<std::istream>> open(const std::filesystem::path& path) const = 0;
    virtual Result<std::size_t> fileSize(const std::filesystem::path& path) const = 0;
};

class FilesystemAssetIO final : public AssetIO {
   public:
    Result<std::unique_ptr<std::istream>> open(const std::filesystem::path& path) const override {
        auto f = std::make_unique<std::ifstream>(path, std::ios::in | std::ios::binary);
        if (!f->is_open()) {
            return Result<std::unique_ptr<std::istream>>::errorResult("Failed to open asset: " +
                                                                      path.string());
        }
        return Result<std::unique_ptr<std::istream>>::ok(std::move(f));
    }

    Result<std::size_t> fileSize(const std::filesystem::path& path) const override {
        std::error_code ec;
        auto s = std::filesystem::file_size(path, ec);
        if (ec) {
            return Result<std::size_t>::errorResult("file_size failed: " + path.string());
        }
        return Result<std::size_t>::ok(static_cast<std::size_t>(s));
    }
};

template <typename T, typename LoadOptions = void>
struct AssetLoader {
    static Result<T> loadAsset(const std::filesystem::path& path,
                               const AssetIO& assetIO,
                               LoadOptions options) {
        static_assert(sizeof(T) != 0,
                      "No OkayAssetLoader<T> specialization found for this asset type.");
        return Result<T>::errorResult("No loader");
    }
};

template <typename T>
using OnAssetSucceedCB = std::function<void(const Asset<T>&)>;

template <typename T>
using OnAssetFailedCB = std::function<void(const std::string&)>;

class AssetManager : public System<SystemScope::ENGINE> {
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
        Result<Asset<T>> asset;
    };

    template <typename T, typename AssetIO = DefaultAssetIO, typename... LoadOptions>
    LoadHandle<T> loadAssetAsync(const Load<T, AssetIO>& load, LoadOptions... options) {
        Result<T> res =
            AssetLoader<T, LoadOptions...>::loadAsset(load.assetPath, load.assetIO, options...);

        if (res.isError()) {
            auto handleAsset = Result<Asset<T>>::errorResult(res.error());
            if (load.onFailCB)
                load.onFailCB(res.error());

            return LoadHandle<T>{.state = LoadHandle<T>::State::FAILED, .asset = handleAsset};
        } else {
            Asset<T> asset = createAsset<T>(load.assetPath, res.value(), load.assetIO);
            auto handleAsset = Result<Asset<T>>::ok(asset);
            if (load.onCompleteCB)
                load.onCompleteCB(asset);

            return LoadHandle<T>{.state = LoadHandle<T>::State::COMPLETE, .asset = handleAsset};
        };
    };

    template <typename T, typename AssetIO = DefaultAssetIO, typename... LoadOptions>
    Result<Asset<T>> loadAssetSync(const Load<T, AssetIO>& load, LoadOptions... options) {
        Result<T> res =
            AssetLoader<T, LoadOptions...>::loadAsset(load.assetPath, load.assetIO, options...);

        if (res.isError()) {
            return Result<Asset<T>>::errorResult(res.error());
        } else {
            return Result<Asset<T>>::ok(
                createAsset<T>(load.assetPath, res.value(), load.assetIO));
        };
    };

    template <typename T, typename AssetIO = DefaultAssetIO, typename... LoadOptions>
    Result<Asset<T>> loadEngineAssetSync(const std::filesystem::path& path,
                                             LoadOptions... options) {
        return loadAssetSync(Load<T, AssetIO>::EngineAsset(path), options...);
    }

    template <typename T, typename AssetIO = DefaultAssetIO, typename... LoadOptions>
    Result<Asset<T>> loadGameAssetSync(const std::filesystem::path& path,
                                           LoadOptions... options) {
        return loadAssetSync(Load<T, AssetIO>::GameAsset(path), options...);
    }

   private:
    template <typename T>
    inline Asset<T> createAsset(const std::filesystem::path& path,
                                    T loaded,
                                    const AssetIO& assetIO) {
        return Asset<T>{.asset = loaded, .assetSize = assetIO.fileSize(path).value()};
    }
};

}  // namespace okay
#endif  // _ASSET_H__