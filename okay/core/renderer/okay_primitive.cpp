#include "okay_primitive.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace okay::primitives {

static constexpr float kPi = 3.14159265358979323846f;

template <typename BuilderT>
static inline okay::OkayVertex applyTransforms(const BuilderT& builder,
                                               const VertexTransformation<BuilderT>& xf,
                                               okay::OkayVertex v) {
    const Placement& placement = builder.transform();

    v.position = placement.rotation * v.position + placement.center;
    v.normal = glm::normalize(placement.rotation * v.normal);

    if (xf) v = xf(v, builder);
    return v;
}

okay::OkayMeshData RectBuilder::build(const VertexTransformation<RectBuilder>& xf) const {
    okay::OkayMeshData out;
    out.vertices.reserve(twoSided ? 8 : 4);
    out.indices.reserve(twoSided ? 12 : 6);

    const float halfWidth = size.x * 0.5f;
    const float halfHeight = size.y * 0.5f;

    const auto PushVertex = [&](glm::vec3 position, glm::vec3 normal,
                                glm::vec2 uv) -> std::uint32_t {
        const std::uint32_t index = static_cast<std::uint32_t>(out.vertices.size());
        out.vertices.push_back(applyTransforms(*this, xf,
                                               okay::OkayVertex{
                                                   .position = position,
                                                   .normal = normal,
                                                   .uv = uv,
                                               }));
        return index;
    };

    // Local quad in XY plane, +Z normal, CCW winding
    PushVertex({-halfWidth, -halfHeight, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f});  // 0
    PushVertex({+halfWidth, -halfHeight, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f});  // 1
    PushVertex({+halfWidth, +halfHeight, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f});  // 2
    PushVertex({-halfWidth, +halfHeight, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f});  // 3

    out.indices.insert(out.indices.end(), {0, 1, 2, 0, 2, 3});

    if (twoSided) {
        const std::uint32_t backBase = 4;

        // Duplicate vertices with flipped normals
        for (int i = 0; i < 4; ++i) {
            okay::OkayVertex v = out.vertices[i];
            v.normal = -v.normal;
            // placement already applied; only apply user transform again so it can differentiate
            if (xf) v = xf(v, *this);
            out.vertices.push_back(v);
        }

        // Back face, reversed winding
        out.indices.insert(out.indices.end(), {backBase + 0, backBase + 2, backBase + 1,
                                               backBase + 0, backBase + 3, backBase + 2});
    }

    return out;
}

okay::OkayMeshData PlaneBuilder::build(const VertexTransformation<PlaneBuilder>& xf) const {
    okay::OkayMeshData out;

    const int segmentsX = std::max(1, segments.x);
    const int segmentsY = std::max(1, segments.y);

    const int vertexCountX = segmentsX + 1;
    const int vertexCountY = segmentsY + 1;

    out.vertices.reserve(static_cast<size_t>(vertexCountX * vertexCountY) * (twoSided ? 2 : 1));
    out.indices.reserve(static_cast<size_t>(segmentsX * segmentsY) * 6 * (twoSided ? 2 : 1));

    const float halfWidth = size.x * 0.5f;
    const float halfHeight = size.y * 0.5f;

    const auto EmitGrid = [&](float normalSign) {
        const glm::vec3 surfaceNormal = {0.0f, 0.0f, normalSign};
        const std::uint32_t baseVertex = static_cast<std::uint32_t>(out.vertices.size());

        for (int y = 0; y < vertexCountY; ++y) {
            const float v = static_cast<float>(y) / static_cast<float>(segmentsY);
            const float localY = -halfHeight + v * size.y;

            for (int x = 0; x < vertexCountX; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(segmentsX);
                const float localX = -halfWidth + u * size.x;

                out.vertices.push_back(applyTransforms(*this, xf,
                                                       okay::OkayVertex{
                                                           .position = {localX, localY, 0.0f},
                                                           .normal = surfaceNormal,
                                                           .uv = {u, v},
                                                       }));
            }
        }

        const auto VertexIndex = [&](int x, int y) -> std::uint32_t {
            return baseVertex + static_cast<std::uint32_t>(y * vertexCountX + x);
        };

        for (int y = 0; y < segmentsY; ++y) {
            for (int x = 0; x < segmentsX; ++x) {
                const std::uint32_t i0 = VertexIndex(x, y);
                const std::uint32_t i1 = VertexIndex(x + 1, y);
                const std::uint32_t i2 = VertexIndex(x + 1, y + 1);
                const std::uint32_t i3 = VertexIndex(x, y + 1);

                if (normalSign > 0.0f) {
                    out.indices.insert(out.indices.end(), {i0, i1, i2, i0, i2, i3});
                } else {
                    out.indices.insert(out.indices.end(), {i0, i2, i1, i0, i3, i2});
                }
            }
        }
    };

    EmitGrid(+1.0f);
    if (twoSided) EmitGrid(-1.0f);

    return out;
}

