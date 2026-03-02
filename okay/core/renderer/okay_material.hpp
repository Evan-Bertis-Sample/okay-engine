#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <memory>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/util/result.hpp>
#include <okay/core/okay.hpp>

namespace okay {

enum class ShaderState { NOT_COMPILED, STANDBY };

class OkayShader;
class OkayMaterial;

class OkayShader {
   public:
    static constexpr std::uint32_t invalidID() {
        return 0xFFFFFFFFu;
    }

    OkayShader(const std::string& vertexSource, const std::string& fragmentSource)
        : vertexShader(std::move(vertexSource)),
          fragmentShader(std::move(fragmentSource)),
          _shaderProgram(0),
          _state(ShaderState::NOT_COMPILED) {
        std::hash<std::string> hasher;
        _id = hasher(vertexShader + fragmentShader);
    }

    OkayShader()
        : vertexShader(""),
          fragmentShader(""),
          _shaderProgram(0),
          _state(ShaderState::NOT_COMPILED),
          _id(invalidID()) {
    }

    std::string vertexShader;
    std::string fragmentShader;

    Failable compile();
    Failable set();

    bool isNone() const {
        return _id == invalidID();
    }

    ShaderState state() const {
        return _state;
    }

    std::uint32_t id() const {
        return _id;
    }

    GLuint programID() const {
        return _shaderProgram;
    }

    // enable equality operator
    bool operator==(const OkayShader& other) const {
        return _shaderProgram == other._shaderProgram && _state == other._state && _id == other._id;
    }

    // enable inequality operator
    bool operator!=(const OkayShader& other) const {
        return !(*this == other);
    }

   private:
    GLuint _shaderProgram;
    ShaderState _state;
    std::size_t _id;
};

class OkayMaterialRegistry;

class OkayMaterial {
   public:
    static constexpr std::uint32_t invalidID() {
        return 0xFFFFFFFFu;
    }

    OkayMaterial(std::shared_ptr<OkayShader> shader,
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
        return _shader->id();
    }

    GLuint programID() const {
        return _shader->programID();
    }

    Failable setShader() {
        if (_shader->isNone()) {
            return Failable::errorResult("Material has no shader.");
        }

        if (_shader->state() == ShaderState::NOT_COMPILED) {
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
            Failable find = _uniforms->init(_shader->programID());
            if (find.isError()) {
                return find;
            }
        }
        return _uniforms->pass(_shader->programID());
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

    std::unique_ptr<IOkayMaterialPropertyCollection> &uniforms() {
        return _uniforms;
    }

   private:
    std::shared_ptr<OkayShader> _shader;
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

    bool isValid() const { return owner != nullptr && id != OkayMaterial::invalidID(); }

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
    OkayMaterialHandle registerMaterial(std::shared_ptr<OkayShader> shader,
                                   std::unique_ptr<IOkayMaterialPropertyCollection> uniforms) {
        std::uint32_t id = nextID();
        _materials.emplace_back(std::make_unique<OkayMaterial>(shader, std::move(uniforms), id));
        return { this, id };
    }

    const std::unique_ptr<OkayMaterial> &getMaterial(const OkayMaterialHandle& handle) const {
        return _materials[handle.id];
    }

    const std::unique_ptr<OkayMaterial> &getMaterial(std::uint32_t id) const {
        return _materials[id];
    }

    // equality
    bool operator==(const OkayMaterialRegistry& other) const {
        return _materials == other._materials;
    }

   private:
    std::vector<std::unique_ptr<OkayMaterial>> _materials;

    static std::uint32_t _idCounter;
    static std::uint32_t nextID() {
        std::cout << "Material ID: " << _idCounter << std::endl;
        return _idCounter++;
    }
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__