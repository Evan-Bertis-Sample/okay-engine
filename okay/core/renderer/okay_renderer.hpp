#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <iostream>
#include <okay/core/okay.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/renderer/okay_primitive.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/singleton.hpp>

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig SurfaceConfig;
};

class OkayRenderer : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static std::unique_ptr<OkayRenderer> create(const OkayRendererSettings& settings) {
        return std::make_unique<OkayRenderer>(settings);
    }

    OkayRenderer(const OkayRendererSettings& settings)
        : _settings(settings), _surface(std::make_unique<Surface>(settings.SurfaceConfig)) {}

    using TestMaterialUniforms =
        OkayMaterialUniformCollection<OkayMaterialUniform<glm::vec3, FixedString("u_color")>>;

    OkayShader<TestMaterialUniforms> shader;

    void initialize() override {
        _surface->initialize();

        OkayAssetManager* assetManager = Engine.systems.getSystemChecked<OkayAssetManager>();


        auto shaderSetup = catchResult(
            assetManager->loadEngineAssetSync<OkayShader<TestMaterialUniforms>>("shaders/test")
                .then([](auto& shaderAsset) {
                    return Result<OkayShader<TestMaterialUniforms>>::ok(shaderAsset.asset);
                })
                .then([](OkayShader<TestMaterialUniforms>& shader) {
                    shader.set();
                    return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
                })
                .then([](OkayShader<TestMaterialUniforms>& shader) {
                    shader.findUniformLocations();
                    return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
                })
                .then([](OkayShader<TestMaterialUniforms>& shader) {
                    shader.uniforms.get<FixedString("u_color")>().set(glm::vec3(1.0f, 1.0f, 1.0f));
                    return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
                }),
            [](const std::string& failMessage) { Engine.logger.error("Cannot setup shader!"); });

        if (!shaderSetup) {
            return;  // we have failed
        }

        Engine.logger.info("Shader setup done! Test float {}", 0.0f, 0.0f);

        _model = _modelBuffer.addMesh(okay::primitives::box().sizeSet({0.1f, 0.1f, 0.1f}).build());

        _modelBuffer.bindMeshData();
    }

    void postInitialize() override {
        std::cout << "Okay Renderer post-initialization." << std::endl;
        // Additional setup after initialization
    }

    void tick() override {
        _surface->pollEvents();
        // Render the current frame

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        Failable f = shader.set();
        if (f.isError()) {
            std::cerr << "Failed to set shader: " << f.error() << std::endl;
            while (true) {
            }
            return;
        }

        _modelBuffer.drawMesh(_model);
        _surface->swapBuffers();
    }

    void postTick() override {
        // Cleanup or prepare for the next frame
    }

    void shutdown() override {
        std::cout << "Okay Renderer shutdown." << std::endl;
        // Cleanup rendering resources here
    }

   private:
    OkayRendererSettings _settings;
    std::unique_ptr<Surface> _surface;

    OkayMeshBuffer _modelBuffer;
    OkayMesh _model;
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__