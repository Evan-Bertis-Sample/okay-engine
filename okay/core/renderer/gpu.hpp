#ifndef __OKAY_GPU_H__
#define __OKAY_GPU_H__

#include <glad/glad.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <okay/core/okay.hpp>
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/util/result.hpp>

namespace okay {

class OkayUniformBlockManager {
   public:
    OkayUniformBlockManager() = default;
    ~OkayUniformBlockManager() { destroyAll(); }

    OkayUniformBlockManager(const OkayUniformBlockManager&) = delete;
    OkayUniformBlockManager& operator=(const OkayUniformBlockManager&) = delete;

    void destroyAll() {
        for (auto& [k, u] : _ubos) {
            if (u.id) {
                glDeleteBuffers(1, &u.id);
                u.id = 0;
            }
        }
        _ubos.clear();
    }

    template <class BlockProp>
    Failable pass(GLuint program, BlockProp& prop) {
        using TBlock = typename BlockProp::ValueType;

        GpuUbo& u = _ubos[propKey(prop)];

        // Create buffer lazily
        if (u.id == 0) {
            GL_CHECK_FAILABLE(glGenBuffers(1, &u.id));
            GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, u.id));
            GL_CHECK_FAILABLE(glBufferData(
                GL_UNIFORM_BUFFER, (GLsizeiptr)sizeof(TBlock), nullptr, GL_DYNAMIC_DRAW));
            GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, 0));
        }

        // Allocate binding point lazily
        if (u.bindingPoint == invalidBinding()) {
            GLuint hint = bindingHint(prop);
            if (hint != invalidBinding()) {
                u.bindingPoint = hint;
            } else {
                GLint maxBindings = 0;
                glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);

                GLuint next = s_nextBindingPoint.fetch_add(1, std::memory_order_relaxed);
                if ((GLint)next >= maxBindings) {
                    return Failable::errorResult(
                        "Out of UBO binding points. GL_MAX_UNIFORM_BUFFER_BINDINGS=" +
                        std::to_string(maxBindings));
                }
                u.bindingPoint = next;
            }
        }

        // Bind program's block index -> binding point once per program
        if (!wasBoundToProgram(u, program)) {
            GLuint idx = glGetUniformBlockIndex(program, std::string(BlockProp::name.sv()).c_str());
            if (idx != GL_INVALID_INDEX) {
                glUniformBlockBinding(program, idx, u.bindingPoint);
            }
            markBoundToProgram(u, program);
        }

        // Bind buffer to binding point every pass
        GL_CHECK_FAILABLE(glBindBufferBase(GL_UNIFORM_BUFFER, u.bindingPoint, u.id));

        // Upload only if version changed
        const std::uint32_t v = versionOf(prop);
        if (u.lastUploadedVersion != v) {
            GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, u.id));
            GL_CHECK_FAILABLE(
                glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)sizeof(TBlock), &prop.get()));
            GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, 0));
            u.lastUploadedVersion = v;
        }

        return Failable::ok({});
    }

   private:
    static constexpr GLuint invalidBinding() { return 0xFFFFFFFFu; }

    struct GpuUbo {
        GLuint id = 0;
        GLuint bindingPoint = invalidBinding();
        std::uint32_t lastUploadedVersion = 0;

        // program -> bound?
        std::unordered_map<GLuint, bool> boundPrograms;
    };

    std::unordered_map<const void*, GpuUbo> _ubos;
    inline static std::atomic<GLuint> s_nextBindingPoint{0};

    template <class BlockProp>
    static const void* propKey(const BlockProp& p) {
        if constexpr (requires { p.identity(); }) {
            return p.identity();
        } else {
            return &p;
        }
    }

    static bool wasBoundToProgram(const GpuUbo& u, GLuint program) {
        auto it = u.boundPrograms.find(program);
        return it != u.boundPrograms.end();
    }

    static void markBoundToProgram(GpuUbo& u, GLuint program) { u.boundPrograms[program] = true; }

    template <class BlockProp>
    static std::uint32_t versionOf(const BlockProp& p) {
        if constexpr (requires { p.version(); }) {
            return (std::uint32_t)p.version();
        } else {
            // If you don't expose a version, we conservatively upload every time.
            // (Strongly recommend adding version() to your BlockProperty.)
            return 0xFFFFFFFFu;
        }
    }

    template <class BlockProp>
    static GLuint bindingHint(const BlockProp& p) {
        if constexpr (requires { p.bindingPointHint(); }) {
            return (GLuint)p.bindingPointHint();
        } else {
            return invalidBinding();
        }
    }
};

class OkayTextureManager {
   public:
    OkayTextureManager() = default;
    ~OkayTextureManager() { destroyAll(); }

    OkayTextureManager(const OkayTextureManager&) = delete;
    OkayTextureManager& operator=(const OkayTextureManager&) = delete;

    void destroyAll() {
        for (auto& [k, gt] : _textures) {
            if (gt.id) {
                glDeleteTextures(1, &gt.id);
                gt.id = 0;
            }
        }
        _textures.clear();
    }

