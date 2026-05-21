#ifndef __SCENE_PASS_H__
#define __SCENE_PASS_H__

#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "okay/core/engine/system.hpp"
#include "okay/core/renderer/material.hpp"
#include "okay/core/renderer/math_types.hpp"
#include "okay/core/renderer/render_world.hpp"
#include "okay/core/renderer/texture.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/materials/lit.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/materials/depth_map.hpp>
#include <okay/core/renderer/render_pipeline.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/renderer/uniform.hpp>

#include "GLFW/glfw3.h"
#include <cstdint>
#include <memory>

namespace okay {

class ScenePass : public IRenderPass {
   public:
    ScenePass() {}

    virtual const std::string_view name() const override {
        return "ScenePass";
    }

    virtual void initialize() override {
        initializeDepthMap();
        initializeDepthTexture();
        initializeSkyboxMesh();

        _shadowDist = 40;
        _shadowWidth = 2048;
        _shadowHeight = 2048;
        _margin = 0.5;
        _screenSpaceProjectionMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 10.0f);
    }

    virtual void resize(int newWidth, int newHeight) override {}

    virtual void render(const RendererContext& context) override {
        glfwGetFramebufferSize((GLFWwindow*)context.renderer.getSurfaceWindow(), &_fbwidth, &_fbheight);
        renderSettings();

        float aspect = float(_fbwidth) / float(_fbheight);
        auto view = context.world.camera().viewMatrix();
        auto projection = context.world.camera().projectionMatrix(aspect);
        auto camPos = context.world.camera().position();
        auto camDir = context.world.camera().direction();

        glCullFace(GL_FRONT);
        renderDepthMap(context, aspect);
        glCullFace(GL_BACK);

        renderSkybox(context, aspect, projection, view);

        renderItems(context, aspect, projection, view, camPos, camDir);

        displayDepthMap(context);
    }

    void initializeDepthMap() {
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
                    _shadowWidth, _shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void initializeDepthTexture() {
        okay::OkayTextureMeta depthMeta {
            .width = (uint32_t) _shadowWidth,
            .height = (uint32_t) _shadowHeight,
            .channels = 1,
            .mipLevels = 1,
            .format = okay::OkayTextureMeta::Format::DEPTH_COMPONENT,
        };
        _depthMapTexture = okay::RenderTexture(_depthMap, depthMeta);
    }

    void initializeSkyboxMesh() {
        _skyboxMesh = Engine.systems.getSystemChecked<Renderer>()->meshBuffer().addMesh(
            okay::primitives::box()
                .sizeSet(glm::vec3{2.0f, 2.0f, 2.0f})  // to span -1.0 to 1.0
                .build());
    }

    void renderSettings() {
        // GL_CHECK(glClearColor(0.113f, 0.008, 0.208, 1.0f));
        GL_CHECK(glClearColor(0.580, 0.580, 0.580, 1.0));
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GL_CHECK(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

        GL_CHECK(glViewport(0, 0, _fbwidth, _fbheight));
        GL_CHECK(glDepthFunc(GL_LESS)); 
        
        GL_CHECK(glEnable(GL_DEPTH_TEST));
        GL_CHECK(glEnable(GL_CULL_FACE));
        GL_CHECK(glCullFace(GL_BACK));
        GL_CHECK(glDisable(GL_BLEND));
        GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GL_CHECK(glDepthMask(GL_TRUE));

        // doesn't need MSAA, but should if the platform can support it
        glEnable(GL_MULTISAMPLE);
    }
    
    bool checkIfNeedsRefit(const RendererContext& context, const glm::vec3& lightDir) {
        if (dot(_cachedLightDir, lightDir) < 0.9999) return true;

        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            RenderItem& item = context.world.getRenderItem(handle);
            if (item.mesh.isEmpty()) continue;
            if (!item.material->properties()->flags().hasFlag(MaterialFlags::CAST_SHADOWS)) continue;

            for (const glm::vec3& corner : item.mesh.bounds.transform(item.worldMatrix).corners()) {
                glm::vec3 lsc = _cachedLightView * glm::vec4(corner, 1.0);
                if (lsc.x < _left || lsc.x > _right || 
                    lsc.y < _bottom || lsc.y > _top || 
                    -lsc.z > _far) {
                    return true;
                }
            }
        }
        return false;
    }

    void refit(const RendererContext& context, const glm::vec3& lightDir, const glm::vec3& lightEye, const glm::mat4& lightView, const std::array<glm::vec3, 8>& worldSpaceCoords) {
        _cachedLightDir = lightDir;
        _cachedLightEye = lightEye;
        _cachedLightView = lightView;
        glm::vec3 minLightCoords(FLT_MAX), maxLightCoords(-FLT_MAX);
        bool hasCasters = false;
        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            RenderItem& item = context.world.getRenderItem(handle);
            if (item.mesh.isEmpty()) continue;
            if (!item.material->properties()->flags().hasFlag(MaterialFlags::CAST_SHADOWS)) continue;

            for (const glm::vec3& corner : item.mesh.bounds.transform(item.worldMatrix).corners()) {
                glm::vec3 lsc = glm::vec3(_cachedLightView * glm::vec4(corner, 1.0f));
                minLightCoords = glm::min(minLightCoords, lsc);
                maxLightCoords = glm::max(maxLightCoords, lsc);
                hasCasters = true;
            }
        }

        if (!hasCasters) {
            minLightCoords = glm::vec3(FLT_MAX);
            maxLightCoords = glm::vec3(-FLT_MAX);
            for (const glm::vec3& c : worldSpaceCoords) {
                glm::vec3 lsc = glm::vec3(_cachedLightView * glm::vec4(c, 1.0f));
                minLightCoords = glm::min(minLightCoords, lsc);
                maxLightCoords = glm::max(maxLightCoords, lsc);
            }
        }

        glm::vec3 size = maxLightCoords - minLightCoords;
        glm::vec3 padding = size * _margin;
        _left = minLightCoords.x - padding.x; _right = maxLightCoords.x + padding.x;
        _bottom = minLightCoords.y - padding.y; _top = maxLightCoords.y + padding.y;
        _far = -minLightCoords.z + padding.z;
        _validMatrix = true;
    }

