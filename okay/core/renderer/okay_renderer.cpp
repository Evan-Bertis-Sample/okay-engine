
#include "okay_gl.hpp"
#include "okay_renderer.hpp"
#include "okay/core/util/result.hpp"
#include "okay_primitive.hpp"
#include "okay_render_pipeline.hpp"
#include "okay_render_world.hpp"

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

    GL_CHECK(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));

    OkayRenderContext context{*this, _world, _renderTargetPool};
    _pipeline.render(context);
    _surface->swapBuffers();
}

void OkayRenderer::postTick() {
}

void OkayRenderer::shutdown() {
}