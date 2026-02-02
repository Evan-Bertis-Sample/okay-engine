#include "okay_renderer.hpp"
#include "okay/core/util/result.hpp"
#include "okay_primitive.hpp"
#include "okay_render_world.hpp"

using namespace okay;

void OkayRenderer::initialize() {
    _surface->initialize();

    OkayAssetManager* assetManager = Engine.systems.getSystemChecked<OkayAssetManager>();

    // load -> compile -> set -> findUniformLocations -> setUniforms
    auto loadShaderResult =
        assetManager->loadEngineAssetSync<OkayShader<TestMaterialUniforms>>("shaders/test");

    if (!loadShaderResult) {
        Engine.logger.error("Failed to load shader: {}", loadShaderResult.error());
    }

    shader = loadShaderResult.value().asset;

    Failable shaderSetup =
        runAll(DEFER(shader.compile()), DEFER(shader.set()), DEFER(shader.findUniformLocations()));

    if (!shaderSetup) {
        Engine.logger.error("Failed to setup shader: {}", shaderSetup.error());
        while (true) {
        }
    }

    // set a uniform
    shader.uniforms.get<FixedString("u_color")>().set(glm::vec3(1.0f, 0.0f, 1.0f));
    shader.passDirtyUniforms();

    // set mesh data
    // _model = _modelBuffer.addMesh(primitives::box().sizeSet({0.1f, 0.1f, 0.1f}).build());
    _model = _modelBuffer.addMesh(
        primitives::uvSphere()
            .radiusSet(0.1f)
            .build()
    );

    Failable meshBufferSetup =
        runAll(DEFER(_modelBuffer.initVertexAttributes()), DEFER(_modelBuffer.bindMeshData()));

    if (!meshBufferSetup) {
        Engine.logger.error("We have failed to setup the model buffer, stalling the program...");
        Engine.logger.error("Mesh Buffer Setup Error: {}", meshBufferSetup.error());
        while (true) {
        }
    }

    OkayRenderWorld world;
    OkayRenderEntity e = world.addRenderEntity(OkayTransform(), nullptr, _model);

    e.prop().transform = OkayTransform(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );
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
        Engine.logger.error("Failed to set shader: {}", f.error());
        while (true) {
        }
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
