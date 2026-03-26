#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gpu.hpp>
#include <okay/core/renderer/shader.hpp>
#include <okay/core/renderer/texture.hpp>
#include <okay/core/renderer/uniform.hpp>
#include <okay/core/util/result.hpp>

#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

namespace okay {

class IOkayMaterialPropertyCollection {
   public:
    virtual Failable init(OkayShaderHandle shader) = 0;
    virtual Failable pass(OkayShaderHandle shader) = 0;
};

class OkayMaterial {
   public:
    static constexpr std::uint32_t invalidID() { return 0xFFFFFFFFu; }

    OkayMaterial(OkayShaderHandle shader,
                 std::unique_ptr<IOkayMaterialPropertyCollection> uniforms,
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

        if (_shader->state() == OkayShader::State::NOT_COMPILED) {
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
    OkayMaterial(const OkayMaterial&) = delete;
    OkayMaterial& operator=(const OkayMaterial&) = delete;
    OkayMaterial(OkayMaterial&&) = default;
    OkayMaterial& operator=(OkayMaterial&&) = default;

    // equality operator
    bool operator==(const OkayMaterial& other) const {
        return _shader == other._shader && _id == other._id;
    }

    bool operator!=(const OkayMaterial& other) const { return !(*this == other); }

    std::unique_ptr<IOkayMaterialPropertyCollection>& uniforms() { return _uniforms; }

   private:
    OkayShaderHandle _shader;
    std::size_t _id{invalidID()};
    std::unique_ptr<IOkayMaterialPropertyCollection> _uniforms;

    void setMaterial() {
        setShader();
        passUniforms();
    }
};

struct OkayMaterialHandle {
    static OkayMaterialHandle none() { return {nullptr, OkayMaterial::invalidID()}; }

    OkayMaterialRegistry* owner = nullptr;
    std::uint32_t id = OkayMaterial::invalidID();

    bool isValid() const { return owner != nullptr && id != OkayMaterial::invalidID(); }

    bool operator==(const OkayMaterialHandle& other) const {
        return owner == other.owner && id == other.id;
    }

    bool operator!=(const OkayMaterialHandle& other) const { return !(*this == other); }

    const std::unique_ptr<OkayMaterial>& operator*() const;
    const std::unique_ptr<OkayMaterial>& operator->() const;
    const std::unique_ptr<OkayMaterial>& get() const;
};

class OkayMaterialRegistry {
   public:
    OkayShaderHandle registerShader(const std::string& vertexSource,
                                    const std::string& fragmentSource) {
        // Create the shader, compile it, and add it to the registry
        OkayShader shader(vertexSource, fragmentSource);
        shader.compile();
        _shaders.emplace(shader.programID(), shader);
        return {this, shader.programID()};
    };

    OkayMaterialHandle registerMaterial(const OkayShaderHandle& shader,
                                        std::unique_ptr<IOkayMaterialPropertyCollection> uniforms) {
        std::uint32_t id = nextID();
        _materials.emplace_back(std::make_unique<OkayMaterial>(shader, std::move(uniforms), id));
        return {this, id};
    }

    const std::unique_ptr<OkayMaterial>& getMaterial(const OkayMaterialHandle& handle) const {
        return _materials[handle.id];
    }

    const std::unique_ptr<OkayMaterial>& getMaterial(std::uint32_t id) const {
        return _materials[id];
    }

    const OkayShader* getShader(const OkayShaderHandle& handle) const {
        return &_shaders.at(handle.id);
    }

    const OkayShader* getShader(GLuint programID) const { return &_shaders.at(programID); }

    OkayShader* getShader(const OkayShaderHandle& handle) { return &_shaders.at(handle.id); }

    OkayShader* getShader(GLuint programID) { return &_shaders.at(programID); }

    const OkayShader& getShader(const OkayMaterialHandle& handle) const {
        return _shaders.at(handle.id);
    }

    OkayShader& getShader(const OkayMaterialHandle& handle) { return _shaders.at(handle.id); }

    // equality
    bool operator==(const OkayMaterialRegistry& other) const {
        return _materials == other._materials;
    }

   private:
    std::vector<std::unique_ptr<OkayMaterial>> _materials;
    std::unordered_map<GLuint, OkayShader> _shaders;

    static std::uint32_t _materialID;
    static std::uint32_t nextID() { return _materialID++; }
};

template <class Derived>
class OkayMaterialProperties : public IOkayMaterialPropertyCollection {
   public:
    Failable init(OkayShaderHandle shader) override {
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

    Failable pass(OkayShaderHandle shader) override {
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

        auto& gpu = OkayGpuState::instance();

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

#endif  // __OKAY_MATERIAL_H__