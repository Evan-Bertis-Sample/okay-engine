#include "okay_imgui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "okay/core/okay.hpp"
#include "okay/core/renderer/okay_renderer.hpp"

using namespace okay;

void OkayIMGUI::initialize() {
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

void OkayIMGUI::postInitialize() {}

void OkayIMGUI::tick() {
    // Rendering
    // (Your code clears your framebuffer, renders your other stuff etc.)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // (Your code calls glfwSwapBuffers() etc.)
}

void OkayIMGUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
