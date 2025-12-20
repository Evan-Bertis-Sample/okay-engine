#ifndef __OKAY_MATERIAL_H__
#define __OKAY_MATERIAL_H__

#include <cstdint>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/util/result.hpp>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace okay {

struct OkayShader {
   public:
    enum ShaderState { NOT_COMPILED, STANDBY, IN_USE };

    OkayShader(const std::string vertexSrc, const std::string fragmentSrc)
        : vertexShader(std::move(vertexSrc)),
          fragmentShader(std::move(fragmentSrc)),
          shaderProgram(0),
          _state(NOT_COMPILED) {
        std::hash<std::string> hasher;
        _id = hasher(vertexShader + fragmentShader);
    }

    OkayShader()
        : vertexShader(""), fragmentShader(""), shaderProgram(0), _state(NOT_COMPILED), _id(1) {}

    std::string vertexShader;
    std::string fragmentShader;

    Failable compile();
    Failable set();
    ShaderState state() const { return _state; }
    std::uint32_t id() const { return _id; }

   private:
    unsigned int shaderProgram;
    ShaderState _state;
    std::size_t _id;
};

class IOkayMaterial {
   public:
    virtual std::uint32_t id() const { return 0; }
    virtual std::uint32_t shaderID() const { return 0; }
    virtual void setShader() {}
    virtual void passUniforms() {}
};

using OkayEngineUniforms =
    OkayMaterialUniformCollection<OkayMaterialUniform<glm::mat4, FixedString("u_Model")>,
                                  OkayMaterialUniform<glm::mat4, FixedString("u_View")>,
                                  OkayMaterialUniform<glm::mat4, FixedString("u_Projection")>>;

template <class... Uniforms>
class OkayMaterial : public IOkayMaterial {
   public:
    OkayEngineUniforms engineUniforms;
    OkayMaterialUniformCollection<Uniforms...> uniforms;

    OkayMaterial(const OkayShader& shader) : _shader(shader) {
        // hash the names of the uniforms + shader id to create a unique material id
        std::hash<std::string> hasher;
        std::string combined;
        ((combined += std::string(Uniforms::name.sv()) + ";"), ...);
        combined += std::to_string(shader.id());
        _id = hasher(combined);
    }

    std::uint32_t id() const override { return _id; }
    std::uint32_t shaderID() const override { return _shader.id(); }

    void setShader() override {
        if (_shader.state() != OkayShader::IN_USE) {
            _shader.set();
        }
    }

    void passUniforms() override {
        uniforms.setUniforms();
        engineUniforms.setUniforms();
    }

   private:
    OkayShader _shader;
    std::size_t _id{0};
};

};  // namespace okay

#endif  // __OKAY_MATERIAL_H__