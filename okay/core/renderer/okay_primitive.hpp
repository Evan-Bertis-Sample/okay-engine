#ifndef __OKAY_PRIMITIVE_H__
#define __OKAY_PRIMITIVE_H__

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <okay/core/renderer/okay_model.hpp>
#include <utility>

namespace okay::primitives {

template <typename BuilderT>
using VertexTransformation =
    std::function<okay::OkayVertex(const okay::OkayVertex&, const BuilderT&)>;

#define OKAY_PRIM_FIELD(FieldName)                \
    template <class T>                            \
    auto& With##FieldName(T&& value) {            \
        this->FieldName = std::forward<T>(value); \
        return *this;                             \
    }

struct Placement {
    glm::vec3 Center{0.0f};
    glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};  // identity

    Placement& Translate(const glm::vec3& d) {
        Center += d;
        return *this;
    }
    Placement& Rotate(const glm::quat& q) {
        Rotation = q * Rotation;
        return *this;
    }
};

template <class Derived>
class PlacementMixin {
   public:
    Derived& WithCenter(const glm::vec3& c) {
        _Xform.Center = c;
        return Self();
    }
    Derived& WithRotation(const glm::quat& q) {
        _Xform.Rotation = q;
        return Self();
    }
    Derived& Translate(const glm::vec3& d) {
        _Xform.Translate(d);
        return Self();
    }
    Derived& Rotate(const glm::quat& q) {
        _Xform.Rotate(q);
        return Self();
    }

    const Placement& Xform() const { return _Xform; }

   protected:
    Placement _Xform{};

   private:
    Derived& Self() { return static_cast<Derived&>(*this); }
};

class RectBuilder final : public PlacementMixin<RectBuilder> {
   public:
    glm::vec2 Size{1.0f, 1.0f};
    bool TwoSided{false};

    OKAY_PRIM_FIELD(Size)
    OKAY_PRIM_FIELD(TwoSided)

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<RectBuilder>& xf) const;
};

class PlaneBuilder final : public PlacementMixin<PlaneBuilder> {
   public:
    glm::vec2 Size{1.0f, 1.0f};
    glm::ivec2 Segments{1, 1};
    bool TwoSided{false};

    OKAY_PRIM_FIELD(Size)
    OKAY_PRIM_FIELD(Segments)
    OKAY_PRIM_FIELD(TwoSided)

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<PlaneBuilder>& xf) const;
};

class BoxBuilder final : public PlacementMixin<BoxBuilder> {
   public:
    glm::vec3 Size{1.0f, 1.0f, 1.0f};

    OKAY_PRIM_FIELD(Size)

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<BoxBuilder>& xf) const;
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

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<UVSphereBuilder>& xf) const;
};

class IcoSphereBuilder final : public PlacementMixin<IcoSphereBuilder> {
   public:
    float Radius{0.5f};
    int Subdivisions{2};

    OKAY_PRIM_FIELD(Radius)
    OKAY_PRIM_FIELD(Subdivisions)

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<IcoSphereBuilder>& xf) const;
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

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<CylinderBuilder>& xf) const;
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

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<ConeBuilder>& xf) const;
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

    okay::OkayMeshData Build() const { return Build(nullptr); }
    okay::OkayMeshData Build(const VertexTransformation<CapsuleBuilder>& xf) const;
};

inline RectBuilder Rect() {
    return RectBuilder{};
}
inline PlaneBuilder Plane() {
    return PlaneBuilder{};
}
inline BoxBuilder Box() {
    return BoxBuilder{};
}
inline UVSphereBuilder UVSphere() {
    return UVSphereBuilder{};
}
inline IcoSphereBuilder IcoSphere() {
    return IcoSphereBuilder{};
}
inline CylinderBuilder Cylinder() {
    return CylinderBuilder{};
}
inline ConeBuilder Cone() {
    return ConeBuilder{};
}
inline CapsuleBuilder Capsule() {
    return CapsuleBuilder{};
}

}  // namespace okay::primitives

#endif  // __OKAY_PRIMITIVE_H__
