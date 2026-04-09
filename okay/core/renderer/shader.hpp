#ifndef __SHADER_H__
#define __SHADER_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/result.hpp>

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>

namespace okay {

class Shader {
   public:
    enum class State { NOT_COMPILED, STANDBY };

    static constexpr std::uint32_t invalidID() { return 0xFFFFFFFFu; }

    Shader(const std::string& vertexSource, const std::string& fragmentSource)
        : vertexShader(std::move(vertexSource)),
          fragmentShader(std::move(fragmentSource)),
          _shaderProgram(0),
          _state(State::NOT_COMPILED) {
        std::hash<std::string> hasher;
        _srcHash = hasher(vertexShader + fragmentShader);
    }

    Shader()
        : vertexShader(""),
          fragmentShader(""),
          _shaderProgram(0),
          _state(State::NOT_COMPILED),
          _srcHash(invalidID()) {}

    std::string vertexShader;
    std::string fragmentShader;

    Failable compile();
    Failable set();

    bool isNone() const { return _srcHash == invalidID(); }

    State state() const { return _state; }

    std::uint32_t srcHash() const { return _srcHash; }

    GLuint programID() const { return _shaderProgram; }

    GLuint findUniformLocation(const std::string& uniform) {
        auto it = _uniforms.find(uniform);
        if (it != _uniforms.end()) {
            return it->second.location;
        }

        GLuint location = glGetUniformLocation(_shaderProgram, uniform.c_str());
        if (location == -1) {
            Engine.logger.warn("Failed to find uniform location for '{}'", uniform);
            location = uni::inactiveLocation();
        } else {
            Engine.logger.debug("Found uniform location for '{}': {}", uniform, location);
        }

        // add to map
        UniformInfo info{
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
            return Failable::ok({});
        };

        // add to map
        UniformInfo& info = _uniforms[uniform];
        info.location = location;
        info.value = value;
        uni::set(location, value);

        return Failable::ok({});
    }

    // enable equality operator
    bool operator==(const Shader& other) const {
        return _shaderProgram == other._shaderProgram && _state == other._state &&
               _srcHash == other._srcHash;
    }

    // enable inequality operator
    bool operator!=(const Shader& other) const { return !(*this == other); }

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

class MaterialRegistry;

struct ShaderHandle {
    MaterialRegistry* owner = nullptr;
    std::uint32_t id = Shader::invalidID();

    bool isValid() const { return owner != nullptr && id != Shader::invalidID(); }

    bool operator==(const ShaderHandle& other) const {
        return owner == other.owner && id == other.id;
    }

    bool operator!=(const ShaderHandle& other) const { return !(*this == other); }

    const Shader* operator*() const;
    const Shader* operator->() const;
    const Shader* get() const;

    Shader* operator*();
    Shader* operator->();
    Shader* get();
};

}  // namespace okay

#endif  // _SHADER_H__