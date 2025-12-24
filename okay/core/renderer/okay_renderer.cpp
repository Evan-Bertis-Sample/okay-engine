#include "okay_renderer.hpp"

using namespace okay;

void OkayRenderer::initialize() {
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

    _model = _modelBuffer.addMesh(primitives::box().sizeSet({0.1f, 0.1f, 0.1f}).build());

    _modelBuffer.bindMeshData();
}

void OkayRenderer::postInitialize() {
    std::cout << "Okay Renderer post-initialization." << std::endl;
    // Additional setup after initialization
}

void OkayRenderer::tick() {
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

void OkayRenderer::postTick() {
    // Cleanup or prepare for the next frame
}

void OkayRenderer::shutdown() {
    std::cout << "Okay Renderer shutdown." << std::endl;
    // Cleanup rendering resources here
}
