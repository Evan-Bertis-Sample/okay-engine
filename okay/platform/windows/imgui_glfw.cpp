#include <okay/core/renderer/imgui_impl.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace okay {

struct IMGUIImpl::Context {};

IMGUIImpl::IMGUIImpl() : _context(std::make_unique<IMGUIImpl::Context>()) {
}

IMGUIImpl::~IMGUIImpl() {
}

void IMGUIImpl::init(void* window, bool enableCallbacks) {
    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, enableCallbacks);
    ImGui_ImplOpenGL3_Init();
}

void IMGUIImpl::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void IMGUIImpl::renderDrawData(ImDrawData* drawData) {
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
}

void IMGUIImpl::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

}  // namespace okay