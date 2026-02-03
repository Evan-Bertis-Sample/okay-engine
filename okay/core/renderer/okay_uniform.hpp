#ifndef __OKAY_UNIFORM_H__
#define __OKAY_UNIFORM_H__

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/util/type.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <sstream>
#include <tuple>
#include <type_traits>

namespace okay {

enum class UniformKind { FLOAT, INT, VEC2, VEC3, VEC4, MAT3, MAT4, TEXTURE, VOID };

namespace uni {

template <class T>
constexpr UniformKind kindFromType() {
    if constexpr (std::is_same_v<T, float>)
        return UniformKind::FLOAT;
    else if constexpr (std::is_same_v<T, int>)
        return UniformKind::INT;
    else if constexpr (std::is_same_v<T, glm::vec2>)
        return UniformKind::VEC2;
    else if constexpr (std::is_same_v<T, glm::vec3>)
        return UniformKind::VEC3;
    else if constexpr (std::is_same_v<T, glm::vec4>)
        return UniformKind::VEC4;
    else if constexpr (std::is_same_v<T, glm::mat3>)
        return UniformKind::MAT3;
    else if constexpr (std::is_same_v<T, glm::mat4>)
        return UniformKind::MAT4;
    else if constexpr (std::is_same_v<T, OkayTexture>)
        return UniformKind::TEXTURE;
    else if constexpr (std::is_same_v<T, NoneType>)
        return UniformKind::VOID;
    else
        static_assert(dependent_false_v<T>, "Unsupported uniform type");
}

template <class T>
Failable set(GLuint uniformLoc, T value) {
    constexpr UniformKind kind = kindFromType<T>();

    // Set the uniform value
    if constexpr (kind == UniformKind::FLOAT) {
        glUniform1f(location(), get());
    } else if constexpr (kind == UniformKind::INT) {
        glUniform1i(location(), get());
    } else if constexpr (kind == UniformKind::VEC2) {
        glUniform2fv(location(), 1, &get().x);
    } else if constexpr (kind == UniformKind::VEC3) {
        glUniform3fv(location(), 1, &get().x);
    } else if constexpr (kind == UniformKind::VEC4) {
        glUniform4fv(location(), 1, &get().x);
    } else if constexpr (kind == UniformKind::MAT3) {
        glUniformMatrix3fv(location(), 1, GL_FALSE, &get()[0][0]);
    } else if constexpr (kind == UniformKind::MAT4) {
        glUniformMatrix4fv(location(), 1, GL_FALSE, &get()[0][0]);
    } else if constexpr (kind == UniformKind::TEXTURE) {
        // Texture binding handled elsewhere
        return Failable::errorResult("Direct setting of texture uniforms is not supported");
    } else if constexpr (kind == UniformKind::VOID) {
        // No action needed
    } else {
        return Failable::errorResult("Unsupported uniform type");
    }

};  // namespace uni

template <class T, auto Name>
struct TemplatedMaterialUniform {
    using ValueType = T;

    static constexpr auto name = Name;
    static constexpr UniformKind kind = uni::kindFromType<T>();
    static constexpr GLuint invalidLocation() {
        return 0xFFFFFFFFu;
    }

    constexpr std::string_view nameView() const {
        return name.sv();
    }

    void set(const T& v) {
        if (_value != v)
            _dirty = true;
        _value = v;
    }

    const T& get() const {
        return _value;
    }

    bool isDirty() const {
        return _dirty;
    }

    void markDirty() {
        _dirty = true;
    }

    GLuint location() const {
        return _location;
    }

    Failable findLocation(GLuint shaderProgram) {
        int location = glGetUniformLocation(shaderProgram, std::string(name.sv()).c_str());
        if (location == -1) {
            return Failable::errorResult("Uniform not found: " + std::string(name.sv()));
        }
        _location = static_cast<GLuint>(location);
        return Failable::ok({});
    };

    Failable passUniform(GLuint shaderProgram) {
        if (!isDirty())
            return Failable::ok({});

        if (location() == invalidLocation()) {
            Failable result = findLocation(shaderProgram);
            if (result.isError()) {
                return Failable::errorResult("Failed to find uniform location for '" +
                                             std::string(name.sv()) + "': " + result.error());
            }
        }

        Failable result = uni::set(location(), _value);
        if (result.isError()) {
            return Failable::errorResult("Failed to set uniform '" + std::string(name.sv()) +
                                         "': " + result.error());
        }

        _dirty = false;
        return Failable::ok({});
    }

   private:
    T _value{};
    bool _dirty{true};
    GLuint _location{invalidLocation()};
};

class IOkayMaterialUniformCollection {
   public:
    virtual Failable setUniforms(GLuint shaderProgram) = 0;
    virtual Failable findLocations(GLuint shaderProgram) = 0;
    virtual bool markAllDirty() = 0;
};

}  // namespace okay

#endif  // __OKAY_UNIFORM_H__