struct BoxFaceDefinition {
    glm::vec3 normal;
    glm::vec3 a, b, c, d;  // CCW quad in local space
};

okay::OkayMeshData BoxBuilder::build(const VertexTransformation<BoxBuilder>& xf) const {
    okay::OkayMeshData out;
    out.vertices.reserve(24);
    out.indices.reserve(36);

    const glm::vec3 halfExtents = size * 0.5f;

    const BoxFaceDefinition faces[6] = {
        // +Z
        {{0, 0, 1},
         {-halfExtents.x, -halfExtents.y, halfExtents.z},
         {halfExtents.x, -halfExtents.y, halfExtents.z},
         {halfExtents.x, halfExtents.y, halfExtents.z},
         {-halfExtents.x, halfExtents.y, halfExtents.z}},
        // -Z
        {{0, 0, -1},
         {halfExtents.x, -halfExtents.y, -halfExtents.z},
         {-halfExtents.x, -halfExtents.y, -halfExtents.z},
         {-halfExtents.x, halfExtents.y, -halfExtents.z},
         {halfExtents.x, halfExtents.y, -halfExtents.z}},
        // +X
        {{1, 0, 0},
         {halfExtents.x, -halfExtents.y, halfExtents.z},
         {halfExtents.x, -halfExtents.y, -halfExtents.z},
         {halfExtents.x, halfExtents.y, -halfExtents.z},
         {halfExtents.x, halfExtents.y, halfExtents.z}},
        // -X
        {{-1, 0, 0},
         {-halfExtents.x, -halfExtents.y, -halfExtents.z},
         {-halfExtents.x, -halfExtents.y, halfExtents.z},
         {-halfExtents.x, halfExtents.y, halfExtents.z},
         {-halfExtents.x, halfExtents.y, -halfExtents.z}},
        // +Y
        {{0, 1, 0},
         {-halfExtents.x, halfExtents.y, halfExtents.z},
         {halfExtents.x, halfExtents.y, halfExtents.z},
         {halfExtents.x, halfExtents.y, -halfExtents.z},
         {-halfExtents.x, halfExtents.y, -halfExtents.z}},
        // -Y
        {{0, -1, 0},
         {-halfExtents.x, -halfExtents.y, -halfExtents.z},
         {halfExtents.x, -halfExtents.y, -halfExtents.z},
         {halfExtents.x, -halfExtents.y, halfExtents.z},
         {-halfExtents.x, -halfExtents.y, halfExtents.z}},
    };

    const auto PushFaceVertex = [&](glm::vec3 position, glm::vec3 normal, glm::vec2 uv) {
        out.vertices.push_back(applyTransforms(*this, xf,
                                               okay::OkayVertex{
                                                   .position = position,
                                                   .normal = normal,
                                                   .uv = uv,
                                               }));
    };

    for (const auto& face : faces) {
        const std::uint32_t baseVertex = static_cast<std::uint32_t>(out.vertices.size());

        PushFaceVertex(face.a, face.normal, {0, 0});
        PushFaceVertex(face.b, face.normal, {1, 0});
        PushFaceVertex(face.c, face.normal, {1, 1});
        PushFaceVertex(face.d, face.normal, {0, 1});

        out.indices.insert(out.indices.end(), {baseVertex + 0, baseVertex + 1, baseVertex + 2,
                                               baseVertex + 0, baseVertex + 2, baseVertex + 3});
    }

    return out;
}

