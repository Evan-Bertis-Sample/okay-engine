#include "renderer.hpp"

#include "gl.hpp"
#include "okay/core/util/result.hpp"
#include "render_pipeline.hpp"

#include <imgui.h>

using namespace okay;

void Renderer::initialize() {
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

    if (_imguiImpl->imguiSupported() && _imguiEnabled) {
        Engine.logger.info("Initializing ImGui");
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        auto win = getSurfaceWindow();
        if (win == nullptr) {
            Engine.logger.warn("No GLFW window available");
            return;
        }
        _imguiImpl->init(win, true);
        _imguiInitialized = true;
    }
}

void Renderer::postInitialize() {
}

void Renderer::preTick() {
    if (_imguiImpl->imguiSupported() && _imguiEnabled && _imguiInitialized) {
        _imguiImpl->newFrame();
        ImGui::NewFrame();
    }
}

void Renderer::tick() {
    _meshBuffer.bindMeshData();
    _surface->pollEvents();
    RendererContext context{*this, _world, _renderTargetPool};
    _pipeline.render(context);
}

void Renderer::postTick() {
    if (_imguiImpl->imguiSupported() && _imguiEnabled && _imguiInitialized) {
        ImGui::Render();
        _imguiImpl->renderDrawData(ImGui::GetDrawData());
    }

    _surface->swapBuffers();
}

void Renderer::shutdown() {
    if (_imguiImpl->imguiSupported() && _imguiInitialized) {
        _imguiImpl->shutdown();
        ImGui::DestroyContext();
    }
    _surface->destroy();
}