    // Ensure a GL texture exists and is uploaded; returns GL id.
    Failable ensureUploaded2D(const OkayTexture& tex,
                              const OkayTexture::TextureParameters& params,
                              GLuint& outTextureId) {
        if (!tex.store || OkayTextureDataStore::TextureHandle::isNone(tex.handle)) {
            return Failable::ok();
        }

        TextureKey key{tex.store.get(), tex.handle};
        GpuTexture2D& gt = _textures[key];

        if (gt.id == 0) {
            // Create + upload once
            auto meta = tex.getMeta();
            auto data = tex.getData();

            GL_CHECK_FAILABLE(glGenTextures(1, &gt.id));
            GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, gt.id));
            Engine.logger.info(
                "Uploading texture to GPU: textureId={}, width={}, height={}, format={}",
                gt.id,
                meta.width,
                meta.height,
                static_cast<int>(meta.format));

            if (meta.format == OkayTextureMeta::Format::DEPTH24_STENCIL8) {
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
                GLenum glFormat = GL_RGBA;
                GLenum glInternal = GL_RGBA8;

                switch (meta.format) {
                    case OkayTextureMeta::Format::RGBA8:
                        glFormat = GL_RGBA;
                        glInternal = GL_RGBA8;
                        break;
                    case OkayTextureMeta::Format::RGB8:
                        glFormat = GL_RGB;
                        glInternal = GL_RGB8;
                        break;
                    case OkayTextureMeta::Format::RGBA16F:
                        glFormat = GL_RGBA;
                        glInternal = GL_RGBA16F;
                        break;
                    case OkayTextureMeta::Format::RGB16F:
                        glFormat = GL_RGB;
                        glInternal = GL_RGB16F;
                        break;
                    default:
                        return Failable::errorResult("Unsupported texture format");
                }

                // NOTE: internalFormat must be glInternal (not glFormat)
                GL_CHECK_FAILABLE(glTexImage2D(GL_TEXTURE_2D,
                                               0,
                                               glInternal,
                                               meta.width,
                                               meta.height,
                                               0,
                                               glFormat,
                                               GL_UNSIGNED_BYTE,
                                               data.data()));
            }

            gt.meta = meta;
            gt.paramsInitialized = false;
        }

        // Apply params if first time or changed
        if (!gt.paramsInitialized || !sameParams(gt.params, params)) {
            Engine.logger.info(
                "Setting texture parameters: minFilter={}, magFilter={}, wrapS={}, wrapT={}",
                params.minFilter,
                params.magFilter,
                params.wrapS,
                params.wrapT);
            GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, gt.id));
            GL_CHECK_FAILABLE(
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.minFilter));
            GL_CHECK_FAILABLE(
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.magFilter));
            GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrapS));
            GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrapT));
            gt.params = params;
            gt.paramsInitialized = true;
        }

        outTextureId = gt.id;
        return Failable::ok({});
    }

    // Bind a 2D texture to a unit and set sampler uniform = unit.
    Failable bindSampler2D(GLuint program,
                           GLuint samplerLoc,
                           const OkayTexture& tex,
                           const OkayTexture::TextureParameters& params,
                           GLuint unit) {
        GLuint id = 0;
        auto r = ensureUploaded2D(tex, params, id);
        if (r.isError())
            return r;

        GL_CHECK_FAILABLE(glUseProgram(program));
        GL_CHECK_FAILABLE(glActiveTexture(GL_TEXTURE0 + unit));
        GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, id));
        GL_CHECK_FAILABLE(glUniform1i((GLint)samplerLoc, (GLint)unit));
        return Failable::ok({});
    }

   private:
    struct TextureKey {
        const OkayTextureDataStore* storePtr{};
        OkayTextureDataStore::TextureHandle handle{};

        bool operator==(const TextureKey& o) const {
            return storePtr == o.storePtr && handle.start == o.handle.start &&
                   handle.size == o.handle.size;
        }
    };

    struct TextureKeyHash {
        size_t operator()(const TextureKey& k) const noexcept {
            size_t h = std::hash<const void*>{}(k.storePtr);
            h ^= std::hash<size_t>{}(k.handle.start) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            h ^= std::hash<size_t>{}(k.handle.size) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
            return h;
        }
    };

    struct GpuTexture2D {
        GLuint id = 0;
        OkayTextureMeta meta{};
        OkayTexture::TextureParameters params{};
        bool paramsInitialized = false;
    };

    static bool sameParams(const OkayTexture::TextureParameters& a,
                           const OkayTexture::TextureParameters& b) {
        return a.minFilter == b.minFilter && a.magFilter == b.magFilter && a.wrapS == b.wrapS &&
               a.wrapT == b.wrapT;
    }

    std::unordered_map<TextureKey, GpuTexture2D, TextureKeyHash> _textures;
};

class OkayGpuState {
   public:
    OkayUniformBlockManager blocks;
    OkayTextureManager textures;

    static OkayGpuState& instance() {
        static OkayGpuState s_instance;
        return s_instance;
    }

   private:
    OkayGpuState() = default;
};

}  // namespace okay

#endif  // __OKAY_GPU_H__