okay::OkayMeshData UVSphereBuilder::build(const VertexTransformation<UVSphereBuilder>& xf) const {
    okay::OkayMeshData out;

    const int longitudeSegments = std::max(3, segments);
    const int latitudeRings = std::max(2, rings);

    const int vertexColumns = longitudeSegments + 1;  // seam duplicate
    const int vertexRows = latitudeRings + 1;         // include poles

    out.vertices.reserve(static_cast<size_t>(vertexColumns * vertexRows));
    out.indices.reserve(static_cast<size_t>(longitudeSegments * latitudeRings * 6));

    for (int row = 0; row < vertexRows; ++row) {
        const float v = static_cast<float>(row) / static_cast<float>(latitudeRings);  // 0..1
        const float phi = v * kPi;                                                    // 0..pi

        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (int col = 0; col < vertexColumns; ++col) {
            const float u =
                static_cast<float>(col) / static_cast<float>(longitudeSegments);  // 0..1
            const float theta = u * (2.0f * kPi);                                 // 0..2pi

            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            const glm::vec3 unitNormal = {cosTheta * sinPhi, sinTheta * sinPhi, cosPhi};
            const glm::vec3 localPosition = unitNormal * radius;

            out.vertices.push_back(applyTransforms(*this, xf,
                                                   okay::OkayVertex{
                                                       .position = localPosition,
                                                       .normal = unitNormal,
                                                       .uv = {u, 1.0f - v},
                                                   }));
        }
    }

    const auto VertexIndex = [&](int col, int row) -> std::uint32_t {
        return static_cast<std::uint32_t>(row * vertexColumns + col);
    };

    for (int row = 0; row < latitudeRings; ++row) {
        for (int col = 0; col < longitudeSegments; ++col) {
            const std::uint32_t i0 = VertexIndex(col, row);
            const std::uint32_t i1 = VertexIndex(col + 1, row);
            const std::uint32_t i2 = VertexIndex(col + 1, row + 1);
            const std::uint32_t i3 = VertexIndex(col, row + 1);

            out.indices.insert(out.indices.end(), {i0, i1, i2, i0, i2, i3});
        }
    }

    return out;
}

struct EdgeKey {
    std::uint32_t a, b;
    bool operator==(const EdgeKey& o) const { return a == o.a && b == o.b; }
};

struct EdgeKeyHash {
    std::size_t operator()(const EdgeKey& k) const noexcept {
        return (static_cast<std::size_t>(k.a) << 32) ^ static_cast<std::size_t>(k.b);
    }
};

static std::uint32_t getOrCreateMidpointVertex(
    std::vector<glm::vec3>& unitPositions,
    std::unordered_map<EdgeKey, std::uint32_t, EdgeKeyHash>& cache, std::uint32_t i0,
    std::uint32_t i1) {
    const std::uint32_t a = std::min(i0, i1);
    const std::uint32_t b = std::max(i0, i1);
    const EdgeKey key{a, b};

    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    const glm::vec3 midpoint = glm::normalize((unitPositions[a] + unitPositions[b]) * 0.5f);
    const std::uint32_t newIndex = static_cast<std::uint32_t>(unitPositions.size());
    unitPositions.push_back(midpoint);
    cache.emplace(key, newIndex);
    return newIndex;
}

