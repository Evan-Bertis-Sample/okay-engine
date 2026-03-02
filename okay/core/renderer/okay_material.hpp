#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <memory>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_shader.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/okay.hpp>

namespace okay {

class IOkayMaterialPropertyCollection {
   public:
    virtual Failable init(OkayShaderHandle shader) = 0;
    virtual Failable pass(OkayShaderHandle shader) = 0;
    virtual bool markDirty() = 0;
    virtual bool foundLocations() const = 0;
};

class OkayMaterial {
   public:
    static constexpr std::uint32_t invalidID() {
        return 0xFFFFFFFFu;
    }

    OkayMaterial(OkayShaderHandle shader,
                 std::unique_ptr<IOkayMaterialPropertyCollection> uniforms,
                 std::uint32_t id)
        : _shader(shader), _uniforms(std::move(uniforms)), _id(id) {
    }

    bool isNone() const {
        return _id == invalidID();
    }

    std::uint32_t id() const {
        return _id;
    }

    std::uint32_t shaderID() const {
        return _shader->srcHash();
    }

    GLuint programID() const {
        return _shader->programID();
    }

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

        if (!_uniforms->foundLocations()) {
            Engine.logger.debug("Finding uniform locations for material {}.", _id);
            Failable find = _uniforms->init(_shader);
            if (find.isError()) {
                return find;
            }
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

    bool operator!=(const OkayMaterial& other) const {
        return !(*this == other);
    }

    std::unique_ptr<IOkayMaterialPropertyCollection>& uniforms() {
        return _uniforms;
    }

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
    OkayMaterialRegistry* owner = nullptr;
    std::uint32_t id = OkayMaterial::invalidID();

    bool isValid() const {
        return owner != nullptr && id != OkayMaterial::invalidID();
    }

    bool operator==(const OkayMaterialHandle& other) const {
        return owner == other.owner && id == other.id;
    }

    bool operator!=(const OkayMaterialHandle& other) const {
        return !(*this == other);
    }

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

    const OkayShader* getShader(GLuint programID) const {
        return &_shaders.at(programID);
    }

    OkayShader* getShader(const OkayShaderHandle& handle) {
        return &_shaders.at(handle.id);
    }

    OkayShader* getShader(GLuint programID) {
        return &_shaders.at(programID);
    }

    const OkayShader& getShader(const OkayMaterialHandle& handle) const {
        return _shaders.at(handle.id);
    }

    OkayShader& getShader(const OkayMaterialHandle& handle) {
        return _shaders.at(handle.id);
    }

    // equality
    bool operator==(const OkayMaterialRegistry& other) const {
        return _materials == other._materials;
    }

   private:
    std::vector<std::unique_ptr<OkayMaterial>> _materials;
    std::unordered_map<GLuint, OkayShader> _shaders;

    static std::uint32_t _materialID;
    static std::uint32_t nextID() {
        std::cout << "Material ID: " << _materialID << std::endl;
        return _materialID++;
    }
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
        // init uniform blocks (bind block->binding point)
        tupleForEach(d.uniformBlockRefs(), [&](auto& b) {
            if (out.isError())
                return;
            auto r = b.init(shader->programID());
            if (r.isError())
                out = r;
        });

        _initialized = true;
        return out;
    }

    Failable pass(OkayShaderHandle shader) override {
        auto& d = static_cast<Derived&>(*this);
        std::stringstream errorMessages;
        bool anyErrors = false;

        tupleForEach(d.uniformRefs(), [&](auto& u) {
            auto r = shader->setUniform(u.name(), u.get());
            if (r.isError() && !_hasPassed) {
                errorMessages << r.error() << std::endl;
                anyErrors = true;
            }
        });
        tupleForEach(d.uniformBlockRefs(), [&](auto& b) {
            auto r = b.pass(shader->programID());
            if (r.isError() && !_hasPassed) {
                errorMessages << r.error() << std::endl;
                anyErrors = true;
            }
        });

        Failable out = anyErrors ? Failable::errorResult(errorMessages.str()) : Failable::ok({});

        // only return an error if you have not passed before
        if (!_hasPassed) {
            _hasPassed = true;
            return out;
        }

        return Failable::ok({});
    }

    bool markDirty() override {
        auto& d = static_cast<Derived&>(*this);
        bool any = false;
        tupleForEach(d.uniformRefs(), [&](auto& u) {
            u.markDirty();
            any = true;
        });

        return any;
    }

    bool foundLocations() const override {
        return _initialized;
    }

   private:
    bool _initialized{false};
    bool _hasPassed{false};
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__