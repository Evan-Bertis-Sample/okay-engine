#include "okay_primitive.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace okay {

static constexpr float kPi = 3.14159265358979323846f;

static inline glm::vec3 RotateVec(const glm::quat& q, const glm::vec3& v) {
    return q * v;
}

void OkayPrimitives::ApplyTransform(OkayVertex& v, const glm::vec3& center, const glm::quat& rot) {
    v.Position = RotateVec(rot, v.Position) + center;
    v.Normal = glm::normalize(RotateVec(rot, v.Normal));
}

OkayMeshData OkayPrimitives::Rect(const RectOptions& opt,
                                  const VertexTransformation<RectOptions>& xf) {
    OkayMeshData out;
    out.Vertices.reserve(opt.TwoSided ? 8 : 4);
    out.Indices.reserve(opt.TwoSided ? 12 : 6);

    const float hx = opt.Size.x * 0.5f;
    const float hy = opt.Size.y * 0.5f;

    // Local quad in XY plane, +Z normal
    auto makeV = [&](float x, float y, float u, float v) {
        OkayVertex ov{};
        ov.Position = {x, y, 0.0f};
        ov.Normal = {0.0f, 0.0f, 1.0f};
        ov.UV = {u, v};
        ov.Color = {1.0f, 1.0f, 1.0f};
        ApplyTransform(ov, opt.Center, opt.Rotation);
        ApplyUserXform(ov, opt, xf);
        return ov;
    };

    // Winding CCW for +Z
    out.Vertices.push_back(makeV(-hx, -hy, 0.0f, 0.0f));  // 0
    out.Vertices.push_back(makeV(hx, -hy, 1.0f, 0.0f));   // 1
    out.Vertices.push_back(makeV(hx, hy, 1.0f, 1.0f));    // 2
    out.Vertices.push_back(makeV(-hx, hy, 0.0f, 1.0f));   // 3

    out.Indices.insert(out.Indices.end(), {0, 1, 2, 0, 2, 3});

    if (opt.TwoSided) {
        // Duplicate with flipped normal and winding
        auto makeVB = [&](const OkayVertex& src) {
            OkayVertex vb = src;
            vb.Normal = -vb.Normal;
            ApplyUserXform(vb, opt, xf);
            return vb;
        };

        const std::uint32_t base = 4;
        out.Vertices.push_back(makeVB(out.Vertices[0]));
        out.Vertices.push_back(makeVB(out.Vertices[1]));
        out.Vertices.push_back(makeVB(out.Vertices[2]));
        out.Vertices.push_back(makeVB(out.Vertices[3]));

        // back face (reverse winding)
        out.Indices.insert(out.Indices.end(),
                           {base + 0, base + 2, base + 1, base + 0, base + 3, base + 2});
    }

    return out;
}

