#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/util/result.hpp>
#include <glad/gl.h>

using namespace okay;

Failable OkayShader::compile() {
    // Compile vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
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
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
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
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        return Failable::errorResult("Shader program linking failed: " + std::string(infoLog));
    }

    // Clean up shaders as they're linked into the program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    _state = STANDBY;
    return Failable::ok({});
}

Failable OkayShader::set() {
    if (_state != STANDBY) {
        return Failable::errorResult("Shader must be compiled before setting it for use.");
    }

    glUseProgram(shaderProgram);
    _state = IN_USE;
    return Failable::ok({});
}

