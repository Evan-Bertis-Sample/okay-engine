#include "okay_renderer.hpp"

using namespace okay;

void OkayRenderer::initialize() {
    _surface->initialize();

    OkayAssetManager* assetManager = Engine.systems.getSystemChecked<OkayAssetManager>();

    // load -> compile -> set -> findUniformLocations -> setUniforms
    auto shaderSetup = catchResult(
        assetManager->loadEngineAssetSync<OkayShader<TestMaterialUniforms>>("shaders/test")
            .then([](auto& shaderAsset) {
                return Result<OkayShader<TestMaterialUniforms>>::ok(shaderAsset.asset);
            })
            .then([](OkayShader<TestMaterialUniforms>& shader) {
                Engine.logger.info("Compiling shader...");
                shader.compile();
                return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
            })
            .then([](OkayShader<TestMaterialUniforms>& shader) {
                Engine.logger.info("Setting shader...");
                shader.set();
                return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
            })
            .then([](OkayShader<TestMaterialUniforms>& shader) {
                Engine.logger.info("Finding uniform locations...");
                shader.findUniformLocations();
                return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
            })
            .then([](OkayShader<TestMaterialUniforms>& shader) {
                Engine.logger.info("Setting uniforms...");
                shader.uniforms.get<FixedString("u_color")>().set(glm::vec3(1.0f, 0.0f, 1.0f));
                return Result<OkayShader<TestMaterialUniforms>>::ok(shader);
            }),
        [](const std::string& failMessage) { Engine.logger.error("Cannot setup shader!"); });

    if (!shaderSetup) {
        Engine.logger.error("We have failed to setup the shader, stalling the program...");
        while (true) {}
    }

    Engine.logger.info("Shader setup done! Test float {}", 0.0f, 0.0f);

    Failable modelBufferSetup = stepThrough(
        [](const std::string& failMessage) { Engine.logger.error("Cannot setup model buffer!"); },
        _modelBuffer.initVertexAttributes(), _modelBuffer.bindMeshData());

    if (!modelBufferSetup) {
        Engine.logger.error("We have failed to setup the model buffer, stalling the program...");
        while (true) {}
    }

    _model = _modelBuffer.addMesh(primitives::box().sizeSet({0.01f, 0.01f, 0.01f}).build());
    _modelBuffer.initVertexAttributes();
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

    // _modelBuffer.drawMesh(_model);
    _surface->swapBuffers();
}

void OkayRenderer::postTick() {
    // Cleanup or prepare for the next frame
}

void OkayRenderer::shutdown() {
    std::cout << "Okay Renderer shutdown." << std::endl;
    // Cleanup rendering resources here
}