okay::OkayMeshData IcoSphereBuilder::build(const VertexTransformation<IcoSphereBuilder>& xf) const {
    okay::OkayMeshData out;

    const int subdivisions = std::max(0, subdivisions);

    std::vector<glm::vec3> unitPositions;
    std::vector<std::uint32_t> triangles;

    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;

    unitPositions = {
        glm::normalize(glm::vec3{-1, t, 0}),  glm::normalize(glm::vec3{1, t, 0}),
        glm::normalize(glm::vec3{-1, -t, 0}), glm::normalize(glm::vec3{1, -t, 0}),
        glm::normalize(glm::vec3{0, -1, t}),  glm::normalize(glm::vec3{0, 1, t}),
        glm::normalize(glm::vec3{0, -1, -t}), glm::normalize(glm::vec3{0, 1, -t}),
        glm::normalize(glm::vec3{t, 0, -1}),  glm::normalize(glm::vec3{t, 0, 1}),
        glm::normalize(glm::vec3{-t, 0, -1}), glm::normalize(glm::vec3{-t, 0, 1}),
    };

    triangles = {0, 11, 5,  0, 5,  1, 0, 1, 7, 0, 7,  10, 0, 10, 11, 1, 5, 9, 5, 11,
                 4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3,  9,  4, 3,  4,  2, 3, 2, 6, 3,
                 6, 8,  3,  8, 9,  4, 9, 5, 2, 4, 11, 6,  2, 10, 8,  6, 7, 9, 8, 1};

    for (int s = 0; s < subdivisions; ++s) {
        std::unordered_map<EdgeKey, std::uint32_t, EdgeKeyHash> midpointCache;
        std::vector<std::uint32_t> refined;
        refined.reserve(triangles.size() * 4);

        for (size_t i = 0; i < triangles.size(); i += 3) {
            const std::uint32_t i0 = triangles[i + 0];
            const std::uint32_t i1 = triangles[i + 1];
            const std::uint32_t i2 = triangles[i + 2];

            const std::uint32_t a = getOrCreateMidpointVertex(unitPositions, midpointCache, i0, i1);
            const std::uint32_t b = getOrCreateMidpointVertex(unitPositions, midpointCache, i1, i2);
            const std::uint32_t c = getOrCreateMidpointVertex(unitPositions, midpointCache, i2, i0);

            refined.insert(refined.end(), {i0, a, c, i1, b, a, i2, c, b, a, b, c});
        }

        triangles.swap(refined);
    }

    out.vertices.reserve(unitPositions.size());
    out.indices = triangles;

    for (const glm::vec3& unitNormal : unitPositions) {
        // fallback UV mapping (not perfect for icosphere)
        const float u = std::atan2(unitNormal.y, unitNormal.x) / (2.0f * kPi) + 0.5f;
        const float v = std::acos(std::clamp(unitNormal.z, -1.0f, 1.0f)) / kPi;

        out.vertices.push_back(applyTransforms(*this, xf,
                                               okay::OkayVertex{
                                                   .position = unitNormal * radius,
                                                   .normal = unitNormal,
                                                   .uv = {u, 1.0f - v},
                                               }));
    }

    return out;
}

