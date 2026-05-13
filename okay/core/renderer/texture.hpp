#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <okay/core/renderer/gl.hpp>
#include <okay/core/util/result.hpp>

#include <span>
#include <vector>

namespace okay {

struct OkayTextureMeta {
    enum class Format : std::uint8_t { RGBA8, RGB8, RGBA16F, RGB16F, DEPTH24_STENCIL8 };

    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t channels;
    std::uint8_t mipLevels;
    Format format;
};

class TextureDataStore {
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

        // comparison operator for using TextureHandle as a key in std::map
        bool operator<(const TextureHandle& other) const {
            return start < other.start || (start == other.start && size < other.size);
        }
    };

    static std::shared_ptr<TextureDataStore> mainStore() {
        static std::shared_ptr<TextureDataStore> store = std::make_shared<TextureDataStore>();
        return store;
    }

    TextureHandle addTexture(OkayTextureMeta meta, void* data, size_t dataSize) {
        return addTexture(
            meta, std::span<const std::byte>(static_cast<const std::byte*>(data), dataSize));
    }

    TextureHandle addTexture(OkayTextureMeta meta, std::span<const std::byte> data) {
        TextureHandle handle = storeTextureData(data);
        _metaMap[handle] = meta;
        return handle;
    }

    TextureHandle addTexture(std::size_t size) {
        TextureHandle handle = getBlock(size);
        _metaMap[handle] = OkayTextureMeta{};
        return handle;
    }

    void releaseTexture(TextureHandle handle) {
        releaseTextureData(handle);
        _metaMap.erase(handle);
    }

    OkayTextureMeta getTextureMeta(TextureHandle handle) const {
        if (TextureHandle::isNone(handle)) {
            // Engine.logger.error("Unable to get texture Meta of invalid handle!");
            return OkayTextureMeta{};
        }

        return _metaMap.at(handle);
    }

    std::span<const std::byte> getTextureData(TextureHandle handle) const {
        return std::span<const std::byte>(_blob.begin() + handle.start, handle.size);
    }

    std::byte* getTextureDataStart(TextureHandle handle) {
        return _blob.data() + handle.start;
    }

   private:
    struct BlockMeta {
        size_t start;
        size_t size;
    };

    std::vector<std::byte> _blob;
    std::vector<BlockMeta> _openBlocks;  // start, size

    std::map<TextureHandle, OkayTextureMeta> _metaMap;

    TextureHandle storeTextureData(const std::span<const std::byte> data) {
        // there are no open blocks that can fit the data, so we append it to the end of the blob
        TextureHandle handle = getBlock(data.size());
        std::copy(data.begin(), data.end(), _blob.begin() + handle.start);
        return handle;
    }

    TextureHandle getBlock(size_t size) {
        // see if there is a free block that can fit the data
        for (size_t i = 0; i < _openBlocks.size(); ++i) {
            size_t blockStart = _openBlocks[i].start;
            size_t blockSize = _openBlocks[i].size;

            if (blockSize < size) {
                continue;  // we can't fit the data in this block
            }

            // we can fit the data into the block, so we use it
            TextureHandle handle;
            handle.start = blockStart;
            handle.size = size;

            Engine.logger.info(
                "Reusing texture block: start={}, size={}", handle.start, handle.size);

            // now update open blocks with the remaining free space, if any
            size_t remainingSize = blockSize - size;
            if (remainingSize > 0) {
                _openBlocks[i].start = handle.start + handle.size;
                _openBlocks[i].size = remainingSize;
                Engine.logger.info("Updated open block: start={}, size={}",
                    _openBlocks[i].start,
                    _openBlocks[i].size);
            } else {
                Engine.logger.info("Removing open block: start={}, size={}",
                    _openBlocks[i].start,
                    _openBlocks[i].size);

                _openBlocks.erase(_openBlocks.begin() + i);
            }

            return handle;
        }

        // there are no open blocks that can fit the data, so we append it to the end of the blob
        size_t end = _blob.size();
        if (end + size > _blob.capacity()) {
            // we need to grow the blob to fit the new data
            _blob.reserve(std::max(_blob.capacity() * 2, end + size));
        }
        Engine.logger.info("Creating new texture block: start={}, size={}", end, size);
        TextureHandle handle;
        handle.start = end;
        handle.size = size;
        _blob.resize(end + size);
        return handle;
    }

    void releaseTextureData(const TextureHandle& handle) {
        _openBlocks.push_back({handle.start, handle.size});
    }
};

class Texture {
   public:
    struct TextureParameters {
        GLenum minFilter{GL_LINEAR};
        GLenum magFilter{GL_LINEAR};
        GLenum wrapS{GL_REPEAT};
        GLenum wrapT{GL_REPEAT};
    };

    std::shared_ptr<TextureDataStore> store;
    TextureDataStore::TextureHandle handle;

    Texture() = default;

    Texture(std::shared_ptr<TextureDataStore> store, TextureDataStore::TextureHandle handle)
        : store(store), handle(handle) {}

    std::span<const std::byte> getData() const {
        return store->getTextureData(handle);
    }

    OkayTextureMeta getMeta() const {
        return store->getTextureMeta(handle);
    }

    bool operator==(const Texture& other) const {
        return handle == other.handle;
    }
    bool operator!=(const Texture& other) const {
        return !(*this == other);
    }
    bool operator<(const Texture& other) const {
        return handle < other.handle;
    }
};

}  // namespace okay

#endif  // _TEXTURE_H__
