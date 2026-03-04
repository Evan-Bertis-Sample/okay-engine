#ifndef __TEXTURE_LOADER_H__
#define __TEXTURE_LOADER_H__


#include <cstddef>
#include <filesystem>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/util/result.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <string>

namespace okay {

struct OkayTextureLoadSettings {
    std::shared_ptr<OkayTextureDataStore> store;
};

template <>
struct OkayAssetLoader<OkayTexture, OkayTextureLoadSettings> {
    static Result<OkayTexture> loadAsset(const std::filesystem::path& path,
                                        const OkayAssetIO& assetIO,
                                        const OkayTextureLoadSettings& settings) {
        Engine.logger.info("Loading texture: {}", path.string());

        std::shared_ptr<OkayTextureDataStore> store = settings.store;
        
        // get the size of the image
        int width, height, channels;
        if (stbi_info(path.string().c_str(), &width, &height, &channels) == 0) {
            Engine.logger.error("Failed to get image info: {}", path.string());
            return Result<OkayTexture>::errorResult("Failed to get image info: " + path.string());
        }

        // now load the image into the texture data store
        stbi_uc *result = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
        if (result == nullptr) {
            Engine.logger.error("Failed to load image: {}", path.string());
            return Result<OkayTexture>::errorResult("Failed to load image: " + path.string());
        }

        // copy the image data into the texture data store
        // TODO: maybe make an allocator function so that we don't 
        // have to copy the data
        OkayTextureMeta meta;
        meta.width = width;
        meta.height = height;
        meta.format = OkayTextureMeta::Format::RGB8;
        std::size_t size = static_cast<size_t>(width * height * channels);
        Engine.logger.debug("Image size: {}", size);
        std::span<const std::byte> data = std::span<const std::byte>(reinterpret_cast<const std::byte*>(result), size);
        OkayTextureDataStore::TextureHandle handle = store->addTexture(meta, data);
        return Result<OkayTexture>::ok(OkayTexture(store, handle));
    }
};

}  // namespace okay

#endif // __TEXTURE_LOADER_H__