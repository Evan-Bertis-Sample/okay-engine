#ifndef __PRIMITIVE_H__
#define __PRIMITIVE_H__

#include <okay/core/renderer/mesh.hpp>

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <type_traits>
#include <utility>

namespace okay::primitives {

template <typename BuilderT>
using VertexTransformation =
    std::function<okay::MeshVertex(const okay::MeshVertex&, const BuilderT&)>;

template <typename BuilderT>
static VertexTransformation<BuilderT> colorTransform(const glm::vec4& color) {
    return [color](const okay::MeshVertex& v, const BuilderT&) -> okay::MeshVertex {
        return {v.position, v.normal, color, v.uv};
    };
}

#define OKAY_PRIM_FIELD(FieldName)                    \
    auto& FieldName##Set(decltype(FieldName) value) { \
        this->FieldName = std::move(value);           \
        return *this;                                 \
    }

struct Placement {
    glm::vec3 center{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // identity

    Placement& translate(const glm::vec3& d) {
        center += d;
        return *this;
    }
    Placement& rotate(const glm::quat& q) {
        rotation = q * rotation;
        return *this;
    }
};

template <class Derived>
class PlacementMixin {
   public:
    Derived& withCenter(const glm::vec3& c) {
        _transform.center = c;
        return self();
    }
    Derived& withRotation(const glm::quat& q) {
        _transform.rotation = q;
        return self();
    }
    Derived& translate(const glm::vec3& d) {
        _transform.translate(d);
        return self();
    }
    Derived& rotate(const glm::quat& q) {
        _transform.rotate(q);
        return self();
    }

    const Placement& transform() const {
        return _transform;
    }

   protected:
    Placement _transform{};

   private:
    Derived& self() {
        return static_cast<Derived&>(*this);
    }
};

class RectBuilder final : public PlacementMixin<RectBuilder> {
   public:
    glm::vec2 size{1.0f, 1.0f};
    bool twoSided{false};

    OKAY_PRIM_FIELD(size)
    OKAY_PRIM_FIELD(twoSided)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<RectBuilder>& xf) const;
};

class PlaneBuilder final : public PlacementMixin<PlaneBuilder> {
   public:
    glm::vec2 size{1.0f, 1.0f};
    glm::ivec2 segments{1, 1};
    bool twoSided{false};

    OKAY_PRIM_FIELD(size)
    OKAY_PRIM_FIELD(segments)
    OKAY_PRIM_FIELD(twoSided)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<PlaneBuilder>& xf) const;
};

class BoxBuilder final : public PlacementMixin<BoxBuilder> {
   public:
    glm::vec3 size{1.0f, 1.0f, 1.0f};

    OKAY_PRIM_FIELD(size)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<BoxBuilder>& xf) const;
};

class UVSphereBuilder final : public PlacementMixin<UVSphereBuilder> {
   public:
    float radius{0.5f};
    int segments{32};
    int rings{16};
    bool generateTangents{false};

    OKAY_PRIM_FIELD(radius)
    OKAY_PRIM_FIELD(segments)
    OKAY_PRIM_FIELD(rings)
    OKAY_PRIM_FIELD(generateTangents)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<UVSphereBuilder>& xf) const;
};

class IcoSphereBuilder final : public PlacementMixin<IcoSphereBuilder> {
   public:
    float radius{0.5f};
    int subdivisions{2};

    OKAY_PRIM_FIELD(radius)
    OKAY_PRIM_FIELD(subdivisions)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<IcoSphereBuilder>& xf) const;
};

class CylinderBuilder final : public PlacementMixin<CylinderBuilder> {
   public:
    float radius{0.5f};
    float height{1.0f};
    int segments{32};
    bool caps{true};

    OKAY_PRIM_FIELD(radius)
    OKAY_PRIM_FIELD(height)
    OKAY_PRIM_FIELD(segments)
    OKAY_PRIM_FIELD(caps)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<CylinderBuilder>& xf) const;
};

class ConeBuilder final : public PlacementMixin<ConeBuilder> {
   public:
    float radius{0.5f};
    float height{1.0f};
    int segments{32};
    bool cap{true};

    OKAY_PRIM_FIELD(radius)
    OKAY_PRIM_FIELD(height)
    OKAY_PRIM_FIELD(segments)
    OKAY_PRIM_FIELD(cap)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<ConeBuilder>& xf) const;
};

class CapsuleBuilder final : public PlacementMixin<CapsuleBuilder> {
   public:
    float radius{0.5f};
    float height{1.5f};
    int segments{32};
    int rings{8};

    OKAY_PRIM_FIELD(radius)
    OKAY_PRIM_FIELD(height)
    OKAY_PRIM_FIELD(segments)
    OKAY_PRIM_FIELD(rings)

    okay::MeshData build() const {
        return build(nullptr);
    }
    okay::MeshData build(const VertexTransformation<CapsuleBuilder>& xf) const;
};

inline RectBuilder rect() {
    return RectBuilder{};
}
inline PlaneBuilder plane() {
    return PlaneBuilder{};
}
inline BoxBuilder box() {
    return BoxBuilder{};
}
inline UVSphereBuilder uvSphere() {
    return UVSphereBuilder{};
}
inline IcoSphereBuilder icoSphere() {
    return IcoSphereBuilder{};
}
inline CylinderBuilder cylinder() {
    return CylinderBuilder{};
}
inline ConeBuilder cone() {
    return ConeBuilder{};
}
inline CapsuleBuilder capsule() {
    return CapsuleBuilder{};
}

}  // namespace okay::primitives

#endif  // _PRIMITIVE_H__
