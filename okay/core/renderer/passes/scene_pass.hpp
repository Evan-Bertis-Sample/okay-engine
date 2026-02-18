#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_renderer.hpp>

namespace okay {

class ScenePass : public IOkayRenderPass {
   public:
    ScenePass() {}

    virtual const std::string_view name() const override {
        return "ScenePass";
    }

    virtual void initialize() override {
    }
    virtual void resize(int newWidth, int newHeight) override {
    }

    virtual void render(const OkayRenderContext& context) override {
        std::uint32_t shaderIndex = OkayShader::invalidID();
        std::uint32_t materialIndex = OkayMaterial::invalidID();

        for (const RenderItemHandle &handle : context.world.getRenderItems()) {
            OkayRenderItem& item = context.world.getRenderItem(handle);

            if (shaderIndex != item.material.get().shaderID()) {
                shaderIndex = item.material.get().shaderID();
                item.material.get().setShader();
                Engine.logger.info("Shader: {}", item.material.get().shaderID());
            }

            if (materialIndex != item.material.get().id()) {
                materialIndex = item.material.get().id();
                item.material.get().passUniforms();
                Engine.logger.info("Material: {}", item.material.get().id());
            }

            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
    }
};

};  // namespace okay

#endif  // __SCENE_PASS_H__