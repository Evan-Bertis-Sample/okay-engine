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
        GL_CHECK(glClearColor(0.113f, 0.008, 0.208, 1.0f));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glCullFace(GL_BACK));
        GL_CHECK(glEnable(GL_DEPTH_TEST));
        GL_CHECK(glEnable(GL_MULTISAMPLE));

        _shaderIndex = OkayShader::invalidID();
        _materialIndex = OkayMaterial::invalidID();

        float aspect = float(context.renderer.width()) / float(context.renderer.height());
        auto view = context.world.camera().viewMatrix();
        auto projection = context.world.camera().projectionMatrix(aspect);
        auto camPos = context.world.camera().position();
        auto camDir = context.world.camera().direction();

        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            OkayRenderItem& item = context.world.getRenderItem(handle);

            if (item.mesh.isEmpty()) continue;
            if (item.material->isNone()) continue;

            // Shader switch: bind program + per-frame stuff
            // if (_shaderIndex != item.material->shaderID()) {
                bindShaderAndPerFrame(context, item, projection, view, camPos, camDir);
            // }

            _materialIndex = item.material->id();

            // Set per-object uniforms
            setPerObjectUniforms(item);

            // Now push uniforms for this draw (or only the ones you changed)
            Failable f = item.material->passUniforms();
            if (f.isError())
                Engine.logger.error("Failed to pass uniforms : {}", f.error());

            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
    }

    void bindShaderAndPerFrame(const OkayRenderContext& context,
                               OkayRenderItem& item,
                               const glm::mat4& projection,
                               const glm::mat4& view,
                               const glm::vec3& camPos,
                               const glm::vec3& camDir) {
        _shaderIndex = item.material->shaderID();

        if (auto f = item.material->setShader(); f.isError()) {
            Engine.logger.error("Failed to set shader : {}", f.error());
            return;
        }

        auto& uniforms = item.material->uniforms();

        if (auto* unlit = dynamic_cast<okay::UnlitMaterial*>(uniforms.get())) {
            unlit->projectionMatrix.set(projection);
            unlit->viewMatrix.set(view);
            unlit->cameraPosition.set(camPos);
            unlit->cameraDirection.set(camDir);
        }

        if (auto* lit = dynamic_cast<okay::LitMaterial*>(uniforms.get())) {
            // per-frame lighting setup
            OkayLight light = OkayLight::directional(glm::vec3(0, 0, 1), glm::vec3(1, 1, 1), 1.0f);
            DefaultLightBlock& block = lit->lights.edit();
            block.lights[0] = light;
            block.meta.x = 1.0f;
        }
    }

    void setPerObjectUniforms(OkayRenderItem& item) {
        auto& uniforms = item.material->uniforms();
        if (auto* unlit = dynamic_cast<okay::UnlitMaterial*>(uniforms.get())) {
            unlit->modelMatrix.set(item.worldMatrix);
        }
    }

   private:
    std::uint32_t _shaderIndex{OkayShader::invalidID()};
    std::uint32_t _materialIndex{OkayMaterial::invalidID()};
};

};  // namespace okay

#endif  // __SCENE_PASS_H__