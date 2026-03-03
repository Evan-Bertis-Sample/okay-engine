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
#include <set>
#include <atomic>

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
        return Failable::errorResult("Texture setting must be done through TextureProperty::pass()");
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

    // comparison operator
    template <class T>
    bool operator==(const T& other) const {
        // check that the kinds match
        if (kind != kindFromType<T>()) {
            return false;
        }
        // check that the values match
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

    // asignment
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

    UniformProperty() {
    }
    UniformProperty(const T& value) : _value(value) {
    }

    constexpr std::string_view nameView() const {
        return nameV.sv();
    }

    constexpr std::string name() const {
        return std::string(nameV.sv());
    }

    void set(const T& v) {
        _value = v;
    }

    const T& get() const {
        return _value;
    }

    bool operator==(const UniformProperty& other) const {
        return _value == other._value && nameView() == other.nameView();
    }

    // set operators
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

template <class T>
class TemplatedUniformBuffer {
   public:
    ~TemplatedUniformBuffer() {
        destroy();
    }

    Failable init(GLenum usage = GL_DYNAMIC_DRAW) {
        if (_id != 0)
            return Failable::ok({});

        GL_CHECK_FAILABLE(glGenBuffers(1, &_id));
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, _id));
        GL_CHECK_FAILABLE(glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)sizeof(T), nullptr, usage));
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, 0));

        Engine.logger.debug("UBO {} initialized with size {}", _id, sizeof(T));
        return Failable::ok({});
    }

    void destroy() {
        if (_id)
            glDeleteBuffers(1, &_id);
        _id = 0;
    }

    Failable upload(const T& data, std::size_t offset = 0) {
        if (_id == 0)
            return Failable::errorResult("UBO not initialized");
        if (offset + sizeof(T) > sizeof(T))
            return Failable::errorResult("UBO upload out of bounds");

        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, _id));
        GL_CHECK_FAILABLE(
            glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, (GLsizeiptr)sizeof(T), &data));
        GL_CHECK_FAILABLE(glBindBuffer(GL_UNIFORM_BUFFER, 0));

        return Failable::ok({});
    }

    void bindBase(GLuint bindingPoint) const {
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, _id);
    }

    GLuint id() const {
        return _id;
    }

   private:
    GLuint _id{0};
};

template <class TBlock, auto BlockName>
class BlockProperty {
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

    // Optional: if YOU want to set a specific binding point manually.
    // If you never call this, it'll auto-allocate.
    void setBindingPoint(GLuint bindingPoint) {
        _bindingPoint = bindingPoint;
    }

    // Keep: init(program)
    Failable init(GLuint program) {
        auto r = ensureInit();
        if (r.isError())
            return r;

        r = ensureBindingPointAllocated();
        if (r.isError())
            return r;

        // Bind this program's uniform block index -> our binding point
        GLuint idx = glGetUniformBlockIndex(program, std::string(name.sv()).c_str());
        if (idx == GL_INVALID_INDEX) {
            return Failable::errorResult("Uniform block not found: " + std::string(name.sv()));
        }

        glUniformBlockBinding(program, idx, _bindingPoint);

        // Cache so we don't redo it for the same program
        _boundPrograms.insert(program);

        Engine.logger.debug("UBO {} initialized for program {}", _bindingPoint, program);

        return Failable::ok({});
    }

    // Keep: pass(program)
    Failable pass(GLuint program) {
        auto r = ensureInit();
        if (r.isError())
            return r;

        r = ensureBindingPointAllocated();
        if (r.isError())
            return r;

        // If init() wasn't called for this program, do the binding now (safe + cheap)
        if (_boundPrograms.find(program) == _boundPrograms.end()) {
            r = init(program);
            if (r.isError())
                return r;
        }

        _ubo.bindBase(_bindingPoint);

        if (!_dirty)
            return Failable::ok({});

        r = _ubo.upload(_value);
        if (r.isError())
            return r;

        _dirty = false;
        return Failable::ok({});
    }

    GLuint bindingPoint() const {
        return _bindingPoint;
    }

   private:
    Failable ensureInit() {
        if (_ubo.id() != 0)
            return Failable::ok({});
        return _ubo.init(GL_DYNAMIC_DRAW);
    }

    Failable ensureBindingPointAllocated() {
        if (_bindingPoint != invalidBinding())
            return Failable::ok({});

        // Allocate a binding point globally (per process) for this instance.
        GLint maxBindings = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxBindings);

        GLuint next = s_nextBindingPoint.fetch_add(1, std::memory_order_relaxed);
        if ((GLint)next >= maxBindings) {
            return Failable::errorResult(
                "Out of UBO binding points. GL_MAX_UNIFORM_BUFFER_BINDINGS=" +
                std::to_string(maxBindings));
        }

        _bindingPoint = next;
        return Failable::ok({});
    }

    static constexpr GLuint invalidBinding() {
        return 0xFFFFFFFFu;
    }

   private:
    TBlock _value{};
    bool _dirty{true};

    TemplatedUniformBuffer<TBlock> _ubo{};

    GLuint _bindingPoint{invalidBinding()};
    std::set<GLuint> _boundPrograms;

    inline static std::atomic<GLuint> s_nextBindingPoint{0};
};

