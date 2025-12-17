#ifndef __OBJ_LOADER_H__
#define __OBJ_LOADER_H__

#include <filesystem>
#include <glm/glm.hpp>
#include <istream>

#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <unordered_map>
#include <vector>

namespace okay {

class ObjLoader final : public MeshLoader {
   public:
    Result<OkayMeshData> Load(const std::filesystem::path& path, std::istream& file) override;

   private:
    struct ObjIndex {
        int v = 0;
        int vt = 0;
        int vn = 0;
    };

    struct VertKey {
        int v = 0;
        int vt = -1;
        int vn = -1;

        bool operator==(const VertKey& o) const { return v == o.v && vt == o.vt && vn == o.vn; }
    };

    struct VertKeyHash {
        std::size_t operator()(const VertKey& k) const noexcept;
    };

    struct State {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> colors;  // optional, if present
        OkayMeshData out;

        std::unordered_map<VertKey, std::uint32_t, VertKeyHash> dedup;
    };

    // record parsers
    Failable ParseV(std::istream& ls, State& st) const;
    Failable ParseVT(std::istream& ls, State& st) const;
    Failable ParseVN(std::istream& ls, State& st) const;
    Failable ParseF(std::istream& ls, State& st) const;

    // face helpers
    bool ParseObjIndexToken(std::string_view tok, ObjIndex& out) const;
    bool ParseInt(std::string_view s, int& out) const;
    int ResolveIndex(int idx, int count) const;
    Result<std::uint32_t> EmitVertex(const ObjIndex& i, State& st) const;

    // sniffing / errors
    Failable ValidateLooksLikeObj(std::istream& file) const;
    std::string PrefixLine(std::size_t lineNo, const std::string& msg) const;
};

}  // namespace okay

#endif  // __OBJ_LOADER_H__