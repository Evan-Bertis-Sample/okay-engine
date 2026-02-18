#include "okay_material.hpp"

namespace okay {

Failable OkayShader::compile() {
    if (_state != ShaderState::NOT_COMPILED) {
        Engine.logger.warn("Shader is already compiled.");
        return Failable::ok({});
    }

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
        return Failable::errorResult("Vertex shader compilation failed: " + std::string(infoLog));
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
        return Failable::errorResult("Fragment shader compilation failed: " + std::string(infoLog));
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

    Engine.logger.info("Shader compiled.");
    return Failable::ok({});
}

Failable OkayShader::set() {
    if (_state != ShaderState::STANDBY && _state != ShaderState::IN_USE) {
        return Failable::errorResult("Shader must be compiled before setting it for use.");
    }
    glUseProgram(_shaderProgram);
    _state = ShaderState::IN_USE;
    return Failable::ok({});
}

OkayMaterial& OkayMaterialHandle::operator*() const {
    return *owner->getMaterial(id);
}

OkayMaterial& OkayMaterialHandle::operator->() const {
    return *owner->getMaterial(id);
}

OkayMaterial& OkayMaterialHandle::get() const {
    return *owner->getMaterial(id);
}

std::uint32_t OkayMaterialRegistry::_idCounter = 0;

};  // namespace okay