OkayMeshData OkayPrimitives::Plane(const PlaneOptions& opt,
                                   const VertexTransformation<PlaneOptions>& xf) {
    OkayMeshData out;

    const int sx = std::max(1, opt.Segments.x);
    const int sy = std::max(1, opt.Segments.y);

    const int vx = sx + 1;
    const int vy = sy + 1;

    out.Vertices.reserve(static_cast<size_t>(vx * vy) * (opt.TwoSided ? 2 : 1));
    out.Indices.reserve(static_cast<size_t>(sx * sy) * 6 * (opt.TwoSided ? 2 : 1));

    const float hx = opt.Size.x * 0.5f;
    const float hy = opt.Size.y * 0.5f;

    auto pushGrid = [&](float normalSign) {
        const glm::vec3 n = {0.0f, 0.0f, normalSign};

        const std::uint32_t base = static_cast<std::uint32_t>(out.Vertices.size());

        for (int y = 0; y < vy; ++y) {
            const float ty = static_cast<float>(y) / static_cast<float>(sy);
            const float py = -hy + ty * opt.Size.y;

            for (int x = 0; x < vx; ++x) {
                const float tx = static_cast<float>(x) / static_cast<float>(sx);
                const float px = -hx + tx * opt.Size.x;

                OkayVertex v{};
                v.Position = {px, py, 0.0f};
                v.Normal = n;
                v.UV = {tx, ty};
                v.Color = {1.0f, 1.0f, 1.0f};

                ApplyTransform(v, opt.Center, opt.Rotation);
                ApplyUserXform(v, opt, xf);

                out.Vertices.push_back(v);
            }
        }

        for (int y = 0; y < sy; ++y) {
            for (int x = 0; x < sx; ++x) {
                const std::uint32_t i0 = base + (y * vx + x);
                const std::uint32_t i1 = base + (y * vx + x + 1);
                const std::uint32_t i2 = base + ((y + 1) * vx + x + 1);
                const std::uint32_t i3 = base + ((y + 1) * vx + x);

                if (normalSign > 0.0f) {
                    out.Indices.insert(out.Indices.end(), {i0, i1, i2, i0, i2, i3});
                } else {
                    // reverse winding
                    out.Indices.insert(out.Indices.end(), {i0, i2, i1, i0, i3, i2});
                }
            }
        }
    };

    pushGrid(+1.0f);
    if (opt.TwoSided) pushGrid(-1.0f);

    return out;
}

OkayMeshData OkayPrimitives::Box(const BoxOptions& opt,
                                 const VertexTransformation<BoxOptions>& xf) {
    OkayMeshData out;
    out.Vertices.reserve(24);
    out.Indices.reserve(36);

    const glm::vec3 h = opt.Size * 0.5f;

    struct Face {
        glm::vec3 n;
        glm::vec3 a, b, c, d;  // CCW quad in local space
    };

    const Face faces[6] = {
        // +Z
        {{0, 0, 1}, {-h.x, -h.y, h.z}, {h.x, -h.y, h.z}, {h.x, h.y, h.z}, {-h.x, h.y, h.z}},
        // -Z
        {{0, 0, -1}, {h.x, -h.y, -h.z}, {-h.x, -h.y, -h.z}, {-h.x, h.y, -h.z}, {h.x, h.y, -h.z}},
        // +X
        {{1, 0, 0}, {h.x, -h.y, h.z}, {h.x, -h.y, -h.z}, {h.x, h.y, -h.z}, {h.x, h.y, h.z}},
        // -X
        {{-1, 0, 0}, {-h.x, -h.y, -h.z}, {-h.x, -h.y, h.z}, {-h.x, h.y, h.z}, {-h.x, h.y, -h.z}},
        // +Y
        {{0, 1, 0}, {-h.x, h.y, h.z}, {h.x, h.y, h.z}, {h.x, h.y, -h.z}, {-h.x, h.y, -h.z}},
        // -Y
        {{0, -1, 0}, {-h.x, -h.y, -h.z}, {h.x, -h.y, -h.z}, {h.x, -h.y, h.z}, {-h.x, -h.y, h.z}},
    };

    for (int f = 0; f < 6; ++f) {
        const std::uint32_t base = static_cast<std::uint32_t>(out.Vertices.size());
        const auto& face = faces[f];

        auto push = [&](glm::vec3 p, glm::vec2 uv) {
            OkayVertex v{};
            v.Position = p;
            v.Normal = face.n;
            v.UV = uv;
            v.Color = {1, 1, 1};
            ApplyTransform(v, opt.Center, opt.Rotation);
            ApplyUserXform(v, opt, xf);
            out.Vertices.push_back(v);
        };

        push(face.a, {0, 0});
        push(face.b, {1, 0});
        push(face.c, {1, 1});
        push(face.d, {0, 1});

        out.Indices.insert(out.Indices.end(),
                           {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3});
    }

    return out;
}

