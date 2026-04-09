#ifndef __SHADER_LOADER_H__
#define __SHADER_LOADER_H__

#include <okay/core/asset/asset.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/result.hpp>

#include <filesystem>
#include <istream>
#include <regex>
#include <string>

namespace okay {

template <>
struct AssetLoader<Shader> {
    static Result<Shader> loadAsset(const std::filesystem::path& path, const AssetIO& assetIO) {
        // by standard, the fragment shader will be path + .frag
        // and the vertex shader will be path + .vert
        Engine.logger.info("Loading shader: {}", path.string());

        // load vertex shader
        Result<std::unique_ptr<std::istream>> vertexStreamRes =
            assetIO.open(path.string() + ".vert");

        if (vertexStreamRes.isError()) {
            Engine.logger.error("Failed to open vertex shader: {}", path.string());
            return Result<Shader>::errorResult("Failed to open vertex shader: " + path.string());
        }

        // read vertex shader code
        std::unique_ptr<std::istream>& vertexStream = vertexStreamRes.valueRef();
        std::string vertexShaderCode((std::istreambuf_iterator<char>(*vertexStream)),
                                     std::istreambuf_iterator<char>());

        // load fragment shader
        Result<std::unique_ptr<std::istream>> fragmentStreamRes =
            assetIO.open(path.string() + ".frag");

        if (fragmentStreamRes.isError()) {
            Engine.logger.error("Failed to open fragment shader: {}", path.string());
            return Result<Shader>::errorResult("Failed to open fragment shader: " + path.string());
        }

        // read fragment shader code
        std::unique_ptr<std::istream>& fragmentStream = fragmentStreamRes.valueRef();
        std::string fragmentShaderCode((std::istreambuf_iterator<char>(*fragmentStream)),
                                       std::istreambuf_iterator<char>());

        // if this mac, we need to replace the version and remove precision qualifiers since mac's
        // OpenGL doesn't support them
#ifdef __APPLE__
        // replace #version 300 es with #version 330 core and remove precision qualifier statements
        auto replaceVersionAndPrecision = [](std::string& shaderCode) {
            shaderCode =
                std::regex_replace(shaderCode, std::regex("#version 300 es"), "#version 330 core");
            shaderCode = std::regex_replace(
                shaderCode,
                std::regex(
                    R"((?:^|\n)[ \t]*precision\s+(lowp|mediump|highp)\s+(float|int|sampler2D|samplerCube)\s*;\s*)"),
                "\n");
        };

        replaceVersionAndPrecision(vertexShaderCode);
        replaceVersionAndPrecision(fragmentShaderCode);
#endif

        return Result<Shader>::ok(Shader(vertexShaderCode, fragmentShaderCode));
    }
};

}  // namespace okay
#endif  // __SHADER_LOADER_H__