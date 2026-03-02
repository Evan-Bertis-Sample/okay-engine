#ifndef __OKAY_UNIFORM_H__
#define __OKAY_UNIFORM_H__

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <okay/core/renderer/okay_gl.hpp>
#include <okay/core/okay.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/util/type.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <type_traits>

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
    else if constexpr (std::is_same_v<T, OkayTexture>)
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
        return Failable::errorResult("Texture uniforms are bound via samplers/units elsewhere");
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

};  // namespace uni

template <class T, auto Name>
struct TemplatedMaterialUniform {
    using ValueType = T;
    static constexpr auto nameV = Name;
    static constexpr uni::UniformKind kind = uni::kindFromType<T>();

    constexpr std::string_view nameView() const {
        return nameV.sv();
    }

    constexpr std::string name() const {
        return std::string(nameV.sv());
    }

    void set(const T& v) {
        if (_value != v) {
            _dirty = true;
        }
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

    bool operator==(const TemplatedMaterialUniform& other) const {
        return _value == other._value && nameView() == other.nameView();
    }

    // set operators
    TemplatedMaterialUniform& operator=(const T& value) {
        set(value);
        return *this;
    }

    TemplatedMaterialUniform& operator=(const TemplatedMaterialUniform& other) {
        set(other._value);
        return *this;
    }

   private:
    T _value{};
    bool _dirty{true};
};

class OkayUniformBuffer {
   public:
    ~OkayUniformBuffer() {
        destroy();
    }

    Failable init(std::size_t byteSize, GLenum usage = GL_DYNAMIC_DRAW) {
        if (_id != 0)
            return Failable::ok({});
        GL_CHECK_FAILABLE(glGenBuffers(1, &_id));
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, _id));
        GL_CHECK_FAILABLE(glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)byteSize, nullptr, usage));
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, 0));
        _size = byteSize;
        return Failable::ok({});
    }

    void destroy() {
        if (_id)
            glDeleteBuffers(1, &_id);
        _id = 0;
        _size = 0;
    }

    Failable upload(const void* data, std::size_t byteSize, std::size_t offset = 0) {
        if (_id == 0)
            return Failable::errorResult("UBO not initialized");
        if (offset + byteSize > _size)
            return Failable::errorResult("UBO upload out of bounds");
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, _id));
        GL_CHECK_FAILABLE(
            glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, (GLsizeiptr)byteSize, data));
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, 0));
        return Failable::ok({});
    }

    void bindBase(GLuint bindingPoint) const {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, _id);
    }

    GLuint id() const {
        return _id;
    }
    std::size_t size() const {
        return _size;
    }

   private:
    GLuint _id{0};
    std::size_t _size{0};
};

template <class TBlock, GLuint BindingPoint, auto BlockName>
class TemplatedUniformBlock {
   public:
    static constexpr auto name = BlockName;

    void set(const TBlock& v) {
        _value = v;
        _dirty = true;
    }
    TBlock& edit() {
        _dirty = true;
        return _value;
    }
    const TBlock& get() const {
        return _value;
    }

    void markDirty() {
        _dirty = true;
    }
    bool isDirty() const {
        return _dirty;
    }

    Failable init(GLuint program) {
        auto r = ensureInit();
        if (r.isError())
            return r;

        // Bind the program’s block index to our binding point once.
        GLuint idx = glGetUniformBlockIndex(program, std::string(name.sv()).c_str());
        if (idx == GL_INVALID_INDEX) {
            return Failable::errorResult("Uniform block not found: " + std::string(name.sv()));
        }
        glUniformBlockBinding(program, idx, BindingPoint);
        return Failable::ok({});
    }

    Failable pass(GLuint program) {
        auto r = ensureInit();
        if (r.isError())
            return r;

        _ubo.bindBase(BindingPoint);

        if (!_dirty)
            return Failable::ok({});
        r = _ubo.upload(&_value, sizeof(TBlock));
        if (r.isError())
            return r;
        _dirty = false;
        return Failable::ok({});
    }

   private:
    Failable ensureInit() {
        if (_ubo.id() != 0)
            return Failable::ok({});
        return _ubo.init(sizeof(TBlock), GL_DYNAMIC_DRAW);
    }

    TBlock _value{};
    bool _dirty{true};
    OkayUniformBuffer _ubo{};
};

};  // namespace okay

#endif  // __OKAY_UNIFORM_H__
