#include <okay/core/renderer/imgui_impl.hpp>

#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace okay {

static void* imguiAlloc(size_t size, void* userData) {
    return std::malloc(size);
}

static void imguiFree(void* ptr, void* userData) {
    std::free(ptr);
}

struct IMGUIImpl::Context {
   public:
    ImGuiContext* guiContext{nullptr};
    ImGuiMemAllocFunc allocatorFn{imguiAlloc};
    ImGuiMemFreeFunc freeFn{imguiFree};
    void* userData{nullptr};
};

IMGUIImpl::IMGUIImpl() : _context(std::make_unique<IMGUIImpl::Context>()) {}

IMGUIImpl::~IMGUIImpl() {}

bool IMGUIImpl::imguiSupported() {
    return true;
}

void IMGUIImpl::init(void* window, ImGuiConfigFlags flags, bool enableCallbacks) {
    _context->guiContext = ImGui::CreateContext();

    ImGui::SetCurrentContext(_context->guiContext);
    ImGui::SetAllocatorFunctions(_context->allocatorFn, _context->freeFn, _context->userData);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = flags;

    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, enableCallbacks);
    ImGui_ImplOpenGL3_Init();
}

void IMGUIImpl::newFrame() {
    ImGui::SetCurrentContext(_context->guiContext);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void IMGUIImpl::renderDrawData(ImDrawData* drawData) {
    ImGui::SetCurrentContext(_context->guiContext);
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
}

void IMGUIImpl::shutdown() {
    ImGui::SetCurrentContext(_context->guiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
}

ImGuiContext* IMGUIImpl::getImguiContext() const {
    return _context->guiContext;
}

}  // namespace okay