okay::OkayMeshData CylinderBuilder::build(const VertexTransformation<CylinderBuilder>& xf) const {
    okay::OkayMeshData out;

    const int radialSegments = std::max(3, segments);
    const int seamColumns = radialSegments + 1;
    const float halfHeight = height * 0.5f;

    out.vertices.reserve(static_cast<size_t>(seamColumns * 2) + (caps ? (2 + seamColumns * 2) : 0));
    out.indices.reserve(static_cast<size_t>(radialSegments * 6) +
                        (caps ? static_cast<size_t>(radialSegments * 6) : 0));

    // Side
    for (int col = 0; col < seamColumns; ++col) {
        const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
        const float theta = u * (2.0f * kPi);

        const float cosTheta = std::cos(theta);
        const float sinTheta = std::sin(theta);

        const glm::vec3 radialNormal = {cosTheta, sinTheta, 0.0f};
        const glm::vec3 bottomPos = radialNormal * radius + glm::vec3(0, 0, -halfHeight);
        const glm::vec3 topPos = radialNormal * radius + glm::vec3(0, 0, +halfHeight);

        out.vertices.push_back(applyTransforms(*this, xf,
                                               okay::OkayVertex{
                                                   .position = bottomPos,
                                                   .normal = radialNormal,
                                                   .uv = {u, 0.0f},
                                               }));

        out.vertices.push_back(applyTransforms(*this, xf,
                                               okay::OkayVertex{
                                                   .position = topPos,
                                                   .normal = radialNormal,
                                                   .uv = {u, 1.0f},
                                               }));
    }

    const auto SideVertexIndex = [&](int col, int top) -> std::uint32_t {
        return static_cast<std::uint32_t>(col * 2 + top);
    };

    for (int col = 0; col < radialSegments; ++col) {
        const std::uint32_t i00 = SideVertexIndex(col, 0);
        const std::uint32_t i01 = SideVertexIndex(col, 1);
        const std::uint32_t i10 = SideVertexIndex(col + 1, 0);
        const std::uint32_t i11 = SideVertexIndex(col + 1, 1);

        out.indices.insert(out.indices.end(), {i00, i10, i11, i00, i11, i01});
    }

    if (!caps) return out;

    // Caps
    const std::uint32_t bottomCenterIndex = static_cast<std::uint32_t>(out.vertices.size());
    out.vertices.push_back(applyTransforms(*this, xf,
                                           okay::OkayVertex{
                                               .position = {0, 0, -halfHeight},
                                               .normal = {0, 0, -1},
                                               .uv = {0.5f, 0.5f},
                                           }));

    const std::uint32_t topCenterIndex = static_cast<std::uint32_t>(out.vertices.size());
    out.vertices.push_back(applyTransforms(*this, xf,
                                           okay::OkayVertex{
                                               .position = {0, 0, +halfHeight},
                                               .normal = {0, 0, +1},
                                               .uv = {0.5f, 0.5f},
                                           }));

    // Bottom Ring
    const std::uint32_t bottomRingBase = static_cast<std::uint32_t>(out.vertices.size());
    for (int col = 0; col < seamColumns; ++col) {
        const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
        const float theta = u * (2.0f * kPi);

        const float cosTheta = std::cos(theta);
        const float sinTheta = std::sin(theta);

        out.vertices.push_back(applyTransforms(
            *this, xf,
            okay::OkayVertex{
                .position =
                    glm::vec3(cosTheta, sinTheta, 0.0f) * radius + glm::vec3(0, 0, -halfHeight),
                .normal = {0, 0, -1},
                .uv = {0.5f + cosTheta * 0.5f, 0.5f + sinTheta * 0.5f},
            }));
    }

    // Top Ring
    const std::uint32_t topRingBase = static_cast<std::uint32_t>(out.vertices.size());
    for (int col = 0; col < seamColumns; ++col) {
        const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
        const float theta = u * (2.0f * kPi);

        const float cosTheta = std::cos(theta);
        const float sinTheta = std::sin(theta);

        out.vertices.push_back(applyTransforms(
            *this, xf,
            okay::OkayVertex{
                .position =
                    glm::vec3(cosTheta, sinTheta, 0.0f) * radius + glm::vec3(0, 0, +halfHeight),
                .normal = {0, 0, +1},
                .uv = {0.5f + cosTheta * 0.5f, 0.5f + sinTheta * 0.5f},
            }));
    }

    // Cap indices
    for (int col = 0; col < radialSegments; ++col) {
        // bottom cap (winding for -Z)
        const std::uint32_t a = bottomRingBase + static_cast<std::uint32_t>(col);
        const std::uint32_t b = bottomRingBase + static_cast<std::uint32_t>(col + 1);
        out.indices.insert(out.indices.end(), {bottomCenterIndex, b, a});

        // top cap (+Z)
        const std::uint32_t c = topRingBase + static_cast<std::uint32_t>(col);
        const std::uint32_t d = topRingBase + static_cast<std::uint32_t>(col + 1);
        out.indices.insert(out.indices.end(), {topCenterIndex, c, d});
    }

    return out;
}

