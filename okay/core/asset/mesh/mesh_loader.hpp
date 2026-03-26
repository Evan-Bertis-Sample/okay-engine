#ifndef __MESH_LOADER_H__
#define __MESH_LOADER_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/util/result.hpp>

#include <filesystem>
#include <istream>

namespace okay {

class MeshLoader {
   public:
    virtual ~MeshLoader() = default;
    virtual Result<OkayMeshData> Load(const std::filesystem::path& path, std::istream& file) = 0;
};

template <>
struct OkayAssetLoader<OkayMeshData> {
    static Result<OkayMeshData> loadAsset(const std::filesystem::path& path,
                                          const OkayAssetIO& assetIO);
};

}  // namespace okay

#endif  // __MESH_LOADER_H__