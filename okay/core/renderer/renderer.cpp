#include "gl.hpp"
#include "renderer.hpp"
#include "okay/core/util/result.hpp"
#include "primitive.hpp"
#include "render_pipeline.hpp"
#include "render_world.hpp"

using namespace okay;

void OkayRenderer::initialize() {
    _surface->initialize();

    Engine.logger.info("GL_VERSION: {}", (const char*)glGetString(GL_VERSION));
    Engine.logger.info("GLSL_VERSION: {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    Failable meshBufferSetup =
        runAll(DEFER(_meshBuffer.bindMeshData()), DEFER(_meshBuffer.initVertexAttributes()));

    if (!meshBufferSetup) {
        Engine.logger.error("Mesh Buffer Setup Error: {}", meshBufferSetup.error());
        Engine.shutdown();
        return;
    }
    Engine.logger.debug("Mesh Buffer Setup Success");

    Engine.logger.debug("Initializing render target pool");
    _renderTargetPool.initializeBuiltins();

    Engine.logger.debug("Initializing pipeline");
    _pipeline.initialize();

    Engine.logger.debug("Resizing pipeline");
    _pipeline.resize(_surfaceConfig.width, _surfaceConfig.height);

    Engine.logger.debug("Renderer initialized");
}

void OkayRenderer::postInitialize() {
}

void OkayRenderer::tick() {
    _surface->pollEvents();
    OkayRenderContext context{*this, _world, _renderTargetPool};
    _pipeline.render(context);
}

void OkayRenderer::postTick() {
    _surface->swapBuffers();
}

void OkayRenderer::shutdown() {
}