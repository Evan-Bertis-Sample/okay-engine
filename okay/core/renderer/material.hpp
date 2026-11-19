#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/gpu.hpp>
#include <okay/core/renderer/shader.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/result.hpp>

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace okay {

enum class MaterialFlags : std::uint32_t {
    NONE = 0,
    TRANSPARENT = 1 << 0,
    DOUBLE_SIDED = 1 << 1,
    UNLIT = 1 << 2,
    CAST_SHADOWS = 1 << 3,
    RECEIVE_SHADOWS = 1 << 4,
    NORMAL_MAP = 1 << 5,
    SCREEN_SPACE = 1 << 6
};

inline MaterialFlags operator|(MaterialFlags a, MaterialFlags b) {
    return static_cast<MaterialFlags>(
        static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

inline MaterialFlags operator&(MaterialFlags a, MaterialFlags b) {
    return static_cast<MaterialFlags>(
        static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

inline MaterialFlags& operator|=(MaterialFlags& a, MaterialFlags b) {
    a = a | b;
    return a;
}

struct MaterialFlagCollection {
    std::uint32_t flags{static_cast<std::uint32_t>(MaterialFlags::NONE)};

    MaterialFlagCollection() = default;
    MaterialFlagCollection(MaterialFlags f) : flags(static_cast<std::uint32_t>(f)) {}
    MaterialFlagCollection(std::uint32_t rawFlags) : flags(rawFlags) {}

    static MaterialFlagCollection opaque() {
        return MaterialFlagCollection(MaterialFlags::CAST_SHADOWS | MaterialFlags::RECEIVE_SHADOWS);
    }

    static MaterialFlagCollection transparent() {
        return MaterialFlagCollection(MaterialFlags::TRANSPARENT | MaterialFlags::RECEIVE_SHADOWS);
    }

    static MaterialFlagCollection unlit() {
        return MaterialFlagCollection(MaterialFlags::UNLIT);
    }

    static MaterialFlagCollection unlitTransparent() {
        return MaterialFlagCollection(MaterialFlags::UNLIT | MaterialFlags::TRANSPARENT);
    }

    static MaterialFlagCollection doubleSided() {
        return MaterialFlagCollection(MaterialFlags::DOUBLE_SIDED | MaterialFlags::CAST_SHADOWS |
                                      MaterialFlags::RECEIVE_SHADOWS);
    }

    bool hasFlag(MaterialFlags f) const {
        return (flags & static_cast<std::uint32_t>(f)) != 0;
    }
    MaterialFlagCollection& addFlag(MaterialFlags f) {
        flags |= static_cast<std::uint32_t>(f);
        return *this;
    }
};

class IMaterialPropertyCollection {
   public:
    virtual Failable init(ShaderHandle shader) = 0;
    virtual Failable pass(ShaderHandle shader) = 0;
    virtual MaterialFlagCollection flags() = 0;
    virtual ~IMaterialPropertyCollection() {};
};

class Material {
   public:
    ShaderHandle shader;

    static constexpr std::uint32_t invalidID() {
        return 0xFFFFFFFFu;
    }

    Material(ShaderHandle shader,
        std::unique_ptr<IMaterialPropertyCollection> uniforms,
        std::uint32_t id)
        : shader(shader), _uniforms(std::move(uniforms)), _id(id) {}

    std::uint32_t id() const {
        return _id;
    }

    std::uint32_t shaderID() const {
        return shader->srcHash();
    }

    GLuint programID() const {
        return shader->programID();
    }

    Failable setShader() {
        if (shader.isNone()) {
            return Failable::errorResult("Material has no shader.");
        }

        if (shader->state() == Shader::State::NOT_COMPILED) {
            Failable compile = shader->compile();
            if (compile.isError()) {
                return compile;
            }
        }

        return shader->set();
    }

    Failable passUniforms() {
        if (!_uniforms) {
            return Failable::errorResult("Material has no uniforms.");
        }

        if (shader.isNone()) {
            return Failable::errorResult("Material has no shader.");
        }

        Failable set = setShader();
        if (set.isError()) {
            return set;
        }

        return _uniforms->pass(shader);
    }

    // disable copy
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) = default;
    Material& operator=(Material&&) = default;

    // equality operator
    bool operator==(const Material& other) const {
        return shader == other.shader && _id == other._id;
    }

    bool operator!=(const Material& other) const {
        return !(*this == other);
    }

    std::unique_ptr<IMaterialPropertyCollection>& properties() {
        return _uniforms;
    }

   private:
    std::size_t _id{invalidID()};
    std::unique_ptr<IMaterialPropertyCollection> _uniforms;

    void setMaterial() {
        setShader();
        passUniforms();
    }
};

struct MaterialHandle {
    static std::uint32_t invalidID() {
        return Material::invalidID();
    };

    static MaterialHandle none() {
        return {nullptr, MaterialHandle::invalidID()};
    }

    MaterialRegistry* owner{nullptr};
    std::uint32_t id{MaterialHandle::invalidID()};

    bool operator==(const MaterialHandle& other) const {
        return owner == other.owner && id == other.id;
    }

    bool operator!=(const MaterialHandle& other) const {
        return !(*this == other);
    }

    bool isNone() const;
    const Material* operator*() const;
    const Material* operator->() const;
    const Material* get() const;

    Material* operator*();
    Material* operator->();
    Material* get();
};

class MaterialRegistry {
   public:
    // Immovable because handles need a stable raw ptr to the registry
    MaterialRegistry() = default;
    MaterialRegistry(const MaterialRegistry&) = delete;
    MaterialRegistry& operator=(const MaterialRegistry&) = delete;

    MaterialRegistry(MaterialRegistry&&) = delete;
    MaterialRegistry& operator=(MaterialRegistry&&) = delete;

    ShaderHandle registerShader(
        const std::string& vertexSource, const std::string& fragmentSource) {
        // Create the shader, compile it, and add it to the registry
        Shader shader(vertexSource, fragmentSource);
        Failable f = shader.compile();
        if (f.isError()) {
            Engine.logger.error("Failed to register shader! Error : {}", f.error());
            return ShaderHandle::none();
        }

        _shaders.emplace(shader.programID(), shader);
        return {this, shader.programID()};
    };

    MaterialHandle registerMaterial(
        const ShaderHandle& shader, std::unique_ptr<IMaterialPropertyCollection> uniforms) {
        std::uint32_t id = _materials.size();

        if (shader.isNone()) {
            Engine.logger.warn(
                "Attempted to register a material with the an invalid shader (ShaderHandle::isNone() is true). "
                "Returning an invalid material.");
            return MaterialHandle::none();
        }

        _materials.emplace_back(std::make_unique<Material>(shader, std::move(uniforms), id));
        return {this, id};
    }

    bool validMaterial(const MaterialHandle& handle) const {
        return handle.owner == this && handle.id < _materials.size();
    }

    const Material* getMaterial(const MaterialHandle& handle) const {
        if (!validMaterial(handle))
            return nullptr;
        return _materials[handle.id].get();
    }

    Material* getMaterial(const MaterialHandle& handle) {
        if (!validMaterial(handle))
            return nullptr;
        return _materials[handle.id].get();
    }

    bool validShader(const ShaderHandle& handle) const {
        return handle.owner == this && _shaders.contains(handle.id);
    }

    const Shader* getShader(const ShaderHandle& handle) const {
        if (!validShader(handle))
            return nullptr;
        return &_shaders.at(handle.id);
    }

    Shader* getShader(const ShaderHandle& handle) {
        if (!validShader(handle))
            return nullptr;
        return &_shaders.at(handle.id);
    }

    // equality
    bool operator==(const MaterialRegistry& other) const {
        return _materials == other._materials;
    }

   private:
    std::vector<std::unique_ptr<Material>> _materials;
    std::unordered_map<GLuint, Shader> _shaders;
};

template <class Derived>
class OkayMaterialProperties : public IMaterialPropertyCollection {
   public:
    Failable init(ShaderHandle shader) override {
        auto& d = static_cast<Derived&>(*this);
        Failable out = Failable::ok({});

        // init plain uniforms (cache locations)
        tupleForEach(d.uniformRefs(), [&](auto& u) {
            if (out.isError())
                return;
            shader->findUniformLocation(u.name());
        });

        tupleForEach(d.textureRefs(), [&](auto& t) {
            if (out.isError())
                return;
            shader->findUniformLocation(t.name());
        });

        _initialized = true;
        return out;
    }

    Failable pass(ShaderHandle shader) override {
        auto& d = static_cast<Derived&>(*this);

        std::stringstream errorMessages;
        bool anyErrors = false;

        // plain uniforms (shader caches value+location)
        tupleForEach(d.uniformRefs(), [&](auto& u) {
            auto r = shader->setUniform(u.name(), u.get());
            if (r.isError()) {
                errorMessages << r.error() << '\n';
                anyErrors = true;
            }
        });

        auto& gpu = GPUState::instance();

        // uniform blocks (GPU manager owns UBO objects + binding points)
        tupleForEach(d.uniformBlockRefs(), [&](auto& b) {
            auto r = gpu.blocks.pass(shader->programID(), b);
            if (r.isError()) {
                errorMessages << r.error() << '\n';
                anyErrors = true;
            }
        });

        GLuint textureUnit = 0;
        tupleForEach(d.textureRefs(), [&](auto& t) {
            GLuint loc = shader->findUniformLocation(t.name());
            if (loc == uni::inactiveLocation()) {
                return;
            }

            auto r = gpu.textures.bindSampler2D(
                shader->programID(), loc, t.get(), t.params(), textureUnit);

            if (r.isError()) {
                errorMessages << r.error() << '\n';
                anyErrors = true;
            }

            ++textureUnit;
        });

        return anyErrors ? Failable::errorResult(errorMessages.str()) : Failable::ok({});
    }

    MaterialFlagCollection flags() override {
        auto& d = static_cast<Derived&>(*this);
        return d.flags();
    }

   private:
    bool _initialized{false};
};
};  // namespace okay

#endif  // _MATERIAL_H__
