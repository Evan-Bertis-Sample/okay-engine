#include "material.hpp"

#include <stdexcept>

namespace okay {

Failable Shader::compile() {
    if (_state != State::NOT_COMPILED) {
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

    _state = State::STANDBY;

    return Failable::ok({});
}

Failable Shader::set() {
    if (_state != State::STANDBY) {
        return Failable::errorResult("Shader must be compiled before setting it for use.");
    }
    glUseProgram(_shaderProgram);
    return Failable::ok({});
}

bool ShaderHandle::isNone() const {
    return owner == nullptr || id == Shader::invalidID() || !owner->validShader(*this);
}

const Shader* ShaderHandle::operator*() const {
    return get();
}

const Shader* ShaderHandle::operator->() const {
    return get();
}

const Shader* ShaderHandle::get() const {
    if (isNone())
        throw std::runtime_error(
            "Dereferencing a shader handle that is none! Check using ShaderHandle::isNone() before using * or -> operators on a handle!");
    return owner->getShader(*this);
}

Shader* ShaderHandle::operator*() {
    return get();
}

Shader* ShaderHandle::operator->() {
    return get();
}

Shader* ShaderHandle::get() {
    if (isNone())
        throw std::runtime_error(
            "Dereferencing a shader handle that is none! Check using ShaderHandle::isNone() before using * or -> operators on a handle!");
    return owner->getShader(*this);
}

bool MaterialHandle::isNone() const {
    return owner == nullptr || id == MaterialHandle::invalidID() || !owner->validMaterial(*this);
}

const Material* MaterialHandle::operator*() const {
    return get();
}

const Material* MaterialHandle::operator->() const {
    return get();
}

const Material* MaterialHandle::get() const {
    if (isNone())
        throw std::runtime_error(
            "Dereferencing a material handle that is none! Check using MaterialHandle::isNone() before using * or -> operators on a handle!");
    return owner->getMaterial(*this);
}

Material* MaterialHandle::operator*() {
    return get();
}

Material* MaterialHandle::operator->() {
    return get();
}

Material* MaterialHandle::get() {
    if (isNone())
        throw std::runtime_error(
            "Dereferencing a material handle that is none! Check using MaterialHandle::isNone() before using * or -> operators on a handle!");
    return owner->getMaterial(*this);
}

};  // namespace okay
