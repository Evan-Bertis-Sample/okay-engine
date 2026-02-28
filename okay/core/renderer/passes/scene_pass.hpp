#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include <memory>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "okay/core/renderer/okay_uniform.hpp"

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
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        // glEnable(GL_DEPTH_TEST);

        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            OkayRenderItem& item = context.world.getRenderItem(handle);

            if (_shaderIndex != item.material.get().shaderID()) {
                _shaderIndex = item.material.get().shaderID();
                Failable f = item.material.get().setShader();
                Engine.logger.info("Shader: {}", item.material.get().shaderID());
                if (f.isError()) {
                    Engine.logger.error("Failed to set shader : {}", f.error());
                }
            }

            if (_materialIndex != item.material.get().id()) {
                _materialIndex = item.material.get().id();

                std::unique_ptr<IOkayMaterialUniformCollection> &uniforms = item.material.get().uniforms();

                // cast to BaseMaterialUniforms
                okay::BaseMaterialUniforms *baseUniforms =
                    dynamic_cast<okay::BaseMaterialUniforms *>(uniforms.get());

                if (baseUniforms) {
                    Engine.logger.info("Passing base uniforms for material {}.", item.material.get().id());
                    baseUniforms->modelMatrix.set(item.worldMatrix);
                    float aspect = static_cast<float>(context.renderer.width()) / static_cast<float>(context.renderer.height());
                    baseUniforms->projectionMatrix.set(context.world.camera().projectionMatrix(aspect));
                    baseUniforms->modelMatrix.set(glm::identity<glm::mat4>());
                    baseUniforms->viewMatrix.set(context.world.camera().viewMatrix());
                    // set evertything to the identity
                    // baseUniforms->viewMatrix.set(glm::identity<glm::mat4>());
                    // baseUniforms->projectionMatrix.set(glm::identity<glm::mat4>());
                }

                Failable f = item.material.get().passUniforms();
                Engine.logger.info("Material: {}", item.material.get().id());
                if (f.isError()) {
                    Engine.logger.error("Failed to pass uniforms : {}", f.error());
                }

            }

            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
    }

   private:
    std::uint32_t _shaderIndex{OkayShader::invalidID()};
    std::uint32_t _materialIndex{OkayMaterial::invalidID()};
};

};  // namespace okay

#endif  // __SCENE_PASS_H__