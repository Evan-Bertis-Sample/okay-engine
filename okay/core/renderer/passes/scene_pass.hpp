#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_renderer.hpp>

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