okay::OkayMeshData ConeBuilder::build(const VertexTransformation<ConeBuilder>& xf) const {
    okay::OkayMeshData out;

    const int radialSegments = std::max(3, segments);
    const int seamColumns = radialSegments + 1;
    const float halfHeight = height * 0.5f;

    out.vertices.reserve(static_cast<size_t>(seamColumns + 1) + (cap ? (seamColumns + 1) : 0));
    out.indices.reserve(static_cast<size_t>(radialSegments * 3 * 2));

    // Side ring vertices
    for (int col = 0; col < seamColumns; ++col) {
        const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
        const float theta = u * (2.0f * kPi);

        const float cosTheta = std::cos(theta);
        const float sinTheta = std::sin(theta);

        const glm::vec3 ringPos =
            glm::vec3(cosTheta, sinTheta, 0.0f) * radius + glm::vec3(0, 0, -halfHeight);
        const glm::vec3 approxNormal =
            glm::normalize(glm::vec3(cosTheta, sinTheta, radius / height));

        out.vertices.push_back(applyTransforms(*this, xf,
                                               okay::OkayVertex{
                                                   .position = ringPos,
                                                   .normal = approxNormal,
                                                   .uv = {u, 0.0f},
                                               }));
    }

    // Apex
    const std::uint32_t apexIndex = static_cast<std::uint32_t>(out.vertices.size());
    out.vertices.push_back(applyTransforms(*this, xf,
                                           okay::OkayVertex{
                                               .position = {0, 0, +halfHeight},
                                               .normal = {0, 0, 1},  // simple
                                               .uv = {0.5f, 1.0f},
                                           }));

    // Side triangles
    for (int col = 0; col < radialSegments; ++col) {
        const std::uint32_t a = static_cast<std::uint32_t>(col);
        const std::uint32_t b = static_cast<std::uint32_t>(col + 1);
        out.indices.insert(out.indices.end(), {a, b, apexIndex});
    }

    if (!cap) return out;

    // Cap center
    const std::uint32_t capCenterIndex = static_cast<std::uint32_t>(out.vertices.size());
    out.vertices.push_back(applyTransforms(*this, xf,
                                           okay::OkayVertex{
                                               .position = {0, 0, -halfHeight},
                                               .normal = {0, 0, -1},
                                               .uv = {0.5f, 0.5f},
                                           }));

    // Cap ring
    const std::uint32_t capRingBase = static_cast<std::uint32_t>(out.vertices.size());
    for (int col = 0; col < seamColumns; ++col) {
        const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
        const float theta = u * (2.0f * kPi);

        const float cosTheta = std::cos(theta);
        const float sinTheta = std::sin(theta);

        out.vertices.push_back(applyTransforms(
            *this, xf,
            okay::OkayVertex{
                .position =
                    glm::vec3(cosTheta, sinTheta, 0.0f) * radius + glm::vec3(0, 0, -halfHeight),
                .normal = {0, 0, -1},
                .uv = {0.5f + cosTheta * 0.5f, 0.5f + sinTheta * 0.5f},
            }));
    }

    for (int col = 0; col < radialSegments; ++col) {
        const std::uint32_t a = capRingBase + static_cast<std::uint32_t>(col);
        const std::uint32_t b = capRingBase + static_cast<std::uint32_t>(col + 1);
        out.indices.insert(out.indices.end(), {capCenterIndex, b, a});
    }

    return out;
}

