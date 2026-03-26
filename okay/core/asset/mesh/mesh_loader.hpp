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
    virtual Result<MeshData> Load(const std::filesystem::path& path, std::istream& file) = 0;
};

template <>
struct AssetLoader<MeshData> {
    static Result<MeshData> loadAsset(const std::filesystem::path& path,
                                          const AssetIO& assetIO);
};

}  // namespace okay

#endif  // __MESH_LOADER_H__