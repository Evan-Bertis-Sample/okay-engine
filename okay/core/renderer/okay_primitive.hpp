#ifndef __OKAY_PRIMITIVE_H__
#define __OKAY_PRIMITIVE_H__

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <okay/core/renderer/okay_model.hpp>
#include <type_traits>
#include <utility>

namespace okay::primitives {

template <typename BuilderT>
using VertexTransformation =
    std::function<okay::OkayVertex(const okay::OkayVertex&, const BuilderT&)>;

#define OKAY_PRIM_FIELD(FieldName)                     \
    auto& with##FieldName(decltype(FieldName) value) { \
        this->FieldName = std::move(value);            \
        return *this;                                  \
    }

struct Placement {
    glm::vec3 Center{0.0f};
    glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};  // identity

    Placement& translate(const glm::vec3& d) {
        Center += d;
        return *this;
    }
    Placement& rotate(const glm::quat& q) {
        Rotation = q * Rotation;
        return *this;
    }
};

template <class Derived>
class PlacementMixin {
   public:
    Derived& withCenter(const glm::vec3& c) {
        _transform.Center = c;
        return self();
    }
    Derived& withRotation(const glm::quat& q) {
        _transform.Rotation = q;
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

    const Placement& transform() const { return _transform; }

   protected:
    Placement _transform{};

   private:
    Derived& self() { return static_cast<Derived&>(*this); }
};

class RectBuilder final : public PlacementMixin<RectBuilder> {
   public:
    glm::vec2 Size{1.0f, 1.0f};
    bool TwoSided{false};

    OKAY_PRIM_FIELD(Size)
    OKAY_PRIM_FIELD(TwoSided)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<RectBuilder>& xf) const;
};

class PlaneBuilder final : public PlacementMixin<PlaneBuilder> {
   public:
    glm::vec2 Size{1.0f, 1.0f};
    glm::ivec2 Segments{1, 1};
    bool TwoSided{false};

    OKAY_PRIM_FIELD(Size)
    OKAY_PRIM_FIELD(Segments)
    OKAY_PRIM_FIELD(TwoSided)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<PlaneBuilder>& xf) const;
};

class BoxBuilder final : public PlacementMixin<BoxBuilder> {
   public:
    glm::vec3 Size{1.0f, 1.0f, 1.0f};

    OKAY_PRIM_FIELD(Size)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<BoxBuilder>& xf) const;
};

class UVSphereBuilder final : public PlacementMixin<UVSphereBuilder> {
   public:
    float Radius{0.5f};
    int Segments{32};
    int Rings{16};
    bool GenerateTangents{false};

    OKAY_PRIM_FIELD(Radius)
    OKAY_PRIM_FIELD(Segments)
    OKAY_PRIM_FIELD(Rings)
    OKAY_PRIM_FIELD(GenerateTangents)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<UVSphereBuilder>& xf) const;
};

class IcoSphereBuilder final : public PlacementMixin<IcoSphereBuilder> {
   public:
    float Radius{0.5f};
    int Subdivisions{2};

    OKAY_PRIM_FIELD(Radius)
    OKAY_PRIM_FIELD(Subdivisions)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<IcoSphereBuilder>& xf) const;
};

class CylinderBuilder final : public PlacementMixin<CylinderBuilder> {
   public:
    float Radius{0.5f};
    float Height{1.0f};
    int Segments{32};
    bool Caps{true};

    OKAY_PRIM_FIELD(Radius)
    OKAY_PRIM_FIELD(Height)
    OKAY_PRIM_FIELD(Segments)
    OKAY_PRIM_FIELD(Caps)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<CylinderBuilder>& xf) const;
};

class ConeBuilder final : public PlacementMixin<ConeBuilder> {
   public:
    float Radius{0.5f};
    float Height{1.0f};
    int Segments{32};
    bool Cap{true};

    OKAY_PRIM_FIELD(Radius)
    OKAY_PRIM_FIELD(Height)
    OKAY_PRIM_FIELD(Segments)
    OKAY_PRIM_FIELD(Cap)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<ConeBuilder>& xf) const;
};

class CapsuleBuilder final : public PlacementMixin<CapsuleBuilder> {
   public:
    float Radius{0.5f};
    float Height{1.5f};
    int Segments{32};
    int Rings{8};

    OKAY_PRIM_FIELD(Radius)
    OKAY_PRIM_FIELD(Height)
    OKAY_PRIM_FIELD(Segments)
    OKAY_PRIM_FIELD(Rings)

    okay::OkayMeshData build() const { return build(nullptr); }
    okay::OkayMeshData build(const VertexTransformation<CapsuleBuilder>& xf) const;
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

#endif  // __OKAY_PRIMITIVE_H__