okay::OkayMeshData CapsuleBuilder::build(const VertexTransformation<CapsuleBuilder>& xf) const {
    okay::OkayMeshData out;

    const int radialSegments = std::max(3, segments);
    const int hemisphereRings = std::max(2, rings);

    const int seamColumns = radialSegments + 1;
    const int hemisphereRows = hemisphereRings + 1;

    const float radius = radius;
    const float halfTotalHeight = height * 0.5f;
    const float halfCylinderHeight = std::max(0.0f, halfTotalHeight - radius);

    // Cylinder sides
    {
        out.vertices.reserve(static_cast<size_t>(seamColumns * 2));
        out.indices.reserve(static_cast<size_t>(radialSegments * 6));

        for (int col = 0; col < seamColumns; ++col) {
            const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
            const float theta = u * (2.0f * kPi);

            const float cosTheta = std::cos(theta);
            const float sinTheta = std::sin(theta);

            const glm::vec3 radialNormal = {cosTheta, sinTheta, 0.0f};

            const glm::vec3 bottomPos =
                radialNormal * radius + glm::vec3(0, 0, -halfCylinderHeight);
            const glm::vec3 topPos = radialNormal * radius + glm::vec3(0, 0, +halfCylinderHeight);

            out.vertices.push_back(applyTransforms(*this, xf,
                                                   okay::OkayVertex{
                                                       .position = bottomPos,
                                                       .normal = radialNormal,
                                                       .uv = {u, 0.0f},
                                                   }));

            out.vertices.push_back(applyTransforms(*this, xf,
                                                   okay::OkayVertex{
                                                       .position = topPos,
                                                       .normal = radialNormal,
                                                       .uv = {u, 1.0f},
                                                   }));
        }

        const auto SideVertexIndex = [&](int col, int top) -> std::uint32_t {
            return static_cast<std::uint32_t>(col * 2 + top);
        };

        for (int col = 0; col < radialSegments; ++col) {
            const std::uint32_t i00 = SideVertexIndex(col, 0);
            const std::uint32_t i01 = SideVertexIndex(col, 1);
            const std::uint32_t i10 = SideVertexIndex(col + 1, 0);
            const std::uint32_t i11 = SideVertexIndex(col + 1, 1);
            out.indices.insert(out.indices.end(), {i00, i10, i11, i00, i11, i01});
        }
    }

    const auto AppendMesh = [&](const okay::OkayMeshData& mesh) {
        const std::uint32_t baseVertex = static_cast<std::uint32_t>(out.vertices.size());
        out.vertices.insert(out.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
        out.indices.reserve(out.indices.size() + mesh.indices.size());
        for (std::uint32_t i : mesh.indices) out.indices.push_back(baseVertex + i);
    };

    const auto BuildHemisphere = [&](bool topHemisphere) -> okay::OkayMeshData {
        okay::OkayMeshData mesh;
        mesh.vertices.reserve(static_cast<size_t>(seamColumns * hemisphereRows));
        mesh.indices.reserve(static_cast<size_t>(radialSegments * hemisphereRings * 6));

        for (int row = 0; row < hemisphereRows; ++row) {
            const float v = static_cast<float>(row) / static_cast<float>(hemisphereRings);

            // top: 0..pi/2, bottom: pi/2..pi
            const float phi =
                topHemisphere ? (v * (kPi * 0.5f)) : ((kPi * 0.5f) + v * (kPi * 0.5f));

            const float sinPhi = std::sin(phi);
            const float cosPhi = std::cos(phi);

            for (int col = 0; col < seamColumns; ++col) {
                const float u = static_cast<float>(col) / static_cast<float>(radialSegments);
                const float theta = u * (2.0f * kPi);

                const float sinTheta = std::sin(theta);
                const float cosTheta = std::cos(theta);

                const glm::vec3 unitNormal = {cosTheta * sinPhi, sinTheta * sinPhi, cosPhi};
                glm::vec3 localPosition = unitNormal * radius;

                localPosition.z += topHemisphere ? halfCylinderHeight : -halfCylinderHeight;

                mesh.vertices.push_back(applyTransforms(
                    *this, xf,
                    okay::OkayVertex{
                        .position = localPosition,
                        .normal = unitNormal,
                        .uv = {u, topHemisphere ? (1.0f - v) : (1.0f - (0.5f + 0.5f * v))},
                    }));
            }
        }

        const auto VertexIndex = [&](int col, int row) -> std::uint32_t {
            return static_cast<std::uint32_t>(row * seamColumns + col);
        };

        for (int row = 0; row < hemisphereRings; ++row) {
            for (int col = 0; col < radialSegments; ++col) {
                const std::uint32_t i0 = VertexIndex(col, row);
                const std::uint32_t i1 = VertexIndex(col + 1, row);
                const std::uint32_t i2 = VertexIndex(col + 1, row + 1);
                const std::uint32_t i3 = VertexIndex(col, row + 1);

                mesh.indices.insert(mesh.indices.end(), {i0, i1, i2, i0, i2, i3});
            }
        }

        return mesh;
    };

    AppendMesh(BuildHemisphere(true));
    AppendMesh(BuildHemisphere(false));

    return out;
}

}  // namespace okay::primitives
