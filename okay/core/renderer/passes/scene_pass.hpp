#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include <memory>
#include <okay/core/renderer/okay_gl.hpp>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/materials/lit.hpp>

namespace okay {

class ScenePass : public IOkayRenderPass {
   public:
    ScenePass() {
    }

    virtual const std::string_view name() const override {
        return "ScenePass";
    }

    virtual void initialize() override {
    }

    virtual void resize(int newWidth, int newHeight) override {
    }

    virtual void render(const OkayRenderContext& context) override {
        // enable culling and depth test
        GL_CHECK(glClearColor(0.113f, 0.008, 0.208, 1.0f));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glCullFace(GL_BACK));
        GL_CHECK(glEnable(GL_DEPTH_TEST));

        // enable anti-aliasing
        GL_CHECK(glEnable(GL_MULTISAMPLE));

        _shaderIndex = OkayShader::invalidID();
        _materialIndex = OkayMaterial::invalidID();

        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            OkayRenderItem& item = context.world.getRenderItem(handle);

            if (_shaderIndex != item.material.get().shaderID()) {
                handleShaderSwitch(context, item);
            }

            if (_materialIndex != item.material.id) {
                handleMaterialSwitch(item);
            }
            context.renderer.meshBuffer().drawMesh(item.mesh);
        }

        _shaderIndex = OkayShader::invalidID();
        _materialIndex = OkayMaterial::invalidID();
    }

    void handleShaderSwitch(const OkayRenderContext& context, OkayRenderItem& item) {
        _shaderIndex = item.material.get().shaderID();
        Failable f = item.material.get().setShader();
        if (f.isError()) {
            Engine.logger.error("Failed to set shader : {}", f.error());
        }

        std::unique_ptr<IOkayMaterialPropertyCollection>& uniforms = item.material.get().uniforms();

        // cast to BaseMaterialUniforms
        okay::UnlitMaterial* baseUniforms = dynamic_cast<okay::UnlitMaterial*>(uniforms.get());

        if (baseUniforms) {
            baseUniforms->modelMatrix.set(item.worldMatrix);
            float aspect = static_cast<float>(context.renderer.width()) /
                           static_cast<float>(context.renderer.height());
            baseUniforms->projectionMatrix.set(context.world.camera().projectionMatrix(aspect));
            baseUniforms->viewMatrix.set(context.world.camera().viewMatrix());
            baseUniforms->cameraPosition.set(context.world.camera().position());
            baseUniforms->cameraDirection.set(context.world.camera().direction());
        }

        // pass lit material uniforms
        okay::LitMaterial* litUniforms = dynamic_cast<okay::LitMaterial*>(uniforms.get());

        if (litUniforms) {
            // pass some lights
            OkayLight light = OkayLight::directional(glm::vec3(0, 0, 1), glm::vec3(1, 1, 1), 1.0f);

            DefaultLightBlock& block = litUniforms->lights.edit();
            block.lights[0] = light;
            block.meta.x = 1.0f;
        }

        f = item.material.get().passUniforms();
        if (f.isError()) {
            Engine.logger.error("Failed to pass uniforms : {}", f.error());
        }
    }

    void handleMaterialSwitch(OkayRenderItem& item) {
        _materialIndex = item.material.get().id();

        Failable f = item.material.get().passUniforms();
        if (f.isError()) {
            Engine.logger.error("Failed to pass uniforms : {}", f.error());
        }
    }

   private:
    std::uint32_t _shaderIndex{OkayShader::invalidID()};
    std::uint32_t _materialIndex{OkayMaterial::invalidID()};
};

};  // namespace okay

#endif  // __SCENE_PASS_H__