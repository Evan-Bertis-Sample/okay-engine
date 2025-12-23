#ifndef __OKAY_MESH_H__
#define __OKAY_MESH_H__

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <okay/core/util/result.hpp>
#include <vector>

namespace okay {

struct OkayVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    static std::size_t numFloats() { return 3 + 3 + 3 + 2; }
};

struct OkayMeshData {
    std::vector<OkayVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct OkayMesh {
    std::size_t vertexOffset;
    std::size_t vertexCount;
    std::size_t indexOffset;
    std::size_t indexCount;

    static OkayMesh none() { return {0, 0, 0, 0}; }
    bool isEmpty() { return indexCount == 0; }
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

    std::size_t Size() const { return _indices.size(); }
    OkayMesh addMesh(const OkayMeshData& model);
    void removeMesh(const OkayMesh& mesh);
    Failable bindMeshData();

    OkayModelView GetModelView(OkayMesh model) const;

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

            glm::vec3* position =
                std::reinterpret_cast<glm::vec3*>(&_buffer->_bufferData[attrIndex + 0]);
            glm::vec3* normal =
                std::reinterpret_cast<glm::vec3*>(&_buffer->_bufferData[attrIndex + 3]);
            glm::vec3* color =
                std::reinterpret_cast<glm::vec3*>(&_buffer->_bufferData[attrIndex + 6]);
            glm::vec2* uv = std::reinterpret_cast<glm::vec2*>(&_buffer->_bufferData[attrIndex + 9]);

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

};  // namespace okay

#endif  // __OKAY_MESH_H__