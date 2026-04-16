#include <okay/core/renderer/imgui_impl.hpp>

#include <EGL/egl.h>
#include <imgui.h>

namespace okay {

struct IMGUIImpl::Context {
    // nothing
};

bool IMGUIImpl::imguiSupported() {
    return false;
}

IMGUIImpl::IMGUIImpl() : _context(std::make_unique<IMGUIImpl::Context>()) {
}

IMGUIImpl::~IMGUIImpl() {
}

void IMGUIImpl::init(void* window, bool enableCallbacks) {
}

void IMGUIImpl::newFrame() {
}

void IMGUIImpl::renderDrawData(ImDrawData* drawData) {
}

void IMGUIImpl::shutdown() {
}

};  // namespace okay