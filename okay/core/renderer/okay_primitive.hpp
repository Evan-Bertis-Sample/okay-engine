#ifndef __OKAY_PRIMITIVE_H__
#define __OKAY_PRIMITIVE_H__

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <okay/core/renderer/okay_model.hpp>
#include <vector>

namespace okay {

template <typename Options>
using VertexTransformation = std::function<OkayVertex(const OkayVertex&, const Options&)>;

class OkayPrimitives {
   public:
    struct RectOptions {
        glm::vec3 Center{0.0f};
        glm::vec2 Size{1.0f, 1.0f};
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};  // identity
        bool TwoSided{false};

        static const RectOptions& Default() { static RectOptions opt{}; return opt; }
    };

    // Rect in local XY plane, normal +Z, then rotated and translated.
    static OkayMeshData Rect(const RectOptions& opt = RectOptions::Default(),
                             const VertexTransformation<RectOptions>& xf = nullptr);

    struct PlaneOptions {
        glm::vec3 Center{0.0f};
        glm::vec2 Size{1.0f, 1.0f};
        glm::ivec2 Segments{1, 1};  // grid resolution
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};
        bool TwoSided{false};

        static const PlaneOptions& Default() { static PlaneOptions opt{}; return opt; }
    };

    static OkayMeshData Plane(const PlaneOptions& opt = PlaneOptions::Default(),
                              const VertexTransformation<PlaneOptions>& xf = nullptr);

    struct BoxOptions {
        glm::vec3 Center{0.0f};
        glm::vec3 Size{1.0f, 1.0f, 1.0f};
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};

        static const BoxOptions& Default() { static BoxOptions opt{}; return opt; }
    };

    static OkayMeshData Box(const BoxOptions& opt = BoxOptions::Default(),
                            const VertexTransformation<BoxOptions>& xf = nullptr);

    // UV Sphere: lat/long parameterization (has pole triangles / seam)
    struct UVSphereOptions {
        glm::vec3 Center{0.0f};
        float Radius{0.5f};
        int Segments{32};  // around (longitude)
        int Rings{16};     // vertical (latitude)
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};
        bool GenerateTangents{false};  // placeholder if you add tangents later

        static const UVSphereOptions& Default() { static UVSphereOptions opt{}; return opt;}
    };

    static OkayMeshData UVSphere(const UVSphereOptions& opt = UVSphereOptions::Default(),
                                 const VertexTransformation<UVSphereOptions>& xf = nullptr);

    // Icosphere/Geosphere: subdivided icosahedron (more uniform triangles)
    struct IcoSphereOptions {
        glm::vec3 Center{0.0f};
        float Radius{0.5f};
        int Subdivisions{2};  // 0..~6 typical
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};

        static const IcoSphereOptions& Default() { static IcoSphereOptions opt{}; return opt;}
    };

    static OkayMeshData IcoSphere(const IcoSphereOptions& opt = IcoSphereOptions::Default(),
                                  const VertexTransformation<IcoSphereOptions>& xf = nullptr);

    struct CylinderOptions {
        glm::vec3 Center{0.0f};
        float Radius{0.5f};
        float Height{1.0f};
        int Segments{32};
        bool Caps{true};
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};

        static const CylinderOptions& Default() { static CylinderOptions opt{}; return opt;}
    };

    static OkayMeshData Cylinder(const CylinderOptions& opt = CylinderOptions::Default(),
                                 const VertexTransformation<CylinderOptions>& xf = nullptr);

    struct ConeOptions {
        glm::vec3 Center{0.0f};
        float Radius{0.5f};
        float Height{1.0f};
        int Segments{32};
        bool Cap{true};
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};

        static const ConeOptions& Default() { static ConeOptions opt{}; return opt;}
    };

    static OkayMeshData Cone(const ConeOptions& opt = ConeOptions::Default(),
                             const VertexTransformation<ConeOptions>& xf = nullptr);

    struct CapsuleOptions {
        glm::vec3 Center{0.0f};
        float Radius{0.5f};
        float Height{1.5f};  // total height, includes hemispheres
        int Segments{32};    // around
        int Rings{8};        // per hemisphere
        glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};

        static const CapsuleOptions& Default() { static CapsuleOptions opt{}; return opt;}
    };

    static OkayMeshData Capsule(const CapsuleOptions& opt = CapsuleOptions::Default(),
                                const VertexTransformation<CapsuleOptions>& xf = nullptr);

   private:
    static void ApplyTransform(OkayVertex& v, const glm::vec3& center, const glm::quat& rot);

    template <typename OptionsT>
    static void ApplyUserXform(OkayVertex& v, const OptionsT& opt,
                               const VertexTransformation<OptionsT>& xf) {
        if (xf) v = xf(v, opt);
    }
};

}  // namespace okay

#endif  // __OKAY_PRIMITIVE_H__
