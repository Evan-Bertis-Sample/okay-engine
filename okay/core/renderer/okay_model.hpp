#ifndef __OKAY_MODEL_H__
#define __OKAY_MODEL_H__

#include <glm/glm.hpp>
#include <vector>

namespace okay {

struct OkayVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 UV;
    glm::vec3 Color;
};

struct OkayModel {
    std::size_t Offset;
    std::size_t Length;
};

class OkayModelView;

class OkayModelBuffer {
   private:
    std::vector<glm::vec3> Positions;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> UVs;
    std::vector<glm::vec3> Colors;

   public:
    OkayModel AddModel(const std::vector<OkayVertex>& model);
    std::size_t Size() const { return Positions.size(); }
    OkayModelView GetModelView(OkayModel model) const;

    class iterator {
       public:
        using value_type = OkayVertex;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator(const OkayModelBuffer* buffer, std::size_t index)
            : _buffer(buffer), _index(index) {}

        OkayVertex operator*() const {
            return OkayVertex{_buffer->Positions[_index], _buffer->Normals[_index],
                              _buffer->UVs[_index], _buffer->Colors[_index]};
        }

        iterator& operator++() {
            ++_index;
            return *this;
        }

        bool operator!=(const iterator& other) const { return _index != other._index; }

       private:
        const OkayModelBuffer* _buffer;
        std::size_t _index;
    };
};

class OkayModelView {
   public:
    OkayModelView(const OkayModelBuffer* buffer, OkayModel model)
        : _buffer(buffer), _model(model) {}

    OkayModelBuffer::iterator begin() const {
        return OkayModelBuffer::iterator(_buffer, _model.Offset);
    }

    OkayModelBuffer::iterator end() const {
        return OkayModelBuffer::iterator(_buffer, _model.Offset + _model.Length);
    }

   private:
    const OkayModelBuffer* _buffer;
    OkayModel _model;
};

};  // namespace okay

#endif  // __OKAY_MODEL_H__