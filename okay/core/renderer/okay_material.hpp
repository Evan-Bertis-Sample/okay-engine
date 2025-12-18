#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <filesystem>
#include <okay/core/util/result.hpp>
#include <string>

namespace okay {

struct OkayShader {
   public:
    enum ShaderState { NOT_COMPILED, STANDBY, IN_USE };

    OkayShader(const std::string vertexSrc, const std::string fragmentSrc)
        : vertexShader(std::move(vertexSrc)),
          fragmentShader(std::move(fragmentSrc)),
          shaderProgram(0),
          _state(NOT_COMPILED) {}

    OkayShader() : vertexShader(""), fragmentShader(""), shaderProgram(0), _state(NOT_COMPILED) {}

    const std::string vertexShader;
    const std::string fragmentShader;

    Failable compile() {
        // dummy compile
        _state = STANDBY;
        return Failable::ok({});
    }

    Failable set() { return Failable::ok({}); }
    ShaderState state() const { return _state; }

   private:
    unsigned int shaderProgram;

    ShaderState _state;
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__