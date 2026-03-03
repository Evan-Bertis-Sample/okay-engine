#ifndef __OKAY_TEXTURE_H__
#define __OKAY_TEXTURE_H__

#include <glad/glad.h>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_gl.hpp>
#include <okay/core/util/result.hpp>

#include <span>
#include <vector>
#include <unordered_map>

namespace okay {

struct OkayTextureMeta {
    enum class Format : std::uint8_t { RGBA8, RGB8, RGBA16F, RGB16F, DEPTH24_STENCIL8 };

    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t channels;
    std::uint8_t mipLevels;
    Format format;
};

class OkayTextureDataStore {
   public:
    // TODO: make texture handles have reference counting so that we can automatically release
    // texture data when it's no longer used
    struct TextureHandle {
        size_t start;
        size_t size;

        static bool isNone(TextureHandle handle) {
            return handle.start == 0 && handle.size == 0;
        }
        static TextureHandle none() {
            return TextureHandle{0, 0};
        }

        bool operator==(TextureHandle other) const {
            return start == other.start && size == other.size;
        }
        bool operator!=(TextureHandle other) const {
            return !(*this == other);
        }
    };

    TextureHandle addTexture(OkayTextureMeta meta, void* data, size_t dataSize) {
        return addTexture(
            meta, std::span<const std::byte>(static_cast<const std::byte*>(data), dataSize));
    }

    TextureHandle addTexture(OkayTextureMeta meta, std::span<const std::byte> data) {
        TextureHandle handle = storeTextureData(data);
        _metaMap[handle] = meta;
        return handle;
    }

    void releaseTexture(TextureHandle handle) {
        releaseTextureData(handle);
        _metaMap.erase(handle);
    }

    OkayTextureMeta getTextureMeta(TextureHandle handle) const {
        return _metaMap.at(handle);
    }

    std::span<const std::byte> getTextureData(TextureHandle handle) const {
        return std::span<const std::byte>(_blob.data() + handle.start, handle.size);
    }

   private:
    struct BlockMeta {
        size_t start;
        size_t size;
    };

    std::vector<std::byte> _blob;
    std::vector<BlockMeta> _openBlocks;  // start, size

    std::unordered_map<TextureHandle, OkayTextureMeta> _metaMap;

    TextureHandle storeTextureData(const std::span<const std::byte> data) {
        // see if there is a free block that can fit the data
        for (size_t i = 0; i < _openBlocks.size(); ++i) {
            size_t blockStart = _openBlocks[i].start;
            size_t blockSize = _openBlocks[i].size;

            if (blockSize <= data.size()) {
                continue;  // we can't fit the data in this block
            }

            // we can fit the data into the block, so we use it
            TextureHandle handle;
            handle.start = blockStart;
            handle.size = data.size();
            _blob.insert(_blob.begin() + blockStart, data.begin(), data.end());

            // now update open blocks with the remaining free space, if any
            size_t remainingSize = blockSize - data.size();
            if (remainingSize > 0) {
                _openBlocks[i].start = handle.start + handle.size;
                _openBlocks[i].size = remainingSize;
            } else {
                _openBlocks.erase(_openBlocks.begin() + i);
            }

            return handle;
        }

        // there are no open blocks that can fit the data, so we append it to the end of the blob
        TextureHandle handle;
        handle.start = _blob.size();
        handle.size = data.size();
        _blob.insert(_blob.end(), data.begin(), data.end());
        return handle;
    }

    void releaseTextureData(const TextureHandle& handle) {
        _openBlocks.push_back({handle.start, handle.size});
    }
};

class OkayTexture {
   public:
    struct TextureParameters {
        GLenum minFilter{GL_LINEAR};
        GLenum magFilter{GL_LINEAR};
        GLenum wrapS{GL_REPEAT};
        GLenum wrapT{GL_REPEAT};
    };

    OkayTextureDataStore& store;
    OkayTextureDataStore::TextureHandle handle;

    OkayTexture(OkayTextureDataStore& store, OkayTextureDataStore::TextureHandle handle)
        : store(store), handle(handle) {
    }

    std::span<const std::byte> getData() const {
        return store.getTextureData(handle);
    }

    OkayTextureMeta getMeta() const {
        return store.getTextureMeta(handle);
    }

    Failable uploadToGPU(const TextureParameters& params = {}) {
        const auto meta = getMeta();
        const auto data = getData();

        if (meta.format == OkayTextureMeta::Format::DEPTH24_STENCIL8) {
            GL_CHECK_FAILABLE(glGenTextures(1, &_glTextureID));
            GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, _glTextureID));
            GL_CHECK_FAILABLE(glTexImage2D(GL_TEXTURE_2D,
                                          0,
                                          GL_DEPTH24_STENCIL8,
                                          meta.width,
                                          meta.height,
                                          0,
                                          GL_DEPTH_STENCIL,
                                          GL_UNSIGNED_INT_24_8,
                                          nullptr));
        } else {
            GLenum glFormat;
            GLenum glInternalFormat;
            switch (meta.format) {
                case OkayTextureMeta::Format::RGBA8:
                    glFormat = GL_RGBA;
                    glInternalFormat = GL_RGBA8;
                    break;
                case OkayTextureMeta::Format::RGB8:
                    glFormat = GL_RGB;
                    glInternalFormat = GL_RGB8;
                    break;
                case OkayTextureMeta::Format::RGBA16F:
                    glFormat = GL_RGBA;
                    glInternalFormat = GL_RGBA16F;
                    break;
                case OkayTextureMeta::Format::RGB16F:
                    glFormat = GL_RGB;
                    glInternalFormat = GL_RGB16F;
                    break;
                default:
                    return Failable::errorResult("Unsupported texture format");
            }

            GL_CHECK_FAILABLE(glGenTextures(1, &_glTextureID));
            GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, _glTextureID));
            GL_CHECK_FAILABLE(glTexImage2D(GL_TEXTURE_2D,
                                          0,
                                          glInternalFormat,
                                          meta.width,
                                          meta.height,
                                          0,
                                          glFormat,
                                          GL_UNSIGNED_BYTE,
                                          data.data()));
        }

        // set parameters
        GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.minFilter));
        GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.magFilter));
        GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrapS));
        GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrapT));

        return Failable::ok({});
    }

    Failable bind() const {
        if (_glTextureID == 0) {
            return Failable::errorResult("Texture not uploaded to GPU");
        }

        glBindTexture(GL_TEXTURE_2D, _glTextureID);
        return Failable::ok({});
    }

   private:
    GLuint _glTextureID{0};
};

}  // namespace okay

#endif  // __OKAY_TEXTURE_H__