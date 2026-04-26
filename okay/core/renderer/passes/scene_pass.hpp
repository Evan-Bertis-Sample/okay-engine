#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include "glm/ext/matrix_transform.hpp"
#include "okay/core/renderer/render_world.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/materials/lit.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/render_pipeline.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/renderer/uniform.hpp>

#include <memory>

namespace okay {

class ScenePass : public IRenderPass {
   public:
    ScenePass() {}

    virtual const std::string_view name() const override {
        return "ScenePass";
    }

    virtual void initialize() override {}

    virtual void resize(int newWidth, int newHeight) override {}

    virtual void render(const RendererContext& context) override {
        GL_CHECK(glClearColor(0.113f, 0.008, 0.208, 1.0f));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glEnable(GL_DEPTH_TEST));
        GL_CHECK(glCullFace(GL_BACK));
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glDisable(GL_BLEND));
        GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        // doesn't need MSAA, but should if the platform can support it
        glEnable(GL_MULTISAMPLE);
        // glFrontFace(GL_CW);

        _shaderIndex = Shader::invalidID();
        _materialIndex = Material::invalidID();

        _ndcProjectionPmatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

        float aspect = float(context.renderer.width()) / float(context.renderer.height());
        auto view = context.world.camera().viewMatrix();
        auto projection = context.world.camera().projectionMatrix(aspect);
        auto camPos = context.world.camera().position();
        auto camDir = context.world.camera().direction();

        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            RenderItem& item = context.world.getRenderItem(handle);
            if (item.mesh.isEmpty())
                continue;
            if (item.material->isNone())
                continue;

            Camera& camera = context.world.camera();
            bool isScreenSpace =
                item.material->properties()->flags().hasFlag(MaterialFlags::SCREEN_SPACE);
            if (!isScreenSpace &&
                !camera.isInFrustum(item.mesh.bounds.transform(item.worldMatrix), aspect)) {
                continue;
            }

            // Shader switch: bind program + per-frame stuff
            if (_materialIndex != item.material->id()) {
                handleMaterialSwitch(context, item, projection, view, camPos, camDir);
                _materialIndex = item.material->id();
            }

            // Set per-object uniforms
            setPerObjectUniforms(item);

            // Now push uniforms for this draw (or only the ones you changed)
            Failable f = item.material->passUniforms();
            if (f.isError())
                Engine.logger.error("Failed to pass uniforms : {}", f.error());

            // Engine.logger.info("Drawing item with transform {}", item.transform);

            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
    }

    void handleMaterialSwitch(const RendererContext& context,
        RenderItem& item,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::vec3& camPos,
        const glm::vec3& camDir) {
        applyMaterialFlags(item);

        if (_shaderIndex != item.material->shaderID()) {
            if (auto f = item.material->setShader(); f.isError()) {
                Engine.logger.error("Failed to set shader : {}", f.error());
                return;
            }
            _shaderIndex = item.material->shaderID();
        }

        auto& uniforms = item.material->properties();
        MaterialFlagCollection flags = uniforms->flags();

        if (auto* unlit = dynamic_cast<okay::SceneMaterialProperties*>(uniforms.get())) {
            if (flags.hasFlag(MaterialFlags::SCREEN_SPACE)) {
                unlit->projectionMatrix.set(_ndcProjectionPmatrix);
                unlit->viewMatrix.set(glm::identity<glm::mat4>());
            } else {
                unlit->projectionMatrix.set(projection);
                unlit->viewMatrix.set(view);
            }
            unlit->cameraPosition.set(camPos);
            unlit->cameraDirection.set(camDir);
        }

        if (auto* lit = dynamic_cast<okay::LitMaterial*>(uniforms.get())) {
            // per-frame lighting setup
            DefaultLightBlock& block = lit->lights.edit();
            block.meta.x = static_cast<float>(context.world.lights().size());
            std::size_t i = 0;
            for (auto l : context.world.lights()) {
                block.lights[i++] = l;
            }
        }
    }

    void setPerObjectUniforms(RenderItem& item) {
        auto& uniforms = item.material->properties();
        if (auto* unlit = dynamic_cast<okay::SceneMaterialProperties*>(uniforms.get())) {
            unlit->modelMatrix.set(item.worldMatrix);
        }
    }

    void applyMaterialFlags(RenderItem& item) {
        auto& uniforms = item.material->properties();
        MaterialFlagCollection flags = uniforms->flags();

        if (flags.hasFlag(MaterialFlags::DOUBLE_SIDED)) {
            GL_CHECK(glDisable(GL_CULL_FACE));
        } else {
            GL_CHECK(glEnable(GL_CULL_FACE));
            GL_CHECK(glCullFace(GL_BACK));
        }

        if (flags.hasFlag(MaterialFlags::TRANSPARENT)) {
            GL_CHECK(glEnable(GL_BLEND));
            GL_CHECK(glDepthMask(GL_FALSE));
        } else {
            GL_CHECK(glDisable(GL_BLEND));
            GL_CHECK(glDepthMask(GL_TRUE));
        }

        if (flags.hasFlag(MaterialFlags::SCREEN_SPACE)) {
            glDisable(GL_DEPTH_TEST);
        } else {
            glEnable(GL_DEPTH_TEST);
        }
    }

   private:
    std::uint32_t _shaderIndex{Shader::invalidID()};
    std::uint32_t _materialIndex{Material::invalidID()};

    glm::mat4 _ndcProjectionPmatrix{};
};

};  // namespace okay

#endif  // __SCENE_PASS_H__