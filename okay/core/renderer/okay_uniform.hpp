#ifndef __OKAY_UNIFORM_H__
#define __OKAY_UNIFORM_H__

#include <glm/glm.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/util/type.hpp>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace okay {

enum class UniformKind { FLOAT, INT, VEC2, VEC3, VEC4, MAT3, MAT4, TEXTURE, VOID };

template <class T>
constexpr UniformKind UniformKindOf() {
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

// Name is an NTTP value, typically FixedString<N>
template <class T, auto Name>
struct OkayMaterialUniform {
    using ValueType = T;

    static constexpr auto name = Name;
    static constexpr UniformKind kind = UniformKindOf<T>();
    static constexpr GLuint invalidLocation() { return 0xFFFFFFFFu; }

    constexpr std::string_view name_sv() const { return name.sv(); }

    void set(const T& v) {
        if (_value != v) _dirty = true;
        _value = v;
    }
    const T& get() const { return _value; }
    bool isDirty() const { return _dirty; }
    void markDirty() { _dirty = true; }
    GLuint location() const { return _location; }

    Failable findLocation(GLuint shaderProgram) {
        int location = glGetUniformLocation(shaderProgram, std::string(name.sv()).c_str());
        if (location == -1) {
            return Failable::errorResult("Uniform not found: " + std::string(name.sv()));
        }
        _location = static_cast<GLuint>(location);
        return Failable::ok({});
    };

    Failable passUniform(GLuint shaderProgram) {
        if (location() == invalidLocation()) {
            Failable result = findLocation(shaderProgram);
            if (result.isError()) {
                return Failable::errorResult("Failed to find uniform location for '" +
                                             std::string(name.sv()) + "': " + result.error());
            }
        }

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
            return Failable::errorResult(
                "Direct setting of texture uniforms is not supported for '" +
                std::string(name.sv()) + "'");
        } else if constexpr (kind == UniformKind::VOID) {
            // No action needed
        } else {
            return Failable::errorResult("Unsupported uniform type for '" + std::string(name.sv()) +
                                         "'");
        }

        _dirty = false;
        return Failable::ok({});
    }

   private:
    T _value{};
    bool _dirty{true};
    GLuint _location{invalidLocation()};
};

// special definition for none type of uniforms
template <>
struct OkayMaterialUniform<NoneType, FixedString("")> {
    using ValueType = NoneType;

    static constexpr auto name = FixedString("");
    static constexpr UniformKind kind = UniformKind::VOID;
    static constexpr GLuint invalidLocation() { return 0xFFFFFFFFu; }

    constexpr std::string_view name_sv() const { return name.sv(); }

    void set(const NoneType&) {}
    const NoneType& get() const { return _value; }
    void markDirty() {}
    bool isDirty() const { return false; }

    GLuint location() const { return invalidLocation(); }
    Failable findLocation(GLuint) { return Failable::ok({}); };
    Failable passUniform(GLuint) { return Failable::ok({}); };

   private:
    NoneType _value{};
};

// define none type for empty uniform collections
using OkayMaterialUniformNone = OkayMaterialUniform<NoneType, FixedString("")>;

template <auto Name, class... Uniforms>
struct UniformIndex;

// Helper to get index of uniform by name at compile time
template <auto Name, class First, class... Rest>
struct UniformIndex<Name, First, Rest...> {
    static constexpr std::size_t value =
        std::bool_constant<First::name.sv() == Name.sv()>::value
            ? 0
            : 1 + UniformIndex<Name, Rest...>::value;
};

template <auto Name, class Last>
struct UniformIndex<Name, Last> {
    static constexpr std::size_t value =
        std::bool_constant<Last::name.sv() == Name.sv()>::value ? 0 : 0;
};

template <auto Name>
struct UniformIndex<Name> {
    static_assert(dependent_false_v<decltype(Name)>,
                  "Uniform with the specified name not found in OkayMaterialUniformCollection");
    static constexpr std::size_t value = 0;
};


template <class... Uniforms>
struct OkayMaterialUniformCollection {
    std::tuple<Uniforms...> uniforms;

    template <auto Name, std::size_t I = UniformIndex<Name, Uniforms...>::value>
    decltype(auto) get() const {
        return std::get<I>(uniforms);
    }

    template <auto Name, std::size_t I = UniformIndex<Name, Uniforms...>::value>
    decltype(auto) get() {
        return std::get<I>(uniforms);
    }

    std::size_t size() const { return sizeof...(Uniforms); }

    template <auto Name>
    constexpr std::size_t index() const {
        return UniformIndex<Name, Uniforms...>::value;
    }

    Failable setUniforms(GLuint shaderProgram) {
        // Iterate over all uniforms and set them if dirty
        bool hasFailed = false;
        std::stringstream errorMessages;
        (
            [&]() {
                Failable result = std::get<Uniforms>(uniforms).passUniform(shaderProgram);
                if (result.isError()) {
                    hasFailed = true;
                    errorMessages << result.error() << "\n";
                }
            }(),
            ...);

        if (hasFailed) {
            return Failable::errorResult(errorMessages.str());
        } else {
            return Failable::ok({});
        }
    }

    Failable findLocations(GLuint shaderProgram) {
        // Iterate over all uniforms and find their locations
        bool hasFailed = false;
        std::stringstream errorMessages;
        (
            [&]() {
                Failable result = std::get<Uniforms>(uniforms).findLocation(shaderProgram);
                if (result.isError()) {
                    hasFailed = true;
                    errorMessages << result.error() << "\n";
                }
            }(),
            ...);

        if (hasFailed) {
            return Failable::errorResult(errorMessages.str());
        } else {
            return Failable::ok({});
        }
    }

    bool markAllDirty() {
        ([&]() { std::get<Uniforms>(uniforms).markDirty(); }(), ...);
        return true;
    }
};

}  // namespace okay

#endif  // __OKAY_UNIFORM_H__
