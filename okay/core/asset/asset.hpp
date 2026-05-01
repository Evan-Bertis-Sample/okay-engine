#ifndef __ASSET_H__
#define __ASSET_H__

#include <okay/core/engine/engine.hpp>
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
            return Result<std::unique_ptr<std::istream>>::errorResult(
                "Failed to open asset: " + path.string());
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

template <typename T, typename LoadOptions = std::tuple<>>
struct AssetLoader {
    static Result<T> loadAsset(
        const std::filesystem::path& path, const AssetIO& assetIO, LoadOptions options) {
        static_assert(
            sizeof(T) != 0, "No OkayAssetLoader<T> specialization found for this asset type.");
        return Result<T>::errorResult("No loader");
    }
};

template <typename T, typename LoadOptions = std::tuple<>>
class CachedAssetStore {
   public:
    static CachedAssetStore& instance() {
        static CachedAssetStore instance;
        return instance;
    }

    void cacheAsset(const std::filesystem::path& path, Asset<T> asset, const LoadOptions& options) {
        LoadKey key = {
            .path = path,
            .options = options,
        };

        _cache[key] = AssetMetadata{
            .asset = asset,
            .referenceCount = 0,
        };
    }

    Option<Asset<T>> getCachedAsset(const std::filesystem::path& path, const LoadOptions& options) {
        LoadKey key = {
            .path = path,
            .options = options,
        };

        if (_cache.contains(key)) {
            return Option<Asset<T>>::some(_cache[key].asset);
        }

        return Option<Asset<T>>::none();
    }

   private:
    struct LoadKey {
        std::filesystem::path path;
        LoadOptions options;

        bool operator==(const LoadKey& other) const {
            return path == other.path && options == other.options;
        }

        bool operator<(const LoadKey& other) const {
            return path < other.path && options < other.options;
        }
    };

    struct AssetMetadata {
        Asset<T> asset;
        std::size_t referenceCount;
    };

    std::map<LoadKey, AssetMetadata> _cache;
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

        static Load<T> engineAsset(const std::filesystem::path& path) {
            return Load(std::filesystem::path(OKAY_ENGINE_ASSET_ROOT) / path);
        };

        static Load<T> gameAsset(const std::filesystem::path& path) {
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
        enum class State { LOADING, COMPLETE, FAILED };
        State state;
        Result<Asset<T>> asset;
    };

    template <typename T, typename AssetIO = DefaultAssetIO, typename LoadOptions>
    LoadHandle<T> loadAssetAsync(const Load<T, AssetIO>& load, const LoadOptions& options) {
        Engine.logger.error(
            "AssetManager::loadAssetAsync: Async loading not implemented! Please use a sync load for now.");
        return LoadHandle<T>{
            .asset = T{},
            .state = LoadHandle<T>::State::FAILED,
        };
    }

    template <typename T, typename AssetIO = DefaultAssetIO, typename LoadOptions>
    Result<Asset<T>> loadAssetSync(const Load<T, AssetIO>& load, const LoadOptions& options) {
        auto& store = CachedAssetStore<T, LoadOptions>::instance();
        Option<Asset<T>> assetOpt = store.getCachedAsset(load.assetPath, options);
        if (assetOpt.isSome()) {
            return Result<Asset<T>>::ok(assetOpt.value());
        }

        Result<T> res =
            AssetLoader<T, LoadOptions>::loadAsset(load.assetPath, load.assetIO, options);

        if (res.isError()) {
            return Result<Asset<T>>::errorResult(res.error());
        } else {
            return Result<Asset<T>>::ok(
                createAsset<T>(load.assetPath, res.value(), load.assetIO, options));
        };
    };

    template <typename T, typename AssetIO = DefaultAssetIO, typename LoadOptions = std::tuple<>>
    Result<Asset<T>> loadEngineAssetSync(
        const std::filesystem::path& path, const LoadOptions& options = LoadOptions{}) {
        return loadAssetSync(Load<T, AssetIO>::engineAsset(path), options);
    }

    template <typename T, typename AssetIO = DefaultAssetIO, typename LoadOptions = std::tuple<>>
    Result<Asset<T>> loadGameAssetSync(
        const std::filesystem::path& path, const LoadOptions& options = LoadOptions{}) {
        return loadAssetSync(Load<T, AssetIO>::gameAsset(path), options);
    }

   private:
    template <typename T, typename LoadOptions>
    inline Asset<T> createAsset(const std::filesystem::path& path,
        T loaded,
        const AssetIO& assetIO,
        const LoadOptions& options) {
        auto asset = Asset<T>{.asset = loaded, .assetSize = assetIO.fileSize(path).value()};
        // cache this guy
        auto& store = CachedAssetStore<T, LoadOptions>::instance();
        store.cacheAsset(path, asset, options);
        return asset;
    }
};

}  // namespace okay
#endif  // _ASSET_H__
