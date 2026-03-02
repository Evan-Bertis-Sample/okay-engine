#ifndef __OKAY_SHADER_H__
#define __OKAY_SHADER_H__

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/okay.hpp>

namespace okay {

class OkayShader {
   public:
    enum class State { NOT_COMPILED, STANDBY };

    static constexpr std::uint32_t invalidID() {
        return 0xFFFFFFFFu;
    }

    OkayShader(const std::string& vertexSource, const std::string& fragmentSource)
        : vertexShader(std::move(vertexSource)),
          fragmentShader(std::move(fragmentSource)),
          _shaderProgram(0),
          _state(State::NOT_COMPILED) {
        std::hash<std::string> hasher;
        _srcHash = hasher(vertexShader + fragmentShader);
    }

    OkayShader()
        : vertexShader(""),
          fragmentShader(""),
          _shaderProgram(0),
          _state(State::NOT_COMPILED),
          _srcHash(invalidID()) {
    }

    std::string vertexShader;
    std::string fragmentShader;

    Failable compile();
    Failable set();

    bool isNone() const {
        return _srcHash == invalidID();
    }

    State state() const {
        return _state;
    }

    std::uint32_t srcHash() const {
        return _srcHash;
    }

    GLuint programID() const {
        return _shaderProgram;
    }

    GLuint findUniformLocation(const std::string& uniform) {
        auto it = _uniforms.find(uniform);
        if (it != _uniforms.end()) {
            return it->second.location;
        }

        GLuint location = glGetUniformLocation(_shaderProgram, uniform.c_str());
        if (location == -1) {
            Engine.logger.error("Failed to find uniform location for '{}'", uniform);
            location = uni::inactiveLocation();
        }

        // add to map
        UniformInfo info {
            .location = location,
            .value = uni::UniformValue::none(),
        };

        _uniforms[uniform] = info;

        return location;
    }

    template <class T>
    Failable setUniform(const std::string& uniform, const T& value) {
        auto it = _uniforms.find(uniform);

        // If we have this uniform already, then we only need to 
        // grab the info once
        if (it != _uniforms.end()) {
            UniformInfo& info = it->second;
            if (info.location == uni::inactiveLocation()) {
                return Failable::ok({});
            }

            if (info.value == value) {
                return Failable::ok({});
            }

            uni::set(info.location, value);
            info.value = value;
            return Failable::ok({});
        }

        // We don't have this uniform yet, so we need to find it
        auto location = findUniformLocation(uniform);
        if (location == uni::inactiveLocation()) {
            Engine.logger.error("Failed to find uniform location for '{}'", uniform);
            return Failable::ok({});
        };
        
        // add to map
        UniformInfo &info = _uniforms[uniform];
        info.location = location;
        info.value = value;
        uni::set(location, value);

        return Failable::ok({});
    }

    // enable equality operator
    bool operator==(const OkayShader& other) const {
        return _shaderProgram == other._shaderProgram && _state == other._state &&
               _srcHash == other._srcHash;
    }

    // enable inequality operator
    bool operator!=(const OkayShader& other) const {
        return !(*this == other);
    }

   private:
   struct UniformInfo {
       GLuint location;
       uni::UniformValue value;
    };

    GLuint _shaderProgram;
    State _state;
    std::size_t _srcHash;

    std::unordered_map<std::string, UniformInfo> _uniforms;
};

class OkayMaterialRegistry;

struct OkayShaderHandle {
    OkayMaterialRegistry* owner = nullptr;
    std::uint32_t id = OkayShader::invalidID();

    bool isValid() const {
        return owner != nullptr && id != OkayShader::invalidID();
    }

    bool operator==(const OkayShaderHandle& other) const {
        return owner == other.owner && id == other.id;
    }

    bool operator!=(const OkayShaderHandle& other) const {
        return !(*this == other);
    }

    const OkayShader* operator*() const;
    const OkayShader* operator->() const;
    const OkayShader* get() const;

    OkayShader* operator*();
    OkayShader* operator->();
    OkayShader* get();
};
}  // namespace okay

#endif  // __OKAY_SHADER_H__