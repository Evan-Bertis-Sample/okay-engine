#ifndef __MATH_TYPES_H__
#define __MATH_TYPES_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace okay {

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    Transform(const glm::vec3& pos = glm::vec3(0.0f),
              const glm::vec3& scl = glm::vec3(1.0f),
              const glm::quat& rot = glm::identity<glm::quat>())
        : position(pos), scale(scl), rotation(rot) {}

    // asignment, equality comparison overloads
    Transform& operator=(const Transform& other) {
        position = other.position;
        scale = other.scale;
        rotation = other.rotation;
        return *this;
    }

    bool operator==(const Transform& other) const {
        return position == other.position && scale == other.scale && rotation == other.rotation;
    }

    bool operator!=(const Transform& other) const { return !(*this == other); }

    glm::mat4 toMatrix() const {
        glm::mat4 mat(1.0f);
        mat = glm::translate(mat, position);
        mat *= glm::mat4_cast(rotation);
        mat = glm::scale(mat, scale);
        return mat;
    }
};

struct Bounds {
    glm::vec3 minBound{};
    glm::vec3 maxBound{};

    Bounds() {}
    Bounds(glm::vec3 min, glm::vec3 max) : minBound(min), maxBound(max) {}

    static Bounds none() { return Bounds(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)); }

    void extend(glm::vec3 point) {
        minBound = glm::min(minBound, point);
        maxBound = glm::max(maxBound, point);
    }

    Bounds transform(const glm::mat4& matrix) const {
        glm::vec3 min = matrix * glm::vec4(minBound, 1.0f);
        glm::vec3 max = matrix * glm::vec4(maxBound, 1.0f);
        return Bounds(min, max);
    }

    Bounds transform(const Transform& t) const { return transform(t.toMatrix()); }

    class CornerIterator {
        glm::vec3 minBound;
        glm::vec3 maxBound;
        std::uint8_t index{0};

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = glm::vec3;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = glm::vec3;

        CornerIterator(const Bounds& bounds, std::uint8_t index = 0)
            : minBound(bounds.minBound), maxBound(bounds.maxBound), index(index) {}

        glm::vec3 operator*() const {
            switch (index) {
                case 0:
                    return {minBound.x, minBound.y, minBound.z};
                case 1:
                    return {maxBound.x, minBound.y, minBound.z};
                case 2:
                    return {minBound.x, maxBound.y, minBound.z};
                case 3:
                    return {maxBound.x, maxBound.y, minBound.z};
                case 4:
                    return {minBound.x, minBound.y, maxBound.z};
                case 5:
                    return {maxBound.x, minBound.y, maxBound.z};
                case 6:
                    return {minBound.x, maxBound.y, maxBound.z};
                case 7:
                    return {maxBound.x, maxBound.y, maxBound.z};
                default:
                    return {};
            }
        }

        CornerIterator& operator++() {
            if (index < 8) {
                ++index;
            }
            return *this;
        }

        CornerIterator operator++(int) {
            CornerIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const CornerIterator& other) const { return index == other.index; }

        bool operator!=(const CornerIterator& other) const { return !(*this == other); }
    };

    struct CornerRange {
        const Bounds& bounds;

        CornerIterator begin() const { return CornerIterator(bounds, 0); }
        CornerIterator end() const { return CornerIterator(bounds, 8); }
    };

    CornerRange corners() const { return CornerRange{*this}; }

    CornerIterator begin() const { return CornerIterator(*this, 0); }
    CornerIterator end() const { return CornerIterator(*this, 8); }
};

};  // namespace okay

#endif  // __MATH_TYPES_H__