#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <cstdint>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/okay.hpp>

namespace okay {

enum class ShaderState { NOT_COMPILED, STANDBY, IN_USE };

class OkayShader;
class OkayMaterial;

class OkayShader {
   public:
    OkayShader(const std::string& vertexSource, const std::string& fragmentSource)
        : vertexShader(std::move(vertexSource)),
          fragmentShader(std::move(fragmentSource)),
          _shaderProgram(0),
          _state(ShaderState::NOT_COMPILED),
    {
        std::hash<std::string> hasher;
        _id = hasher(vertexShader + fragmentShader);
    }

    OkayShader()
        : vertexShader(""),
          fragmentShader(""),
          _shaderProgram(0),
          _state(ShaderState::NOT_COMPILED),
          _id(1) {
    }

    std::string vertexShader;
    std::string fragmentShader;

    Failable compile();

    Failable set();

    ShaderState state() const {
        return _state;
    }

    std::uint32_t id() const {
        return _id;
    }

    GLuint programID() const {
        return _shaderProgram;
    }

   private:
    GLuint _shaderProgram;
    ShaderState _state;
    std::size_t _id;
};

class OkayMaterial {
   public:
    std::unique_ptr<IOkayMaterialUniformCollection> uniforms;

    std::uint32_t id() const {
        return _id;
    }
    std::uint32_t shaderID() const {
        return _shader.id();
    }

    void setShader() {
        if (_shader.state() != ShaderState::IN_USE) {
            _shader.set();
        }
    }

    void passUniforms() {
        uniforms->setUniforms(_shader.programID());
    }

   private:
    OkayShader _shader;
    std::size_t _id{0};

    void setMaterial() {
        setShader();
        passUniforms();
    }
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__