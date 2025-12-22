#ifndef __OKAY_MESH_H__
#define __OKAY_MESH_H__

#include <glm/glm.hpp>
#include <vector>

namespace okay {

struct OkayVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;
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
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> colors;
    std::vector<uint32_t> indices;

   public:
    OkayMesh AddModel(const OkayMeshData& model);
    std::size_t Size() const { return positions.size(); }
    OkayModelView GetModelView(OkayMesh model) const;

    class iterator {
       public:
        using value_type = OkayVertex;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator(const OkayMeshBuffer* buffer, std::size_t index)
            : _buffer(buffer), _index(index) {}

        OkayVertex operator*() const {
            std::size_t attrIndex = _buffer->indices[_index];
            return OkayVertex{_buffer->positions[attrIndex], _buffer->normals[attrIndex],
                              _buffer->uvs[attrIndex], _buffer->colors[attrIndex]};
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
};

class OkayModelView {
   public:
    OkayModelView(const OkayMeshBuffer* buffer, OkayMesh model) : _buffer(buffer), _model(model) {}

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