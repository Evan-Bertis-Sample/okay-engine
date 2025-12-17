#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/asset/mesh/obj_loader.hpp>
#include <okay/core/util/string.hpp>

#include <memory>
#include <string>

using namespace okay;

static std::unique_ptr<MeshLoader> CreateMeshLoaderForExtension(const std::filesystem::path& path) {
    std::string ext = StringUtils::ToLower(path.extension().string());

    if (ext == ".obj") return std::make_unique<ObjLoader>();

    return nullptr;
}

Result<OkayMeshData> OkayAssetLoader<OkayMeshData>::loadAsset(const std::filesystem::path& path,
                                                              std::istream& file) {
    auto loader = CreateMeshLoaderForExtension(path);
    if (!loader) {
        const std::string ext = path.has_extension() ? path.extension().string() : "(no extension)";
        return Result<OkayMeshData>::errorResult(
            "Mesh loading not supported for extension: " + ext + " (only .obj is supported).");
    }

    return loader->Load(path, file);
}
