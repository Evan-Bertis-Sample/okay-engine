#ifndef __OKAY_MESH_H__
#define __OKAY_MESH_H__

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <okay/core/util/result.hpp>
#include <vector>

namespace okay {

struct OkayVertex {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 0.0f, 0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    glm::vec2 uv{0.0f, 0.0f};

    static std::size_t numFloats() {
        return 3 + 3 + 3 + 2;
    }

    OkayVertex& operator=(const OkayVertex& other) {
        position = other.position;
        normal = other.normal;
        color = other.color;
        uv = other.uv;
        return *this;
    }

    bool operator==(const OkayVertex& other) const {
        return position == other.position && normal == other.normal && color == other.color &&
               uv == other.uv;
    }

    bool operator!=(const OkayVertex& other) const {
        return !(*this == other);
    }
};

struct OkayMeshData {
    std::vector<OkayVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct OkayMesh {
    std::size_t vertexOffset{0};
    std::size_t vertexCount{0};
    std::size_t indexOffset{0};
    std::size_t indexCount{0};

    OkayMesh() = default;
    OkayMesh(std::size_t vOffset, std::size_t vCount, std::size_t iOffset, std::size_t iCount)
        : vertexOffset(vOffset), vertexCount(vCount), indexOffset(iOffset), indexCount(iCount) {}

    static OkayMesh none() {
        return OkayMesh(0, 0, 0, 0);
    }
    
    bool isEmpty() {
        return indexCount == 0;
    }


    OkayMesh& operator=(const OkayMesh& other) {
        vertexOffset = other.vertexOffset;
        vertexCount = other.vertexCount;
        indexOffset = other.indexOffset;
        indexCount = other.indexCount;
        return *this;
    }

    bool operator==(const OkayMesh& other) const {
        return vertexOffset == other.vertexOffset && vertexCount == other.vertexCount &&
               indexOffset == other.indexOffset && indexCount == other.indexCount;
    }

    bool operator!=(const OkayMesh& other) const {
        return !(*this == other);
    }
};

class OkayModelView;

class OkayMeshBuffer {
   private:
    std::vector<float> _bufferData;
    std::vector<uint32_t> _indices;

    // openGL buffers
    GLuint _vao{0};
    GLuint _vbo{0};
    GLuint _ebo{0};

    bool _hasBound{false};

   public:
    static Failable initVertexAttributes();

    std::size_t size() const { return _indices.size(); }
    OkayMesh addMesh(const OkayMeshData& model);
    void removeMesh(const OkayMesh& mesh);
    Failable bindMeshData();
    void drawMesh(const OkayMesh& mesh);

    class iterator {
       public:
        using value_type = OkayVertex;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator(const OkayMeshBuffer* buffer, std::size_t index)
            : _buffer(buffer), _index(index) {}

        OkayVertex operator*() const {
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

            return OkayVertex{*position, *normal, *color, *uv};
        }

        iterator& operator++() {
            ++_index;
            return *this;
        }

        bool operator!=(const iterator& other) const { return _index != other._index; }

       private:
        const OkayMeshBuffer* _buffer;
        std::size_t _index;
    };

    class OkayModelView {
       public:
        OkayModelView(const OkayMeshBuffer* buffer, OkayMesh model)
            : _buffer(buffer), _model(model) {}

        OkayMeshBuffer::iterator begin() const {
            return OkayMeshBuffer::iterator(_buffer, _model.indexOffset);
        }

        OkayMeshBuffer::iterator end() const {
            return OkayMeshBuffer::iterator(_buffer, _model.indexOffset + _model.indexCount);
        }

       private:
        const OkayMeshBuffer* _buffer;
        OkayMesh _model;
    };
};

}  // namespace okay

#endif  // __OKAY_MESH_H__