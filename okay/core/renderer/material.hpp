#ifndef _MATERIAL_H__
#define _MATERIAL_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gpu.hpp>
#include <okay/core/renderer/shader.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/renderer/gl.hpp>

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>
#include <memory>

namespace okay {

class IMaterialPropertyCollection {
   public:
    virtual Failable init(ShaderHandle shader) = 0;
    virtual Failable pass(ShaderHandle shader) = 0;
};

class Material {
   public:
    static constexpr std::uint32_t invalidID() { return 0xFFFFFFFFu; }

    Material(ShaderHandle shader,
                 std::unique_ptr<IMaterialPropertyCollection> uniforms,
                 std::uint32_t id)
        : _shader(shader), _uniforms(std::move(uniforms)), _id(id) {}

    bool isNone() const { return _id == invalidID(); }

    std::uint32_t id() const { return _id; }

    std::uint32_t shaderID() const { return _shader->srcHash(); }

    GLuint programID() const { return _shader->programID(); }

    Failable setShader() {
        if (_shader->isNone()) {
            return Failable::errorResult("Material has no shader.");
        }

        if (_shader->state() == Shader::State::NOT_COMPILED) {
            Failable compile = _shader->compile();
            if (compile.isError()) {
                return compile;
            }
        }

        return _shader->set();
    }

    Failable passUniforms() {
        if (!_uniforms) {
            return Failable::errorResult("Material has no uniforms.");
        }

        if (_shader->isNone()) {
            return Failable::errorResult("Material has no shader.");
        }

        Failable set = setShader();
        if (set.isError()) {
            return set;
        }

        return _uniforms->pass(_shader);
    }

    // disable copy
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) = default;
    Material& operator=(Material&&) = default;

    // equality operator
    bool operator==(const Material& other) const {
        return _shader == other._shader && _id == other._id;
    }

    bool operator!=(const Material& other) const { return !(*this == other); }

    std::unique_ptr<IMaterialPropertyCollection>& uniforms() { return _uniforms; }

   private:
    ShaderHandle _shader;
    std::size_t _id{invalidID()};
    std::unique_ptr<IMaterialPropertyCollection> _uniforms;

    void setMaterial() {
        setShader();
        passUniforms();
    }
};

struct MaterialHandle {
    static MaterialHandle none() { return {nullptr, Material::invalidID()}; }

    MaterialRegistry* owner = nullptr;
    std::uint32_t id = Material::invalidID();

    bool isValid() const { return owner != nullptr && id != Material::invalidID(); }

    bool operator==(const MaterialHandle& other) const {
        return owner == other.owner && id == other.id;
    }

    bool operator!=(const MaterialHandle& other) const { return !(*this == other); }

    const std::unique_ptr<Material>& operator*() const;
    const std::unique_ptr<Material>& operator->() const;
    const std::unique_ptr<Material>& get() const;
};

class MaterialRegistry {
   public:
    ShaderHandle registerShader(const std::string& vertexSource,
                                    const std::string& fragmentSource) {
        // Create the shader, compile it, and add it to the registry
        Shader shader(vertexSource, fragmentSource);
        shader.compile();
        _shaders.emplace(shader.programID(), shader);
        return {this, shader.programID()};
    };

    MaterialHandle registerMaterial(const ShaderHandle& shader,
                                        std::unique_ptr<IMaterialPropertyCollection> uniforms) {
        std::uint32_t id = nextID();
        _materials.emplace_back(std::make_unique<Material>(shader, std::move(uniforms), id));
        return {this, id};
    }

    const std::unique_ptr<Material>& getMaterial(const MaterialHandle& handle) const {
        return _materials[handle.id];
    }

    const std::unique_ptr<Material>& getMaterial(std::uint32_t id) const {
        return _materials[id];
    }

    const Shader* getShader(const ShaderHandle& handle) const {
        return &_shaders.at(handle.id);
    }

    const Shader* getShader(GLuint programID) const { return &_shaders.at(programID); }

    Shader* getShader(const ShaderHandle& handle) { return &_shaders.at(handle.id); }

    Shader* getShader(GLuint programID) { return &_shaders.at(programID); }

    const Shader& getShader(const MaterialHandle& handle) const {
        return _shaders.at(handle.id);
    }

    Shader& getShader(const MaterialHandle& handle) { return _shaders.at(handle.id); }

    // equality
    bool operator==(const MaterialRegistry& other) const {
        return _materials == other._materials;
    }

   private:
    std::vector<std::unique_ptr<Material>> _materials;
    std::unordered_map<GLuint, Shader> _shaders;

    static std::uint32_t _materialID;
    static std::uint32_t nextID() { return _materialID++; }
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

        // textures (GPU manager owns GL textures + param cache)
        tupleForEach(d.textureRefs(), [&](auto& t) {
            GLuint loc = shader->findUniformLocation(t.name());
            if (loc == uni::inactiveLocation()) {
                return;  // optimized out / not present
            }

            auto r =
                gpu.textures.bindSampler2D(shader->programID(), loc, t.get(), t.params(), t.unit());
            if (r.isError()) {
                errorMessages << r.error() << '\n';
                anyErrors = true;
            }
        });

        return anyErrors ? Failable::errorResult(errorMessages.str()) : Failable::ok({});
    }

   private:
    bool _initialized{false};
};

};  // namespace okay

#endif  // _MATERIAL_H__