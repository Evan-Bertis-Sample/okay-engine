#include "okay_renderer.hpp"
#include "okay/core/util/result.hpp"
#include "okay_primitive.hpp"
#include "okay_render_pipeline.hpp"
#include "okay_render_world.hpp"

using namespace okay;

void OkayRenderer::initialize() {
    _surface->initialize();

    Failable meshBufferSetup =
        runAll(DEFER(_meshBuffer.initVertexAttributes()), DEFER(_meshBuffer.bindMeshData()));

    if (!meshBufferSetup) {
        Engine.logger.error("Mesh Buffer Setup Error: {}", meshBufferSetup.error());
        while (true) {
        }
    }

    _renderTargetPool.initializeBuiltins();
    _pipeline.initialize();
    _pipeline.resize(_settings.surfaceConfig.width, _settings.surfaceConfig.height);
}

void OkayRenderer::postInitialize() {
}

void OkayRenderer::tick() {
    _surface->pollEvents();

    if (_meshBufferDirty) {
        Failable meshBufferSetup = _meshBuffer.bindMeshData();
        if (!meshBufferSetup) {
            Engine.logger.error("Mesh Buffer Setup Error: {}", meshBufferSetup.error());
            while (true) {
            }
        }
        _meshBufferDirty = false;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    OkayRenderContext context{_world, _renderTargetPool};
    _pipeline.render(context);
    _surface->swapBuffers();
}

void OkayRenderer::postTick() {
}

void OkayRenderer::shutdown() {
    std::cout << "Okay Renderer shutdown." << std::endl;
}
