#include <okay/core/renderer/imgui_impl.hpp>

#include <EGL/egl.h>
#include <imgui.h>
#include <imgui_impl_egl.h>
#include <imgui_impl_opengl3.h>

namespace okay {

struct IMGUIImpl::Context {
    // nothing
};

IMGUIImpl::IMGUIImpl() : _context(std::make_unique<IMGUIImpl::Context>()) {
}

IMGUIImpl::~IMGUIImpl() {
}

void IMGUIImpl::init(void* window, bool enableCallbacks) {
    ImGui_ImplEGL_Init(window);
    ImGui_ImplOpenGL3_Init();
}

void IMGUIImpl::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplEGL_NewFrame();
}

void IMGUIImpl::renderDrawData(ImDrawData* drawData) {
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
}

void IMGUIImpl::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplEGL_Shutdown();
}

};  // namespace okay