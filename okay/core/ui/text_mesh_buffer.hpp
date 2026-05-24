#ifndef __TEXT_MESH_BUFFER_H__
#define __TEXT_MESH_BUFFER_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/ui/font.hpp>
#include <okay/core/ui/text_layout.hpp>
#include <okay/core/ui/text_mesh_builder.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/result.hpp>

namespace okay {

class TextMeshBuffer {
   public:
    using TextMeshHandle = ObjectPoolHandle;

    TextMeshBuffer(SystemParameter<Renderer> renderer = nullptr) : _renderer(renderer) {}

    TextMeshHandle allocateHandle(std::size_t maxFontLength = 25) {
        Mesh mesh = _renderer->meshBuffer().reserveMesh(maxFontLength * 4,  // 4 vertices per quad
            maxFontLength * 6                                               // 6 indices per quad
        );

        TextMeshMeta meta = {.mesh = mesh, .maxQuads = maxFontLength};

        return _textMeshPool.emplace(meta);
    };

    void freeHandle(TextMeshHandle handle) {
        if (!_textMeshPool.valid(handle)) {
            Engine.logger.warn("Tried to free an invalid handle with id {}", handle.index);
        }
        TextMeshMeta meta = _textMeshPool.get(handle);
        _renderer->meshBuffer().removeMesh(meta.mesh);
        _textMeshPool.destroy(handle);
    }

    Mesh getMesh(TextMeshHandle handle) {
        if (!_textMeshPool.valid(handle)) {
            Engine.logger.warn(
                "Tried to get a mesh with an invalid handle with id {}", handle.index);
            return Mesh::none();
        }
        return _textMeshPool.get(handle).mesh;
    }

    void updateMesh(TextMeshHandle handle, const TextStyle& style, const std::string& text) {
        if (!_textMeshPool.valid(handle)) {
            Engine.logger.warn(
                "Tried to update a mesh with an invalid handle with id {}", handle.index);
            return;
        }

        const std::size_t quadsUsed = text.size();
        TextMeshMeta& meta = _textMeshPool.get(handle);

        if (quadsUsed <= meta.maxQuads) {
            // this is easy, we can simply reuse the mesh
            MeshData textMeshData = TextMeshBuilder::build(text, style, false);
            Result<Mesh> updated = _renderer->meshBuffer().updateMesh(meta.mesh, textMeshData);

            if (updated.isError()) {
                Engine.logger.error("Failed to update text mesh! Error: {}", updated.error());
                return;
            }

            meta.mesh = updated.value();
            return;
        }

        // we need to reallocate the mesh
        _renderer->meshBuffer().removeMesh(meta.mesh);

        const std::size_t newQuadSize = std::max(meta.maxQuads * 2, quadsUsed);

        Mesh mesh = _renderer->meshBuffer().reserveMesh(newQuadSize * 4,  // 4 vertices per quad
            newQuadSize * 6                                               // 6 indices per quad
        );
        meta.mesh = mesh;
        meta.maxQuads = newQuadSize;

        MeshData textMeshData = TextMeshBuilder::build(text, style, false);

        Result<Mesh> updated = _renderer->meshBuffer().updateMesh(meta.mesh, textMeshData);
        if (updated.isError()) {
            Engine.logger.error("Failed to update text mesh! Error: {}", updated.error());
            return;
        }

        meta.mesh = updated.value();
    };

    bool isValidHandle(TextMeshHandle handle) const {
        return _textMeshPool.valid(handle);
    }

   private:
    struct TextMeshMeta {
        Mesh mesh{Mesh::none()};
        std::size_t maxQuads;
    };

    ObjectPool<TextMeshMeta> _textMeshPool;
    SystemParameter<Renderer> _renderer;
};

};  // namespace okay

#endif
