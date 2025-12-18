#include <memory>
#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/asset/mesh/obj_loader.hpp>
#include <okay/core/util/string.hpp>
#include <string>

using namespace okay;

static std::unique_ptr<MeshLoader> CreateMeshLoaderForExtension(const std::filesystem::path& path) {
    std::string ext = StringUtils::ToLower(path.extension().string());

    if (ext == ".obj") return std::make_unique<ObjLoader>();

    return nullptr;
}

Result<OkayMeshData> OkayAssetLoader<OkayMeshData>::loadAsset(
    const std::filesystem::path& path,
    const OkayAssetIO& assetIO) {
    auto loader = CreateMeshLoaderForExtension(path);
    if (!loader) {
        const std::string ext = path.has_extension() ? path.extension().string() : "(no extension)";
        return Result<OkayMeshData>::errorResult(
            "Mesh loading not supported for extension: " + ext + " (only .obj is supported).");
    }

    Result<std::unique_ptr<std::istream>> fileRes = assetIO.open(path);
    if (fileRes.isError()) {
        return Result<OkayMeshData>::errorResult(
            "Failed to open mesh file: " + path.string() + ": " + fileRes.error());
    }
    
    std::unique_ptr<std::istream> file = fileRes.take();

    return loader->Load(path, *file);
}
