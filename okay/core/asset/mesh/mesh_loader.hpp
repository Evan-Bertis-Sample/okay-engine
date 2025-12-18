#ifndef __MESH_LOADER_H__
#define __MESH_LOADER_H__

#include <filesystem>
#include <istream>

#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/renderer/okay_mesh.hpp>

namespace okay {

class MeshLoader {
   public:
    virtual ~MeshLoader() = default;
    virtual Result<OkayMeshData> Load(const std::filesystem::path& path, std::istream& file) = 0;
};

template <>
struct OkayAssetLoader<OkayMeshData> {
    static Result<OkayMeshData> loadAsset(const std::filesystem::path& path, OkayAssetIO& assetIO);
};

}  // namespace okay

#endif  // __MESH_LOADER_H__