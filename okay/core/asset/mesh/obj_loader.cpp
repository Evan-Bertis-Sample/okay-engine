#include <cctype>
#include <okay/core/asset/mesh/obj_loader.hpp>
#include <okay/core/util/string.hpp>
#include <sstream>
#include <string>

using namespace okay;

std::size_t ObjLoader::VertKeyHash::operator()(const VertKey& k) const noexcept {
    // cheap combine
    std::size_t h = 1469598103934665603ull;
    auto mix = [&](int x) {
        h ^= static_cast<std::size_t>(x) + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
    };
    mix(k.v);
    mix(k.vt);
    mix(k.vn);
    return h;
}

Result<OkayMeshData> ObjLoader::Load(const std::filesystem::path& path, std::istream& file) {
    // Good error if it isn't OBJ-ish content
    auto sniff = ValidateLooksLikeObj(file);
    if (sniff.isError()) return Result<OkayMeshData>::errorResult(sniff.error());

    State st;
    st.positions.reserve(1024);
    st.normals.reserve(1024);
    st.uvs.reserve(1024);
    st.out.vertices.reserve(1024);
    st.out.indices.reserve(1024);

    std::string line;
    std::size_t lineNo = 0;

    while (std::getline(file, line)) {
        ++lineNo;
        line = StringUtils::LTrim(std::move(line));
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ls(line);
        std::string head;
        ls >> head;
        if (head.empty()) continue;

        head = StringUtils::ToLower(std::move(head));

        Failable r = Failable::ok({});
        if (head == "v")
            r = ParseV(ls, st);
        else if (head == "vt")
            r = ParseVT(ls, st);
        else if (head == "vn")
            r = ParseVN(ls, st);
        else if (head == "f")
            r = ParseF(ls, st);
        else {
            // ignore: o, g, s, mtllib, usemtl, etc.
            continue;
        }

        if (r.isError()) {
            return Result<OkayMeshData>::errorResult(PrefixLine(lineNo, r.error()));
        }
    }

    if (file.bad()) {
        return Result<OkayMeshData>::errorResult(
            "OBJ parse failed: stream became bad while reading.");
    }

    if (st.out.vertices.empty() || st.out.indices.empty()) {
        return Result<OkayMeshData>::errorResult(
            "OBJ parse produced an empty mesh (no faces found).");
    }

    (void)path;  // reserved for future use (mtl, relative lookups)
    return Result<OkayMeshData>::ok(std::move(st.out));
}

Failable ObjLoader::ValidateLooksLikeObj(std::istream& file) const {
    // If seekable, sniff first meaningful line and restore position.
    const std::streampos pos = file.tellg();

    std::string line;
    while (std::getline(file, line)) {
        line = StringUtils::LTrim(std::move(line));
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ls(line);
        std::string tok;
        ls >> tok;
        tok = StringUtils::ToLower(std::move(tok));

        const bool ok = (tok == "v" || tok == "vt" || tok == "vn" || tok == "f" || tok == "o" ||
                         tok == "g" || tok == "s" || tok == "mtllib" || tok == "usemtl");
        if (!ok) {
            // Restore before returning
            if (pos != std::streampos(-1)) {
                file.clear();
                file.seekg(pos);
            }
            return Failable::errorResult(
                "File does not appear to be an OBJ (unexpected leading token: '" + tok + "').");
        }

        break;  // sniffed successfully
    }

    if (pos != std::streampos(-1)) {
        file.clear();
        file.seekg(pos);
    }
    return Failable::ok({});
}

std::string ObjLoader::PrefixLine(std::size_t lineNo, const std::string& msg) const {
    return "OBJ parse error at line " + std::to_string(lineNo) + ": " + msg;
}

Failable ObjLoader::ParseV(std::istream& ls, State& st) const {
    float x, y, z;
    if (!(ls >> x >> y >> z)) {
        return Failable::errorResult("Malformed 'v' record (expected: v x y z).");
    }
    st.positions.emplace_back(x, y, z);

    // Optional: some OBJ exporters append vertex colors (r g b) after xyz.
    float a, b, c;
    if (ls >> a >> b >> c) {
        st.colors.emplace_back(a, b, c);
    }

    return Failable::ok({});
}

Failable ObjLoader::ParseVT(std::istream& ls, State& st) const {
    float u, v;
    if (!(ls >> u >> v)) {
        return Failable::errorResult("Malformed 'vt' record (expected: vt u v).");
    }
    st.uvs.emplace_back(u, v);
    return Failable::ok({});
}

