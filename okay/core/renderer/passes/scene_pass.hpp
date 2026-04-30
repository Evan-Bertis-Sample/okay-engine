#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include "okay/core/renderer/render_world.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/materials/lit.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/materials/depth_map.hpp>
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
        glGenFramebuffers(1, &_depthMapFBO); 

        glGenTextures(1, &_depthMap);
        glBindTexture(GL_TEXTURE_2D, _depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                    SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  

        glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
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
                case Light::Type::DIRECTIONAL: {
                    glm::vec3 lightDir = glm::normalize(glm::vec3(l.direction));
                    glm::vec3 sceneCenter(0.0f);
                    glm::vec3 lightEye = sceneCenter - lightDir * 5.0f;
                    
                    float nearPlane = 0.1f, farPlane = 15.0f;
                    glm::mat4 lightProjection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, nearPlane, farPlane);
                    glm::mat4 lightView = glm::lookAt(lightEye, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
                    _lightSpaceMatrix = lightProjection * lightView;

                    // --- Depth pass ---
                    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                    glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
                    glClear(GL_DEPTH_BUFFER_BIT);

                    if (auto f = _depthMapMaterial->setShader(); f.isError()) {
                        Engine.logger.error("Failed to set depth shader: {}", f.error());
                    }

                    auto* depthProps = dynamic_cast<DepthMapMaterial*>(_depthMapMaterial->properties().get());
                    if (depthProps) depthProps->lightSpaceMatrix.set(_lightSpaceMatrix);

                    for (const RenderItemHandle& handle : context.world.getRenderItems()) {
                        RenderItem& item = context.world.getRenderItem(handle);
                        if (item.mesh.isEmpty()) continue;

                        if (depthProps) depthProps->modelMatrix.set(item.worldMatrix);
                        _depthMapMaterial->passUniforms();
                        context.renderer.meshBuffer().drawMesh(item.mesh);
                    }

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glViewport(0, 0, context.renderer.width(), context.renderer.height());
                    break;
                }
            }
        }

        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            RenderItem& item = context.world.getRenderItem(handle);
            if (item.mesh.isEmpty())
                continue;
            if (item.material->isNone())
                continue;

            Camera& camera = context.world.camera();
            if (!camera.isInFrustum(item.mesh.bounds.transform(item.worldMatrix), aspect)) {
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

            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
        displayDepthMap(context);

    }

    void displayDepthMap(const RendererContext& context) {
        static GLuint program = 0;
        static GLuint depthmapVAO = 0;
        if (program == 0) {
            const char* depthVert = R"(
                #version 330 core
                out vec2 TexCoords;
                void main() {
                    TexCoords = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
                    gl_Position = vec4(TexCoords * 2.0 - 1.0, 0.0, 1.0);
                }
            )";
            const char* depthFrag = R"(
                #version 330 core
                in vec2 TexCoords;
                out vec4 FragColor;
                uniform sampler2D depthMap;
                void main() {
                    float d = texture(depthMap, TexCoords).r;
                    FragColor = vec4(vec3(d), 1.0);
                }
            )";
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vs, 1, &depthVert, nullptr); glCompileShader(vs);
            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fs, 1, &depthFrag, nullptr); glCompileShader(fs);
            program = glCreateProgram();
            glAttachShader(program, vs);
            glAttachShader(program, fs);
            glLinkProgram(program);
            glDeleteShader(vs); glDeleteShader(fs);
            glGenVertexArrays(1, &depthmapVAO);
        }
        
        int w = context.renderer.width();
        int h = context.renderer.height();
        glViewport(0, 0, w / 3, h / 3);  // bottom-left, 1/3 of screen
        
        glDisable(GL_DEPTH_TEST);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _depthMap);
        glUniform1i(glGetUniformLocation(program, "depthMap"), 0);
        
        glBindVertexArray(depthmapVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glViewport(0, 0, w, h);  // restore
        glEnable(GL_DEPTH_TEST);
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
             // bind texture to shadow map
            //  glBindFramebuffer(_depthMapFBO, lit->shadowMap.get().getGLTextureID());
            //  lit->shadowMap.set()

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
    const GLsizei SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    std::uint32_t _shaderIndex{Shader::invalidID()};
    std::uint32_t _materialIndex{Material::invalidID()};
    unsigned int _depthMapFBO { 0 };
    unsigned int _depthMap { 0 };
    glm::mat4 _lightSpaceMatrix;
    MaterialHandle _depthMapMaterial;
};

};  // namespace okay

#endif  // __SCENE_PASS_H__