    void renderDepthMap(const RendererContext& context, float aspect) {
        for (const Light& l : context.world.lights()) {
            switch (l.type()) {
                case Light::Type::POINT:
                case Light::Type::SPOT:
                    continue;
                case Light::Type::DIRECTIONAL: {                    
                    glm::vec3 frustumCenter(0.0f);
                    auto worldSpaceCoords = context.world.camera().frustumCornersWorld(aspect, _shadowDist);
                    for (const auto& c : worldSpaceCoords) frustumCenter += c;
                    frustumCenter /= 8.0f;

                    glm::vec3 lightDir = glm::normalize(glm::vec3(l.direction));
                    glm::vec3 lightEye = frustumCenter - lightDir * 25.0f;
                    glm::mat4 lightView = glm::lookAt(lightEye, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
                    
                    bool needsRefit = !_validMatrix;

                    if (_validMatrix) {
                        needsRefit = checkIfNeedsRefit(context, lightDir);
                    }
                    
                    if (needsRefit) {
                        refit(context, lightDir, lightEye, lightView, worldSpaceCoords);
                    }

                    _orthoCamera.transform.position = _cachedLightEye;
                    _orthoCamera.transform.rotation = glm::quat_cast(glm::transpose(glm::mat3(_cachedLightView)));
                    _orthoCamera.lens = okay::Camera::OrthographicLens { _left, _right, _bottom, _top, _near, _far };

                    Engine.logger.debug("Left: {}, Right: {}, Bottom: {}, Top: {}, Near: {}, Far: {}", _left, _right, _bottom, _top, _near, _far);
                    
                    glm::mat4 lightProjection = _orthoCamera.projectionMatrix(aspect);
                    _lightSpaceMatrix = lightProjection * _cachedLightView;

                    // --- Depth pass ---
                    glViewport(0, 0, _shadowWidth, _shadowHeight);
                    glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
                    glClear(GL_DEPTH_BUFFER_BIT);

                    if (auto f = _depthMapMaterial->setShader(); f.isError()) {
                        Engine.logger.error("Failed to set depth shader: {}", f.error());
                    }

                    auto* depthProps = dynamic_cast<DepthMapMaterial*>(_depthMapMaterial->properties().get());
                    if (depthProps) depthProps->lightSpaceMatrix.set(_lightSpaceMatrix);

                    // render depth map
                    drawDepthMesh(context, aspect, depthProps);

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glViewport(0, 0, _fbwidth, _fbheight);
                    break;
                }
            }
        }
    }

    void drawDepthMesh(const RendererContext& context, float aspect, DepthMapMaterial* depthProps) {
        for (const RenderItemHandle& handle : context.world.getRenderItems()) {
            RenderItem& item = context.world.getRenderItem(handle);
            if (item.mesh.isEmpty()) continue;

            if (!item.material->properties()->flags().hasFlag(MaterialFlags::CAST_SHADOWS)) continue;

            if (depthProps) depthProps->modelMatrix.set(item.worldMatrix);

            if(!_orthoCamera.isInFrustum(item.mesh.bounds.transform(item.worldMatrix), aspect)) {
                continue;
            }

            Failable df = _depthMapMaterial->passUniforms();
            if (df.isError()) Engine.logger.error("depth passUniforms: {}", df.error());
            
            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
    }

    void renderSkybox(const RendererContext& context, float aspect, const glm::mat4& projection, const glm::mat4& view) {
        MaterialHandle skyboxMaterial = context.renderer.skyboxMaterial();
        if (skyboxMaterial.isValid()) {
            handleMaterialSwitch(context,
                skyboxMaterial,
                projection,
                view,
                context.world.camera().position(),
                context.world.camera().direction());

            if (auto* props = dynamic_cast<SceneMaterialProperties*>(skyboxMaterial->properties().get())) {
                props->modelMatrix = glm::identity<glm::mat4>();
            }

            Failable f = skyboxMaterial->passUniforms();

            if (f.isError()) {
                Engine.logger.error("Failed to pass skybox uniforms! Error: {}", f.error());
            } else {
                context.renderer.meshBuffer().drawMesh(_skyboxMesh);
            }
        }
    }

    void renderItems(const RendererContext& context, float aspect, const glm::mat4& projection, const glm::mat4& view, const glm::vec3& camPos, const glm::vec3& camDir) {
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
                handleMaterialSwitch(context, item.material, projection, view, camPos, camDir);
                _materialIndex = item.material->id();
            }

            // Set per-object uniforms
            setPerObjectUniforms(item);
            
            if (auto* lit = dynamic_cast<okay::LitMaterial*>(item.material->properties().get())) {
                lit->projectionMatrix.set(projection);
                lit->viewMatrix.set(view);
                lit->cameraPosition.set(camPos);
                lit->cameraDirection.set(camDir);
                lit->lightSpaceMatrix.set(_lightSpaceMatrix);
                lit->shadowMap.set(_depthMapTexture);
            }
            // Now push uniforms for this draw (or only the ones you changed)
            Failable f = item.material->passUniforms();
            if (f.isError())
                Engine.logger.error("Failed to pass uniforms : {}", f.error());

            applyMaterialFlags(item.material);
            context.renderer.meshBuffer().drawMesh(item.mesh);
        }
    }

