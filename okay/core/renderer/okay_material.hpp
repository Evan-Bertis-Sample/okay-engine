#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <filesystem>
#include <okay/core/util/result.hpp>
#include <string>
#include <unordered_map>

namespace okay {

struct OkayShader {
   public:
    enum ShaderState { NOT_COMPILED, STANDBY, IN_USE };

    OkayShader(const std::string vertexSrc, const std::string fragmentSrc)
        : vertexShader(std::move(vertexSrc)),
          fragmentShader(std::move(fragmentSrc)),
          shaderProgram(0),
          _state(NOT_COMPILED) {
        std::hash<std::string> hasher;
        _id = hasher(vertexShader + fragmentShader);
    }

    OkayShader()
        : vertexShader(""), fragmentShader(""), shaderProgram(0), _state(NOT_COMPILED), _id(1) {}

    std::string vertexShader;
    std::string fragmentShader;

    Failable compile();
    Failable set();
    ShaderState state() const { return _state; }
    std::uint32_t id() const { return _id; }

   private:
    unsigned int shaderProgram;
    ShaderState _state;
    std::size_t _id;
};

class OkayMaterial {
   public:
    std::uint32_t id() const { return 0; }
    std::uint32_t shaderID() const { return _shader.id(); }

   private:
    OkayShader _shader;
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__