#include "renderer.hpp"

#include "gl.hpp"
#include "okay/core/util/result.hpp"
#include "render_pipeline.hpp"

#include <okay/core/asset/asset_util.hpp>
#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/gpu.hpp>
#include <okay/core/renderer/texture.hpp>

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

    AssetManager* am = Engine.systems.getSystemChecked<AssetManager>();
    TextureLoadSettings tLoad(TextureDataStore::mainStore());
    Texture white = okay::load::engineTexture("textures/white.jpg");

    TextureParameters params{
        .minFilter = GL_LINEAR,
        .magFilter = GL_LINEAR,
        .wrapS = GL_REPEAT,
        .wrapT = GL_REPEAT,
    };
    GLuint whiteID = 0;
    Failable uploadRes = GPUState::instance().textures.ensureUploaded2D(white, params, whiteID);
    if (uploadRes.isError()) {
        Engine.logger.error("Unable to upload white texture to GPU! Error: {}", uploadRes.error());
    } else {
        if (whiteID != 1) {
            Engine.logger.warn(
                "White texture is not uploaded at 1, "
                "it is located at {}. "
                "For texture defaults to work properly,"
                "ensure no texture is uploaded before "
                "render initialization.",
                whiteID);
        }
    }

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

void Renderer::postInitialize() {}

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