    void handleMaterialSwitch(const RendererContext& context,
        MaterialHandle material,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::vec3& camPos,
        const glm::vec3& camDir) {
        applyMaterialFlags(material);

        if (_shaderIndex != material->shaderID()) {
            if (auto f = material->setShader(); f.isError()) {
                Engine.logger.error("Failed to set shader : {}", f.error());
                return;
            }
            _shaderIndex = material->shaderID();
        }

        auto& properties = material->properties();
        MaterialFlagCollection flags = properties->flags();

        if (auto* sceneProps = dynamic_cast<okay::SceneMaterialProperties*>(properties.get())) {
            if (flags.hasFlag(MaterialFlags::SCREEN_SPACE)) {
                sceneProps->projectionMatrix.set(_screenSpaceProjectionMat);
                sceneProps->viewMatrix.set(glm::identity<glm::mat4>());
            } else {
                sceneProps->projectionMatrix.set(projection);
                sceneProps->viewMatrix.set(view);
            }
            sceneProps->cameraPosition.set(camPos);
            sceneProps->cameraDirection.set(camDir);
            sceneProps->timeMs.set(static_cast<float>(Engine.time->timeSinceStartMs()));
        }

        if (auto* lit = dynamic_cast<okay::LitMaterial*>(properties.get())) {
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

    void applyMaterialFlags(MaterialHandle mat) {
        auto& uniforms = mat->properties();
        MaterialFlagCollection flags = uniforms->flags();

        if (flags.hasFlag(MaterialFlags::DOUBLE_SIDED)) {
            GL_CHECK(glDisable(GL_CULL_FACE));
        } else {
            GL_CHECK(glEnable(GL_CULL_FACE));
            GL_CHECK(glCullFace(GL_BACK));
        }

        if (flags.hasFlag(MaterialFlags::TRANSPARENT)) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        } else {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }

        if (flags.hasFlag(MaterialFlags::SCREEN_SPACE)) {
            glDisable(GL_DEPTH_TEST);
        } else {
            glEnable(GL_DEPTH_TEST);
        }
    }

    // display the depth map onto the screen
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
        
        int w = _fbwidth / 3;
        int h = _fbheight / 3;
        glViewport(0, 0, w, h);  // bottom-left, 1/3 of screen
        
        glDisable(GL_DEPTH_TEST);
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _depthMap);
        glUniform1i(glGetUniformLocation(program, "depthMap"), 0);
        
        glBindVertexArray(depthmapVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glViewport(0, 0, _fbwidth, _fbheight);  // restore
        glEnable(GL_DEPTH_TEST);
    }

   private:
    int _fbwidth{ 0 }, _fbheight{ 0 };
    float _shadowDist = 40.0;
    GLsizei _shadowWidth = 2048, _shadowHeight = 2048;
    float _margin = 0.5;

    std::uint32_t _shaderIndex{Shader::invalidID()};
    std::uint32_t _materialIndex{Material::invalidID()};
    unsigned int _depthMapFBO { 0 };
    unsigned int _depthMap { 0 };

    okay::Camera _orthoCamera{};
    bool _validMatrix { false };
    glm::mat4 _cachedLightView{};
    glm::vec3 _cachedLightDir{};
    glm::vec3 _cachedLightEye{};
    float _left{}, _right{};
    float _bottom{}, _top{}; 
    float _near{ 0.1f }, _far{};

    glm::mat4 _lightSpaceMatrix;
    MaterialHandle _depthMapMaterial;
    RenderTexture _depthMapTexture;
    glm::mat4 _screenSpaceProjectionMat{};
    Mesh _skyboxMesh;
};

};  // namespace okay

#endif  // __SCENE_PASS_H__
