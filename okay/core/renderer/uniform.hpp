#ifndef __UNIFORM_H__
#define __UNIFORM_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/logger.hpp>
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/util/type.hpp>

#include <atomic>
#include <cstdint>
#include <glm/glm.hpp>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace okay {

namespace uni {

enum class UniformKind { FLOAT, INT, VEC2, VEC3, VEC4, MAT3, MAT4, TEXTURE, VOID };

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
    else if constexpr (std::is_same_v<T, Texture>)
        return UniformKind::TEXTURE;
    else if constexpr (std::is_same_v<T, NoneType>)
        return UniformKind::VOID;
    else
        static_assert(dependent_false_v<T>, "Unsupported uniform type");
}

template <class T>
Failable set(GLuint uniformLoc, const T& value) {
    constexpr UniformKind kind = kindFromType<T>();

    if constexpr (kind == UniformKind::FLOAT) {
        glUniform1f(uniformLoc, value);
    } else if constexpr (kind == UniformKind::INT) {
        glUniform1i(uniformLoc, value);
    } else if constexpr (kind == UniformKind::VEC2) {
        glUniform2fv(uniformLoc, 1, &value.x);
    } else if constexpr (kind == UniformKind::VEC3) {
        glUniform3fv(uniformLoc, 1, &value.x);
    } else if constexpr (kind == UniformKind::VEC4) {
        glUniform4fv(uniformLoc, 1, &value.x);
    } else if constexpr (kind == UniformKind::MAT3) {
        glUniformMatrix3fv(uniformLoc, 1, GL_FALSE, &value[0][0]);
    } else if constexpr (kind == UniformKind::MAT4) {
        glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, &value[0][0]);
    } else if constexpr (kind == UniformKind::TEXTURE) {
        return Failable::errorResult(
            "Texture setting must be done through OkayTextureManager::bindSampler2D()");
    } else if constexpr (kind == UniformKind::VOID) {
        // no-op
    } else {
        return Failable::errorResult("Unsupported uniform type");
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        return Failable::errorResult("Failed to set uniform. Error code: " + std::to_string(err));
    }

    return Failable::ok({});
}

static constexpr GLuint invalidLocation() {
    return 0xFFFFFFFFu;
}
static constexpr GLuint inactiveLocation() {
    return 0xFFFFFFFEu;
}

struct UniformValue {
    UniformKind kind;
    union {
        float f;
        int i;
        glm::vec2 v2;
        glm::vec3 v3;
        glm::vec4 v4;
        glm::mat3 m3;
        glm::mat4 m4;
    };

    static UniformValue none() {
        UniformValue v{};
        v.kind = UniformKind::VOID;
        return v;
    }

    template <class T>
    bool operator==(const T& other) const {
        if (kind != kindFromType<T>()) {
            return false;
        }
        if constexpr (std::is_same_v<T, float>) {
            return f == other;
        } else if constexpr (std::is_same_v<T, int>) {
            return i == other;
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            return v2 == other;
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            return v3 == other;
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            return v4 == other;
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            return m3 == other;
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            return m4 == other;
        } else {
            static_assert(dependent_false_v<T>, "Unsupported uniform type");
        }
        return false;
    }

    template <class T>
    void operator=(const T& other) {
        if constexpr (std::is_same_v<T, float>) {
            f = other;
        } else if constexpr (std::is_same_v<T, int>) {
            i = other;
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            v2 = other;
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            v3 = other;
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            v4 = other;
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            m3 = other;
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            m4 = other;
        } else {
            static_assert(dependent_false_v<T>, "Unsupported uniform type");
        }
        kind = kindFromType<T>();
    }
};

};  // namespace uni

template <class T, auto Name>
struct UniformProperty {
    using ValueType = T;
    static constexpr auto nameV = Name;
    static constexpr uni::UniformKind kind = uni::kindFromType<T>();

    UniformProperty() {}
    UniformProperty(const T& value) : _value(value) {}

    constexpr std::string_view nameView() const { return nameV.sv(); }
    constexpr std::string name() const { return std::string(nameV.sv()); }

    void set(const T& v) { _value = v; }
    const T& get() const { return _value; }

    bool operator==(const UniformProperty& other) const {
        return _value == other._value && nameView() == other.nameView();
    }

    UniformProperty& operator=(const T& value) {
        set(value);
        return *this;
    }
    UniformProperty& operator=(const UniformProperty& other) {
        set(other._value);
        return *this;
    }

   private:
    T _value{};
};

template <class TBlock, auto BlockName>
class BlockProperty {
   public:
    using ValueType = TBlock;
    static constexpr auto name = BlockName;

    void set(const TBlock& v) {
        _value = v;
        ++_version;
    }
    TBlock& edit() {
        ++_version;
        return _value;
    }
    const TBlock& get() const { return _value; }

    std::uint32_t version() const { return _version; }

    // Stable identity for manager caching
    const void* identity() const { return this; }

    // Optional binding point hint for the manager (defaults to invalid)
    void setBindingPoint(GLuint bindingPoint) { _bindingHint = bindingPoint; }
    GLuint bindingPointHint() const { return _bindingHint; }

   private:
    static constexpr GLuint invalidBinding() { return 0xFFFFFFFFu; }

    TBlock _value{};
    std::uint32_t _version{1};
    GLuint _bindingHint{invalidBinding()};
};

template <auto SamplerName>
class TextureProperty {
   public:
    using ValueType = Texture;
    static constexpr auto nameV = SamplerName;
    static constexpr uni::UniformKind kind = uni::UniformKind::TEXTURE;

    TextureProperty() = default;
    TextureProperty(const Texture& tex, const Texture::TextureParameters& params = {})
        : _value(tex), _params(params) {}

    constexpr std::string_view nameView() const { return nameV.sv(); }
    constexpr std::string name() const { return std::string(nameV.sv()); }

    void set(const Texture& tex) {
        _value = tex;
        ++_version;
    }
    Texture& edit() {
        ++_version;
        return _value;
    }
    const Texture& get() const { return _value; }

    void setParams(const Texture::TextureParameters& p) {
        _params = p;
        ++_version;
    }
    const Texture::TextureParameters& params() const { return _params; }

    void setUnit(GLuint unit) { _unit = unit; }
    GLuint unit() const { return _unit; }

    std::uint32_t version() const { return _version; }

    TextureProperty& operator=(const Texture& tex) {
        set(tex);
        return *this;
    }

   private:
    Texture _value{};
    Texture::TextureParameters _params{};
    GLuint _unit{0};
    std::uint32_t _version{1};
};

};  // namespace okay

#endif  // _UNIFORM_H__