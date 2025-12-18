#ifndef __SHADER_LOADER_H__
#define __SHADER_LOADER_H__

#include <filesystem>
#include <istream>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/util/result.hpp>
#include <string>

namespace okay {

template <>
struct OkayAssetLoader<OkayShader> {
    static Result<OkayShader> loadAsset(const std::filesystem::path& path,
                                        const OkayAssetIO& assetIO) {
                                            
        // by standard, the fragment shader will be path + .frag
        // and the vertex shader will be path + .vert

        // load vertex shader
        Result<std::unique_ptr<std::istream>> vertexStreamRes =
            assetIO.open(path.string() + ".vert");

        if (vertexStreamRes.isError()) {
            return Result<OkayShader>::errorResult("Failed to open vertex shader: " + path.string());
        }

        // read vertex shader code
        std::unique_ptr<std::istream> vertexStream = vertexStreamRes.take();
        std::string vertexShaderCode((std::istreambuf_iterator<char>(*vertexStream)),
                                    std::istreambuf_iterator<char>());

        // load fragment shader
        Result<std::unique_ptr<std::istream>> fragmentStreamRes =
            assetIO.open(path.string() + ".frag");

        if (fragmentStreamRes.isError()) {
            return Result<OkayShader>::errorResult("Failed to open fragment shader: " + path.string());
        }

        // read fragment shader code
        std::unique_ptr<std::istream> fragmentStream = fragmentStreamRes.take();
        std::string fragmentShaderCode((std::istreambuf_iterator<char>(*fragmentStream)),
                                       std::istreambuf_iterator<char>());

        return Result<OkayShader>::ok(OkayShader(vertexShaderCode, fragmentShaderCode));
    }
};

}  // namespace okay
#endif  // __SHADER_LOADER_H__