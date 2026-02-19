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
    // ImGui_ImplOpenGL3_NewFrame();
    // ImGui_ImplGlfw_NewFrame();
    // ImGui::NewFrame();
    
    // ImGui::Begin("Properties");
    // if (ImGui::CollapsingHeader("test", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     if (ImGui::Button("testButton")) {
    //         okay::Engine.logger.debug("aaa");
    //     }
    // }
    // float placeholder {};
    // bool b1 {};
    // bool b2 {};
    // bool b3 {};
    // if (ImGui::CollapsingHeader("stuff")) {
    //     ImGui::Text("Name");
    //     ImGui::ColorPicker3("Color", &placeholder, ImGuiColorEditFlags_PickerHueWheel);
    //     ImGui::SliderFloat("Metallic", &placeholder, 0.0f, 1.0f);
    //     ImGui::Separator();
    //     if (ImGui::BeginTable("split", 3))
    //     {
    //         ImGui::TableNextColumn(); ImGui::Checkbox("No titlebar", &b1);
    //         ImGui::TableNextColumn(); ImGui::Checkbox("No scrollbar", &b2);
    //         ImGui::TableNextColumn(); ImGui::Checkbox("No menu", &b3);
    //         ImGui::EndTable();
    //     }
    // }
    // ImGui::End();

    // ImGui::Render();
    // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // (Your code calls glfwSwapBuffers() etc.)
}

void OkayIMGUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
