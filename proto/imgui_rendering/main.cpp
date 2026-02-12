#include <cstddef>
#include <functional>
#include <memory>
#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>

#include <utility>
#include <okay/core/tween/okay_tween.hpp>
#include <okay/core/tween/okay_tween_easing.hpp>
#include <okay/core/tween/okay_tween_engine.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();


int main() {
    okay::SurfaceConfig surfaceConfig;
    okay::Surface surface(surfaceConfig);
    
    okay::OkayRendererSettings rendererSettings{surfaceConfig};
    auto renderer = okay::OkayRenderer::create(rendererSettings);

    okay::OkayLevelManagerSettings levelManagerSettings;
    auto levelManager = okay::OkayLevelManager::create(levelManagerSettings);
 
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
    
    // Setup Platform/Renderer backends
    // ImGui_ImplGlfw_InitForOpenGL(okay::Engine.systems.getSystem<okay::OkayRenderer>().value()->getSurfaceWindow(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.

    // (Your code calls glfwPollEvents())
    // ...
    // Start the Dear ImGui frame

    okay::OkayGame::create()
        .addSystems(std::move(renderer), std::move(levelManager),
                    std::make_unique<okay::OkayAssetManager>(),
                    std::make_unique<okay::OkayTweenEngine>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}


static void __gameInitialize() {
    // Additional game initialization logic
    okay::Engine.logger.info("Game initialized.");

        // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    okay::OkayRenderer *renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    auto win = renderer->getSurfaceWindow();
    ImGui_ImplGlfw_InitForOpenGL(win, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
    
}

static void __gameUpdate() {
    // Game update logic
     // Rendering
    // (Your code clears your framebuffer, renders your other stuff etc.)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(); // Show demo window! :)


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // // (Your code calls glfwSwapBuffers() etc.)
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
