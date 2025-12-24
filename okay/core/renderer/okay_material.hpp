#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <cstdint>
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/util/result.hpp>

namespace okay {

enum class ShaderState { NOT_COMPILED, STANDBY, IN_USE };

template <class UniformCollection>
class OkayShader;

template <class UniformCollection>
class OkayMaterial;

template <class... Uniforms>
struct OkayShader<OkayMaterialUniformCollection<Uniforms...>> {
   public:
    OkayShader(const std::string vertexSrc, const std::string fragmentSrc)
        : vertexShader(std::move(vertexSrc)),
          fragmentShader(std::move(fragmentSrc)),
          _shaderProgram(0),
          _state(ShaderState::NOT_COMPILED) {
        std::hash<std::string> hasher;
        _id = hasher(vertexShader + fragmentShader);

        
    } 

    OkayShader()
        : vertexShader(""),
          fragmentShader(""),
          _shaderProgram(0),
          _state(ShaderState::NOT_COMPILED),
          _id(1) {}

    std::string vertexShader;
    std::string fragmentShader;
    OkayMaterialUniformCollection<Uniforms...> uniforms;

    Failable compile() {
        // Compile vertex shader
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexSrcCStr = vertexShader.c_str();
        glShaderSource(vertex, 1, &vertexSrcCStr, NULL);
        glCompileShader(vertex);

        // Check for compilation errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            return Failable::errorResult("Vertex shader compilation failed: " +
                                         std::string(infoLog));
        }

        // Compile fragment shader
        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fragmentSrcCStr = fragmentShader.c_str();
        glShaderSource(fragment, 1, &fragmentSrcCStr, NULL);
        glCompileShader(fragment);

        // Check for compilation errors
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            return Failable::errorResult("Fragment shader compilation failed: " +
                                         std::string(infoLog));
        }

        // Link shaders into a program
        _shaderProgram = glCreateProgram();
        glAttachShader(_shaderProgram, vertex);
        glAttachShader(_shaderProgram, fragment);
        glLinkProgram(_shaderProgram);

        // Check for linking errors
        glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(_shaderProgram, 512, NULL, infoLog);
            return Failable::errorResult("Shader program linking failed: " + std::string(infoLog));
        }

        // Clean up shaders as they're linked into the program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        _state = ShaderState::STANDBY;
        return Failable::ok({});
    }

    Failable set() {
        if (_state != ShaderState::STANDBY && _state != ShaderState::IN_USE) {
            return Failable::errorResult("Shader must be compiled before setting it for use.");
        }
        glUseProgram(_shaderProgram);
        _state = ShaderState::IN_USE;
        uniforms.markAllDirty();
        return Failable::ok({});
    }

    Failable findUniformLocations() { return uniforms.findLocations(_shaderProgram); };

    Failable passDirtyUniforms() { return uniforms.setUniforms(_shaderProgram); };

    ShaderState state() const { return _state; }
    std::uint32_t id() const { return _id; }

   private:
    GLuint _shaderProgram;
    ShaderState _state;
    std::size_t _id;
};

class IOkayMaterial {
   public:
    virtual std::uint32_t id() const { return 0; }
    virtual std::uint32_t shaderID() const { return 0; }
    virtual void setShader() {}
    virtual void passUniforms() {}
};

// template takes in a OkayMaterialUniformCollection<...>
template <class... Uniforms>
class OkayMaterial<OkayMaterialUniformCollection<Uniforms...>> : public IOkayMaterial {
   public:
    OkayMaterialUniformCollection<Uniforms...> uniforms;

    OkayMaterial(const OkayShader<Uniforms...>& shader) : _shader(shader), uniforms() {
        // hash the names of the uniforms + shader id to create a unique material id
        std::hash<std::string> hasher;
        std::string combined;
        ((combined += std::string(Uniforms::name.sv()) + ";"), ...);
        combined += std::to_string(shader.id());
        _id = hasher(combined);
    }

    std::uint32_t id() const override { return _id; }
    std::uint32_t shaderID() const override { return _shader.id(); }

    void setShader() override {
        if (_shader.state() != ShaderState::IN_USE) {
            _shader.set();
        }
    }

    void passUniforms() override { uniforms.setUniforms(); }

   private:
    OkayShader<Uniforms...> _shader;
    std::size_t _id{0};
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__