#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

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

    virtual const std::string_view name() const override { return "ScenePass"; }

    virtual void initialize() override {
        Shader depthMapShader = load::engineShader("shaders/depth_map");
        auto depthMapProperties = std::make_unique<DepthMapMaterial>();
        Renderer * r = Engine.systems.getSystemChecked<Renderer>();
        ShaderHandle shader = r->materialRegistry().registerShader(depthMapShader.vertexShader,
                                                                depthMapShader.fragmentShader);
        _depthMapMaterial = r->materialRegistry().registerMaterial(shader, std::move(depthMapProperties));
    }

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

        float aspect = float(context.renderer.width()) / float(context.renderer.height());
        auto view = context.world.camera().viewMatrix();
        auto projection = context.world.camera().projectionMatrix(aspect);
        auto camPos = context.world.camera().position();
        auto camDir = context.world.camera().direction();


        for (const Light& l : context.world.lights()) {
            switch (l.type()) {
                case Light::Type::POINT:
                case Light::Type::SPOT:
                    continue;
                case Light::Type::DIRECTIONAL:
                    glm::vec4 direction = l.direction;
                    glm::vec4 color = l.color;

                    // handle direction light -- add shadow mappin

                    // Generating Depth Map
                    unsigned int depthMapFBO;
                    glGenFramebuffers(1, &depthMapFBO);  

                    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

                    unsigned int depthMap;
                    glGenTextures(1, &depthMap);
                    glBindTexture(GL_TEXTURE_2D, depthMap);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  

                    
                    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
                    glDrawBuffer(GL_NONE);
                    glReadBuffer(GL_NONE);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

                    float nearPlane = 7.5f, farPlane = 100.0f;
                    

                    // calculate frustum based on camera and give an area slightly larger than it, only when the camera goes outside the area
                    glm::mat4 lightProjection = glm::ortho(100.0f, 100.0f, 100.0f, 100.0f, nearPlane, farPlane); 

                    // use light position and direction
                    glm::vec4 lightPos = l.posType;
                    glm::vec4 lightDirection = l.direction;
                    glm::mat4 lightView = glm::lookAt(glm::vec3(lightPos.x, lightPos.y, lightPos.z), 
                                  glm::vec3(lightPos.x + lightDirection.x, lightPos.y + lightDirection.y, lightPos.z + lightDirection.z), 
                                  glm::vec3( 0.0f, 1.0f,  0.0f));
                    glm::mat4 lightSpaceMatrix = lightProjection * lightView; 
                    
                    if (DepthMapMaterial * depthMapProperties = dynamic_cast<DepthMapMaterial*>(_depthMapMaterial->properties().get())) {
                        depthMapProperties->lightSpaceMatrix.set(lightSpaceMatrix);
                    }
                    _depthMapMaterial->setShader();
                    
                   
                    // First pass, render to depth map
                    for (const RenderItemHandle& handle : context.world.getRenderItems()) {
                        RenderItem& item = context.world.getRenderItem(handle);
                        if (item.mesh.isEmpty())
                            continue;
                        
                        
                        
                        if (DepthMapMaterial * depthMapProperties = dynamic_cast<DepthMapMaterial*>(_depthMapMaterial->properties().get())) {
                            depthMapProperties->modelMatrix.set(item.worldMatrix);
                        }
                        
                        _depthMapMaterial->passUniforms();
                        context.renderer.meshBuffer().drawMesh(item.mesh);
                    }
                    
                    
                    

                    // Second pass, render scene with depth map
                    for (const RenderItemHandle& handle : context.world.getRenderItems()) {

                        // set material to depth map material and render to depth map
                        
                        RenderItem& item = context.world.getRenderItem(handle);
                        if (item.mesh.isEmpty())
                            continue;
                        if (item.material->isNone())
                            continue;
                        item.material = _depthMapMaterial;

                        
                        item.material->passUniforms();
                        context.renderer.meshBuffer().drawMesh(item.mesh);
                    }

                    break; // only render one directional light
            }
        }

        // for (const RenderItemHandle& handle : context.world.getRenderItems()) {
        //     RenderItem& item = context.world.getRenderItem(handle);
        //     if (item.mesh.isEmpty())
        //         continue;
        //     if (item.material->isNone())
        //         continue;

        //     Camera& camera = context.world.camera();
        //     if (!camera.isInFrustum(item.mesh.bounds.transform(item.worldMatrix), aspect)) {
        //         continue;
        //     }

        //     // Shader switch: bind program + per-frame stuff
        //     if (_materialIndex != item.material->id()) {
        //         handleMaterialSwitch(context, item, projection, view, camPos, camDir);
        //         _materialIndex = item.material->id();
        //     }

        //     // Set per-object uniforms
        //     setPerObjectUniforms(item);

        //     // Now push uniforms for this draw (or only the ones you changed)
        //     Failable f = item.material->passUniforms();
        //     if (f.isError())
        //         Engine.logger.error("Failed to pass uniforms : {}", f.error());

        //     context.renderer.meshBuffer().drawMesh(item.mesh);
        // }


        // for (const RenderItemHandle& handle : context.world.getRenderItems()) {
        //     RenderItem& item = context.world.getRenderItem(handle);
        //     if (item.mesh.isEmpty())
        //         continue;
        //     if (item.material->isNone())
        //         continue;

        //     Camera& camera = context.world.camera();
        //     if (!camera.isInFrustum(item.mesh.bounds.transform(item.worldMatrix), aspect)) {
        //         continue;
        //     }

        //     // Shader switch: bind program + per-frame stuff
        //     if (_materialIndex != item.material->id()) {
        //         handleMaterialSwitch(context, item, projection, view, camPos, camDir);
        //         _materialIndex = item.material->id();
        //     }

        //     // Set per-object uniforms
        //     setPerObjectUniforms(item);

        //     // Now push uniforms for this draw (or only the ones you changed)
        //     Failable f = item.material->passUniforms();
        //     if (f.isError())
        //         Engine.logger.error("Failed to pass uniforms : {}", f.error());

        //     context.renderer.meshBuffer().drawMesh(item.mesh);
        // }
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

        if (auto* unlit = dynamic_cast<okay::SceneMaterialProperties*>(uniforms.get())) {
            unlit->projectionMatrix.set(projection);
            unlit->viewMatrix.set(view);
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

             glBindFramebuffer(depthMapFBO, lit->shadowMap.get().getGLTextureID());
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
    }

   private:
    std::uint32_t _shaderIndex{Shader::invalidID()};
    std::uint32_t _materialIndex{Material::invalidID()};
};

};  // namespace okay

#endif  // __SCENE_PASS_H__