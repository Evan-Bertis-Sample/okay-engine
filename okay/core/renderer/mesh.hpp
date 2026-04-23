#ifndef __MESH_H__
#define __MESH_H__

#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/math_types.hpp>
#include <okay/core/util/result.hpp>

#include <glm/glm.hpp>
#include <vector>

namespace okay {

struct MeshVertex {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 0.0f, 0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    glm::vec2 uv{0.0f, 0.0f};

    static std::size_t numFloats() { return 3 + 3 + 3 + 2; }

    static std::size_t stride() { return numFloats() * sizeof(float); }

    MeshVertex& operator=(const MeshVertex& other) {
        position = other.position;
        normal = other.normal;
        color = other.color;
        uv = other.uv;
        return *this;
    }

    bool operator==(const MeshVertex& other) const {
        return position == other.position && normal == other.normal && color == other.color &&
               uv == other.uv;
    }

    bool operator!=(const MeshVertex& other) const { return !(*this == other); }
};

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct Mesh {
    std::size_t vertexOffset{0};
    std::size_t vertexCount{0};
    std::size_t indexOffset{0};
    std::size_t indexCount{0};
    Bounds bounds{};

    Mesh() = default;
    Mesh(std::size_t vOffset, std::size_t vCount, std::size_t iOffset, std::size_t iCount, Bounds b)
        : vertexOffset(vOffset),
          vertexCount(vCount),
          indexOffset(iOffset),
          indexCount(iCount),
          bounds(b) {}

    static Mesh none() { return Mesh(0, 0, 0, 0, Bounds::none()); }

    bool isEmpty() { return indexCount == 0; }

    Mesh& operator=(const Mesh& other) {
        vertexOffset = other.vertexOffset;
        vertexCount = other.vertexCount;
        indexOffset = other.indexOffset;
        indexCount = other.indexCount;
        return *this;
    }

    bool operator==(const Mesh& other) const {
        return vertexOffset == other.vertexOffset && vertexCount == other.vertexCount &&
               indexOffset == other.indexOffset && indexCount == other.indexCount;
    }

    bool operator!=(const Mesh& other) const { return !(*this == other); }
};

class ModelView;

class MeshBuffer {
   private:
    std::vector<float> _bufferData;
    std::vector<uint32_t> _indices;

    // openGL buffers
    GLuint _vao{0};
    GLuint _vbo{0};
    GLuint _ebo{0};

    bool _hasInitVertexAttributes{false};
    bool _dataOutofDate{true};

    struct BlockMeta {
        std::size_t vOffset;
        std::size_t vCount;
        std::size_t iOffset;
        std::size_t iCount;
        bool isFree = false;
    };

    std::vector<BlockMeta> _blocks;

   public:
    Failable initVertexAttributes();

    std::size_t size() const { return _indices.size(); }
    Mesh addMesh(const MeshData& model);
    void removeMesh(const Mesh& mesh);
    float* getMeshPointer(const Mesh& mesh);
    Failable bindMeshData();
    void drawMesh(const Mesh& mesh);

    class iterator {
       public:
        using value_type = MeshVertex;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator(const MeshBuffer* buffer, std::size_t index) : _buffer(buffer), _index(index) {}

        MeshVertex operator*() const {
            std::size_t attrIndex = _buffer->_indices[_index];

            // A SLIGHT bit of UB at the end of the reinterpret cast
            // as vec3 is 12 bytes, and 4 byte aligned
            // our buffer is tightly packed, which means that
            // the padding of vec3 bleeds into the next vec3
            // it's fine though

            const glm::vec3* position =
                reinterpret_cast<const glm::vec3*>(&_buffer->_bufferData[attrIndex + 0]);
            const glm::vec3* normal =
                reinterpret_cast<const glm::vec3*>(&_buffer->_bufferData[attrIndex + 3]);
            const glm::vec3* color =
                reinterpret_cast<const glm::vec3*>(&_buffer->_bufferData[attrIndex + 6]);
            const glm::vec2* uv =
                reinterpret_cast<const glm::vec2*>(&_buffer->_bufferData[attrIndex + 9]);

            return MeshVertex{*position, *normal, *color, *uv};
        }

        iterator& operator++() {
            ++_index;
            return *this;
        }

        bool operator!=(const iterator& other) const { return _index != other._index; }

       private:
        const MeshBuffer* _buffer;
        std::size_t _index;
    };

    class OkayModelView {
       public:
        OkayModelView(const MeshBuffer* buffer, Mesh model) : _buffer(buffer), _model(model) {}

        MeshBuffer::iterator begin() const {
            return MeshBuffer::iterator(_buffer, _model.indexOffset);
        }

        MeshBuffer::iterator end() const {
            return MeshBuffer::iterator(_buffer, _model.indexOffset + _model.indexCount);
        }

       private:
        const MeshBuffer* _buffer;
        Mesh _model;
    };
};

}  // namespace okay

#endif  // _MESH_H__