OkayMeshData OkayPrimitives::UVSphere(const UVSphereOptions& opt,
                                      const VertexTransformation<UVSphereOptions>& xf) {
    OkayMeshData out;

    const int seg = std::max(3, opt.Segments);
    const int ring = std::max(2, opt.Rings);

    // We create (ring+1) rows including poles, and (seg+1) columns including seam duplicate
    const int cols = seg + 1;
    const int rows = ring + 1;

    out.Vertices.reserve(static_cast<size_t>(cols * rows));
    out.Indices.reserve(static_cast<size_t>(seg * ring * 6));

    for (int y = 0; y < rows; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(ring);  // 0..1
        const float phi = v * kPi;                                         // 0..pi

        const float sp = std::sin(phi);
        const float cp = std::cos(phi);

        for (int x = 0; x < cols; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(seg);  // 0..1
            const float theta = u * (2.0f * kPi);                             // 0..2pi

            const float st = std::sin(theta);
            const float ct = std::cos(theta);

            glm::vec3 n = {ct * sp, st * sp, cp};
            glm::vec3 p = n * opt.Radius;

            OkayVertex vert{};
            vert.Position = p;
            vert.Normal = glm::normalize(n);
            vert.UV = {u, 1.0f - v};
            vert.Color = {1, 1, 1};

            ApplyTransform(vert, opt.Center, opt.Rotation);
            ApplyUserXform(vert, opt, xf);

            out.Vertices.push_back(vert);
        }
    }

    auto idx = [&](int x, int y) -> std::uint32_t {
        return static_cast<std::uint32_t>(y * cols + x);
    };

    for (int y = 0; y < ring; ++y) {
        for (int x = 0; x < seg; ++x) {
            const std::uint32_t i0 = idx(x, y);
            const std::uint32_t i1 = idx(x + 1, y);
            const std::uint32_t i2 = idx(x + 1, y + 1);
            const std::uint32_t i3 = idx(x, y + 1);

            out.Indices.insert(out.Indices.end(), {i0, i1, i2, i0, i2, i3});
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
        // 64-bit mix
        return (static_cast<std::size_t>(k.a) << 32) ^ static_cast<std::size_t>(k.b);
    }
};

static std::uint32_t GetMidpoint(std::vector<glm::vec3>& positions,
                                 std::unordered_map<EdgeKey, std::uint32_t, EdgeKeyHash>& cache,
                                 std::uint32_t i0, std::uint32_t i1) {
    const std::uint32_t a = std::min(i0, i1);
    const std::uint32_t b = std::max(i0, i1);
    EdgeKey key{a, b};

    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    glm::vec3 mid = glm::normalize((positions[a] + positions[b]) * 0.5f);
    const std::uint32_t idx = static_cast<std::uint32_t>(positions.size());
    positions.push_back(mid);
    cache.emplace(key, idx);
    return idx;
}

OkayMeshData OkayPrimitives::IcoSphere(const IcoSphereOptions& opt,
                                       const VertexTransformation<IcoSphereOptions>& xf) {
    OkayMeshData out;

    const int sub = std::max(0, opt.Subdivisions);

    // Build icosahedron
    std::vector<glm::vec3> pos;
    std::vector<std::uint32_t> idx;

    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;

    pos = {
        glm::normalize(glm::vec3{-1, t, 0}),  glm::normalize(glm::vec3{1, t, 0}),
        glm::normalize(glm::vec3{-1, -t, 0}), glm::normalize(glm::vec3{1, -t, 0}),
        glm::normalize(glm::vec3{0, -1, t}),  glm::normalize(glm::vec3{0, 1, t}),
        glm::normalize(glm::vec3{0, -1, -t}), glm::normalize(glm::vec3{0, 1, -t}),
        glm::normalize(glm::vec3{t, 0, -1}),  glm::normalize(glm::vec3{t, 0, 1}),
        glm::normalize(glm::vec3{-t, 0, -1}), glm::normalize(glm::vec3{-t, 0, 1}),
    };

    idx = {0, 11, 5,  0, 5,  1, 0, 1, 7, 0, 7,  10, 0, 10, 11, 1, 5, 9, 5, 11,
           4, 11, 10, 2, 10, 7, 6, 7, 1, 8, 3,  9,  4, 3,  4,  2, 3, 2, 6, 3,
           6, 8,  3,  8, 9,  4, 9, 5, 2, 4, 11, 6,  2, 10, 8,  6, 7, 9, 8, 1};

    // Subdivide
    for (int s = 0; s < sub; ++s) {
        std::unordered_map<EdgeKey, std::uint32_t, EdgeKeyHash> cache;
        std::vector<std::uint32_t> idx2;
        idx2.reserve(idx.size() * 4);

        for (size_t i = 0; i < idx.size(); i += 3) {
            std::uint32_t i0 = idx[i + 0];
            std::uint32_t i1 = idx[i + 1];
            std::uint32_t i2 = idx[i + 2];

            const std::uint32_t a = GetMidpoint(pos, cache, i0, i1);
            const std::uint32_t b = GetMidpoint(pos, cache, i1, i2);
            const std::uint32_t c = GetMidpoint(pos, cache, i2, i0);

            idx2.insert(idx2.end(), {i0, a, c, i1, b, a, i2, c, b, a, b, c});
        }

        idx.swap(idx2);
    }

    // Emit vertices (positions are unit sphere, compute normals from them)
    out.Vertices.reserve(pos.size());
    out.Indices = idx;

    for (const auto& p_unit : pos) {
        OkayVertex v{};
        v.Position = p_unit * opt.Radius;
        v.Normal = p_unit;
        // UVs for icosphere are non-trivial; give a fallback spherical projection
        float u = std::atan2(p_unit.y, p_unit.x) / (2.0f * kPi) + 0.5f;
        float vv = std::acos(std::clamp(p_unit.z, -1.0f, 1.0f)) / kPi;
        v.UV = {u, 1.0f - vv};
        v.Color = {1, 1, 1};

        ApplyTransform(v, opt.Center, opt.Rotation);
        ApplyUserXform(v, opt, xf);

        out.Vertices.push_back(v);
    }

    return out;
}

OkayMeshData OkayPrimitives::Cylinder(const CylinderOptions& opt,
                                      const VertexTransformation<CylinderOptions>& xf) {
    OkayMeshData out;

    const int seg = std::max(3, opt.Segments);
    const float h = opt.Height * 0.5f;

    // Side vertices: (seg+1)*2 (seam duplicate)
    const int cols = seg + 1;
    const std::uint32_t baseSide = 0;

    out.Vertices.reserve(static_cast<size_t>(cols * 2) + (opt.Caps ? (2 + cols * 2) : 0));
    out.Indices.reserve(static_cast<size_t>(seg * 6) +
                        (opt.Caps ? static_cast<size_t>(seg * 6) : 0));

    // Side
    for (int i = 0; i < cols; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(seg);
        const float t = u * (2.0f * kPi);
        const float ct = std::cos(t);
        const float st = std::sin(t);

        glm::vec3 n = {ct, st, 0.0f};
        glm::vec3 p0 = n * opt.Radius + glm::vec3(0, 0, -h);
        glm::vec3 p1 = n * opt.Radius + glm::vec3(0, 0, h);

        OkayVertex v0{}, v1{};
        v0.Position = p0;
        v0.Normal = glm::normalize(n);
        v0.UV = {u, 0.0f};
        v0.Color = {1, 1, 1};
        v1.Position = p1;
        v1.Normal = glm::normalize(n);
        v1.UV = {u, 1.0f};
        v1.Color = {1, 1, 1};

        ApplyTransform(v0, opt.Center, opt.Rotation);
        ApplyTransform(v1, opt.Center, opt.Rotation);
        ApplyUserXform(v0, opt, xf);
        ApplyUserXform(v1, opt, xf);

        out.Vertices.push_back(v0);
        out.Vertices.push_back(v1);
    }

    auto vSide = [&](int i, int top) -> std::uint32_t {
        return baseSide + static_cast<std::uint32_t>(i * 2 + top);
    };

    for (int i = 0; i < seg; ++i) {
        const std::uint32_t i00 = vSide(i, 0);
        const std::uint32_t i01 = vSide(i, 1);
        const std::uint32_t i10 = vSide(i + 1, 0);
        const std::uint32_t i11 = vSide(i + 1, 1);

        out.Indices.insert(out.Indices.end(), {i00, i10, i11, i00, i11, i01});
    }

    if (!opt.Caps) return out;

    // Caps: center + ring (with cap normals)
    const std::uint32_t baseCaps = static_cast<std::uint32_t>(out.Vertices.size());

    // bottom center
    {
        OkayVertex c{};
        c.Position = {0, 0, -h};
        c.Normal = {0, 0, -1};
        c.UV = {0.5f, 0.5f};
        c.Color = {1, 1, 1};
        ApplyTransform(c, opt.Center, opt.Rotation);
        ApplyUserXform(c, opt, xf);
        out.Vertices.push_back(c);
    }

    // top center
    {
        OkayVertex c{};
        c.Position = {0, 0, h};
        c.Normal = {0, 0, 1};
        c.UV = {0.5f, 0.5f};
        c.Color = {1, 1, 1};
        ApplyTransform(c, opt.Center, opt.Rotation);
        ApplyUserXform(c, opt, xf);
        out.Vertices.push_back(c);
    }

    const std::uint32_t bottomCenter = baseCaps + 0;
    const std::uint32_t topCenter = baseCaps + 1;

    // bottom ring
    const std::uint32_t bottomRing = static_cast<std::uint32_t>(out.Vertices.size());
    for (int i = 0; i < cols; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(seg);
        const float t = u * (2.0f * kPi);
        const float ct = std::cos(t);
        const float st = std::sin(t);

        glm::vec3 p = glm::vec3(ct, st, 0.0f) * opt.Radius + glm::vec3(0, 0, -h);

        OkayVertex v{};
        v.Position = p;
        v.Normal = {0, 0, -1};
        v.UV = {0.5f + ct * 0.5f, 0.5f + st * 0.5f};
        v.Color = {1, 1, 1};

        ApplyTransform(v, opt.Center, opt.Rotation);
        ApplyUserXform(v, opt, xf);
        out.Vertices.push_back(v);
    }

    // top ring
    const std::uint32_t topRing = static_cast<std::uint32_t>(out.Vertices.size());
    for (int i = 0; i < cols; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(seg);
        const float t = u * (2.0f * kPi);
        const float ct = std::cos(t);
        const float st = std::sin(t);

        glm::vec3 p = glm::vec3(ct, st, 0.0f) * opt.Radius + glm::vec3(0, 0, h);

        OkayVertex v{};
        v.Position = p;
        v.Normal = {0, 0, 1};
        v.UV = {0.5f + ct * 0.5f, 0.5f + st * 0.5f};
        v.Color = {1, 1, 1};

        ApplyTransform(v, opt.Center, opt.Rotation);
        ApplyUserXform(v, opt, xf);
        out.Vertices.push_back(v);
    }

    // cap indices
    for (int i = 0; i < seg; ++i) {
        // bottom (winding so normal points -Z)
        const std::uint32_t a = bottomRing + i;
        const std::uint32_t b = bottomRing + i + 1;
        out.Indices.insert(out.Indices.end(), {bottomCenter, b, a});

        // top (+Z)
        const std::uint32_t c = topRing + i;
        const std::uint32_t d = topRing + i + 1;
        out.Indices.insert(out.Indices.end(), {topCenter, c, d});
    }

    return out;
}

OkayMeshData OkayPrimitives::Cone(const ConeOptions& opt,
                                  const VertexTransformation<ConeOptions>& xf) {
    OkayMeshData out;

    const int seg = std::max(3, opt.Segments);
    const int cols = seg + 1;
    const float h = opt.Height * 0.5f;

    out.Vertices.reserve(static_cast<size_t>(cols + 1) + (opt.Cap ? (cols + 1) : 0));
    out.Indices.reserve(static_cast<size_t>(seg * 3 * 2));

    const glm::vec3 apexLocal = {0, 0, h};

    // Side ring + apex
    const std::uint32_t baseSide = 0;

    // ring
    for (int i = 0; i < cols; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(seg);
        const float t = u * (2.0f * kPi);
        const float ct = std::cos(t);
        const float st = std::sin(t);

        glm::vec3 p = glm::vec3(ct, st, 0.0f) * opt.Radius + glm::vec3(0, 0, -h);

        // Approx normal: perpendicular to generatrix
        glm::vec3 n = glm::normalize(glm::vec3(ct, st, opt.Radius / opt.Height));

        OkayVertex v{};
        v.Position = p;
        v.Normal = n;
        v.UV = {u, 0.0f};
        v.Color = {1, 1, 1};

        ApplyTransform(v, opt.Center, opt.Rotation);
        ApplyUserXform(v, opt, xf);
        out.Vertices.push_back(v);
    }

    // apex
    const std::uint32_t apex = static_cast<std::uint32_t>(out.Vertices.size());
    {
        OkayVertex v{};
        v.Position = apexLocal;
        v.Normal = {0, 0, 1};  // not perfect; ok for now
        v.UV = {0.5f, 1.0f};
        v.Color = {1, 1, 1};

        ApplyTransform(v, opt.Center, opt.Rotation);
        ApplyUserXform(v, opt, xf);
        out.Vertices.push_back(v);
    }

    // side indices
    for (int i = 0; i < seg; ++i) {
        const std::uint32_t a = baseSide + i;
        const std::uint32_t b = baseSide + i + 1;
        out.Indices.insert(out.Indices.end(), {a, b, apex});
    }

    if (!opt.Cap) return out;

    // Cap: center + ring with -Z normal
    const std::uint32_t capCenter = static_cast<std::uint32_t>(out.Vertices.size());
    {
        OkayVertex c{};
        c.Position = {0, 0, -h};
        c.Normal = {0, 0, -1};
        c.UV = {0.5f, 0.5f};
        c.Color = {1, 1, 1};
        ApplyTransform(c, opt.Center, opt.Rotation);
        ApplyUserXform(c, opt, xf);
        out.Vertices.push_back(c);
    }

    const std::uint32_t capRing = static_cast<std::uint32_t>(out.Vertices.size());
    for (int i = 0; i < cols; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(seg);
        const float t = u * (2.0f * kPi);
        const float ct = std::cos(t);
        const float st = std::sin(t);

        glm::vec3 p = glm::vec3(ct, st, 0.0f) * opt.Radius + glm::vec3(0, 0, -h);

        OkayVertex v{};
        v.Position = p;
        v.Normal = {0, 0, -1};
        v.UV = {0.5f + ct * 0.5f, 0.5f + st * 0.5f};
        v.Color = {1, 1, 1};

        ApplyTransform(v, opt.Center, opt.Rotation);
        ApplyUserXform(v, opt, xf);
        out.Vertices.push_back(v);
    }

    for (int i = 0; i < seg; ++i) {
        const std::uint32_t a = capRing + i;
        const std::uint32_t b = capRing + i + 1;
        out.Indices.insert(out.Indices.end(), {capCenter, b, a});
    }

    return out;
}

OkayMeshData OkayPrimitives::Capsule(const CapsuleOptions& opt,
                                     const VertexTransformation<CapsuleOptions>& xf) {
    OkayMeshData out;

    const int seg = std::max(3, opt.Segments);
    const int rings = std::max(2, opt.Rings);
    const float radius = opt.Radius;

    const float halfTotal = opt.Height * 0.5f;
    const float halfCyl = std::max(0.0f, halfTotal - radius);

    // We build:
    // - cylinder side (no caps)
    // - top hemisphere (rings)
    // - bottom hemisphere (rings)
    //
    // Seam duplicates included for clean UVs.

    // Cylinder (no caps)
    {
        CylinderOptions copt;
        copt.Center = opt.Center;
        copt.Radius = radius;
        copt.Height = halfCyl * 2.0f;
        copt.Segments = seg;
        copt.Caps = false;
        copt.Rotation = opt.Rotation;

        OkayMeshData cyl = Cylinder(copt, nullptr);
        // shift cylinder to center (already centered), but we need to place hemispheres too.
        // We'll append it now; indices will be adjusted later.
        out = std::move(cyl);
    }

    auto appendMesh = [&](const OkayMeshData& m) {
        const std::uint32_t base = static_cast<std::uint32_t>(out.Vertices.size());
        out.Vertices.insert(out.Vertices.end(), m.Vertices.begin(), m.Vertices.end());
        out.Indices.reserve(out.Indices.size() + m.Indices.size());
        for (std::uint32_t i : m.Indices) out.Indices.push_back(base + i);
    };

    // Hemisphere helper: uses UVSphere section, but only half the rings.
    auto hemisphere = [&](bool top) -> OkayMeshData {
        OkayMeshData m;
        const int cols = seg + 1;
        const int rows = rings + 1;

        m.Vertices.reserve(static_cast<size_t>(cols * rows));
        m.Indices.reserve(static_cast<size_t>(seg * rings * 6));

        // phi: 0..pi/2 for top, pi/2..pi for bottom (in local hemisphere frame)
        for (int y = 0; y < rows; ++y) {
            const float v = static_cast<float>(y) / static_cast<float>(rings);
            float phi = 0.0f;
            if (top) {
                phi = v * (kPi * 0.5f);
            } else {
                phi = (kPi * 0.5f) + v * (kPi * 0.5f);
            }

            const float sp = std::sin(phi);
            const float cp = std::cos(phi);

            for (int x = 0; x < cols; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(seg);
                const float theta = u * (2.0f * kPi);
                const float st = std::sin(theta);
                const float ct = std::cos(theta);

                glm::vec3 n = {ct * sp, st * sp, cp};
                glm::vec3 p = n * radius;

                // Move hemisphere so its equator meets cylinder
                p.z += top ? halfCyl : -halfCyl;

                OkayVertex vtx{};
                vtx.Position = p;
                vtx.Normal = glm::normalize(n);
                vtx.UV = {u, top ? (1.0f - v) : (1.0f - (0.5f + 0.5f * v))};
                vtx.Color = {1, 1, 1};

                ApplyTransform(vtx, opt.Center, opt.Rotation);
                ApplyUserXform(vtx, opt, xf);

                m.Vertices.push_back(vtx);
            }
        }

        auto idx = [&](int x, int y) -> std::uint32_t {
            return static_cast<std::uint32_t>(y * cols + x);
        };

        for (int y = 0; y < rings; ++y) {
            for (int x = 0; x < seg; ++x) {
                const std::uint32_t i0 = idx(x, y);
                const std::uint32_t i1 = idx(x + 1, y);
                const std::uint32_t i2 = idx(x + 1, y + 1);
                const std::uint32_t i3 = idx(x, y + 1);
                m.Indices.insert(m.Indices.end(), {i0, i1, i2, i0, i2, i3});
            }
        }

        return m;
    };

    // Append hemispheres
    appendMesh(hemisphere(true));
    appendMesh(hemisphere(false));

    // Apply user transform to cylinder vertices too (so Capsule transform affects all)
    if (xf) {
        for (auto& v : out.Vertices) {
            // We already applied opt center/rot in Cylinder() call;
            // only apply user's xf here so it affects the full capsule.
            v = xf(v, opt);
        }
    }

    return out;
}

}  // namespace okay