Failable ObjLoader::ParseVN(std::istream& ls, State& st) const {
    float x, y, z;
    if (!(ls >> x >> y >> z)) {
        return Failable::errorResult("Malformed 'vn' record (expected: vn x y z).");
    }
    st.normals.emplace_back(x, y, z);
    return Failable::ok({});
}

Failable ObjLoader::ParseF(std::istream& ls, State& st) const {
    std::vector<ObjIndex> face;
    face.reserve(8);

    std::string tok;
    while (ls >> tok) {
        ObjIndex idx{};
        if (!ParseObjIndexToken(tok, idx)) {
            return Failable::errorResult("Malformed 'f' token: '" + tok + "'.");
        }
        face.push_back(idx);
    }

    if (face.size() < 3) {
        return Failable::errorResult("Face has fewer than 3 vertices.");
    }

    // Fan triangulate: (0, i, i+1)
    for (std::size_t i = 1; i + 1 < face.size(); ++i) {
        auto a = EmitVertex(face[0], st);
        if (a.isError()) return Failable::errorResult(a.error());
        auto b = EmitVertex(face[i], st);
        if (b.isError()) return Failable::errorResult(b.error());
        auto c = EmitVertex(face[i + 1], st);
        if (c.isError()) return Failable::errorResult(c.error());

        st.out.indices.push_back(a.value());
        st.out.indices.push_back(b.value());
        st.out.indices.push_back(c.value());
    }

    return Failable::ok({});
}

bool ObjLoader::ParseObjIndexToken(std::string_view tok, ObjIndex& out) const {
    // v
    // v/vt
    // v//vn
    // v/vt/vn
    auto parts = StringUtils::Split(tok, '/', /*keepEmpty=*/true);

    int v = 0, vt = 0, vn = 0;
    if (parts.empty() || parts[0].empty() || !ParseInt(parts[0], v)) return false;

    if (parts.size() >= 2 && !parts[1].empty()) {
        if (!ParseInt(parts[1], vt)) return false;
    }
    if (parts.size() >= 3 && !parts[2].empty()) {
        if (!ParseInt(parts[2], vn)) return false;
    }

    out.v = v;
    out.vt = vt;
    out.vn = vn;
    return true;
}

bool ObjLoader::ParseInt(std::string_view s, int& out) const {
    int sign = 1;
    std::size_t i = 0;
    if (i < s.size() && s[i] == '-') {
        sign = -1;
        ++i;
    }

    if (i >= s.size()) return false;

    long val = 0;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        val = val * 10 + (c - '0');
        if (val > 1'000'000'000L) return false;
    }

    out = static_cast<int>(val) * sign;
    return true;
}

int ObjLoader::ResolveIndex(int idx, int count) const {
    // OBJ indices are 1-based; negative indices are relative to end (-1 is last)
    if (idx > 0) return idx - 1;
    if (idx < 0) return count + idx;
    return -1;
}

Result<std::uint32_t> ObjLoader::EmitVertex(const ObjIndex& i, State& st) const {
    const int vCount = static_cast<int>(st.positions.size());
    const int vtCount = static_cast<int>(st.uvs.size());
    const int vnCount = static_cast<int>(st.normals.size());

    const int vi = ResolveIndex(i.v, vCount);
    const int vti = (i.vt != 0) ? ResolveIndex(i.vt, vtCount) : -1;
    const int vni = (i.vn != 0) ? ResolveIndex(i.vn, vnCount) : -1;

    if (vi < 0 || vi >= vCount)
        return Result<std::uint32_t>::errorResult("Position index out of range.");
    if (i.vt != 0 && (vti < 0 || vti >= vtCount))
        return Result<std::uint32_t>::errorResult("UV index out of range.");
    if (i.vn != 0 && (vni < 0 || vni >= vnCount))
        return Result<std::uint32_t>::errorResult("Normal index out of range.");

    VertKey key{vi, vti, vni};
    auto it = st.dedup.find(key);
    if (it != st.dedup.end()) return Result<std::uint32_t>::ok(it->second);

    OkayVertex v{};
    v.position = st.positions[vi];
    v.uv = (vti >= 0) ? st.uvs[vti] : glm::vec2(0.0f, 0.0f);
    v.normal = (vni >= 0) ? st.normals[vni] : glm::vec3(0.0f, 1.0f, 0.0f);
    v.color = (!st.colors.empty() && static_cast<std::size_t>(vi) < st.colors.size())
                  ? st.colors[vi]
                  : glm::vec3(1.0f, 1.0f, 1.0f);

    const std::uint32_t newIndex = static_cast<std::uint32_t>(st.out.vertices.size());
    st.out.vertices.push_back(v);
    st.dedup.emplace(key, newIndex);

    return Result<std::uint32_t>::ok(newIndex);
}