template <auto SamplerName>
class TextureProperty {
   public:
    using ValueType = OkayTexture;
    static constexpr auto nameV = SamplerName;
    static constexpr uni::UniformKind kind = uni::UniformKind::TEXTURE;

    TextureProperty() = default;

    TextureProperty(const OkayTexture& tex,
                            const OkayTexture::TextureParameters& params = {})
        : _value(tex), _params(params) {
    }

    constexpr std::string_view nameView() const {
        return nameV.sv();
    }
    constexpr std::string name() const {
        return std::string(nameV.sv());
    }

    void set(const OkayTexture& tex) {
        _value = tex;
        _dirty = true;
    }
    OkayTexture& edit() {
        _dirty = true;
        return _value;
    }
    const OkayTexture& get() const {
        return _value;
    }

    void setParams(const OkayTexture::TextureParameters& p) {
        _params = p;
        _dirty = true;
    }
    const OkayTexture::TextureParameters& params() const {
        return _params;
    }

    void markDirty() {
        _dirty = true;
    }
    bool isDirty() const {
        return _dirty;
    }

    // "block location" == texture unit
    void setUnit(GLuint unit) {
        _unit = unit;
    }
    GLuint unit() const {
        return _unit;
    }

    // Cache location for this program (optional, pass() will lazy-init too)
    Failable init(GLuint program) {
        auto r = ensureUnitAllocated();
        if (r.isError())
            return r;

        auto loc = glGetUniformLocation(program, std::string(nameV.sv()).c_str());
        // If not found, treat as inactive (shader optimized it out)
        if (loc < 0) {
            _locations[program] = inactiveLocation();
            return Failable::ok({});
        }
        _locations[program] = (GLuint)loc;
        return Failable::ok({});
    }

    Failable pass(GLuint program) {
        auto r = ensureUnitAllocated();
        if (r.isError())
            return r;

        GLuint loc = cachedLocation(program);
        if (loc == invalidLocation()) {
            r = init(program);
            if (r.isError())
                return r;
            loc = cachedLocation(program);
        }

        // Inactive sampler -> nothing to do
        if (loc == inactiveLocation()) {
            _dirty = false;
            return Failable::ok({});
        }

        if (OkayTextureDataStore::TextureHandle::isNone(_value.handle) || !_value.store) {
            return Failable::errorResult("Texture uniform '" + std::string(nameV.sv()) +
                                         "' has no texture value/store");
        }

        // Ensure uploaded (you can choose to only do this when dirty)
        if (!_value.hasBeenUploadedToGPU()) {
            auto up = _value.uploadToGPU(_params);
            if (up.isError())
                return up;
        } else if (_dirty) {
            // Optional: if you want param changes to apply even after upload
            // (this is cheap; it just sets tex parameters on the already-created texture)
            GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, _value.getGLTextureID()));
            GL_CHECK_FAILABLE(
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _params.minFilter));
            GL_CHECK_FAILABLE(
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _params.magFilter));
            GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _params.wrapS));
            GL_CHECK_FAILABLE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _params.wrapT));
        }

        // Bind to unit + assign sampler = unit
        GL_CHECK_FAILABLE(glUseProgram(program));
        GL_CHECK_FAILABLE(glActiveTexture(GL_TEXTURE0 + _unit));
        GL_CHECK_FAILABLE(glBindTexture(GL_TEXTURE_2D, _value.getGLTextureID()));
        GL_CHECK_FAILABLE(glUniform1i((GLint)loc, (GLint)_unit));

        _dirty = false;
        return Failable::ok({});
    }

   private:
    static constexpr GLuint invalidLocation() {
        return 0xFFFFFFFFu;
    }
    static constexpr GLuint inactiveLocation() {
        return 0xFFFFFFFEu;
    }
    static constexpr GLuint invalidUnit() {
        return 0xFFFFFFFFu;
    }

    GLuint cachedLocation(GLuint program) const {
        auto it = _locations.find(program);
        if (it == _locations.end())
            return invalidLocation();
        return it->second;
    }

    Failable ensureUnitAllocated() {
        if (_unit != invalidUnit())
            return Failable::ok({});

        GLint maxUnits = 0;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);

        GLuint next = s_nextUnit.fetch_add(1, std::memory_order_relaxed);
        if ((GLint)next >= maxUnits) {
            return Failable::errorResult(
                "Out of texture units. GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS=" +
                std::to_string(maxUnits));
        }

        _unit = next;
        return Failable::ok({});
    }

   private:
    OkayTexture _value{};
    OkayTexture::TextureParameters _params{};

    bool _dirty{true};

    GLuint _unit{invalidUnit()};
    std::unordered_map<GLuint, GLuint> _locations;

    inline static std::atomic<GLuint> s_nextUnit{0};
};

};  // namespace okay

#endif  // __OKAY_UNIFORM_H__
