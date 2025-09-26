#include <GLFW/glfw3.h>

#include <okay/core/renderer/okay_surface.hpp>
#include <stdexcept>
#include <string>

namespace okay {

struct Surface::SurfaceImpl {
    SurfaceConfig cfg;
    GLFWwindow* window = nullptr;
    explicit SurfaceImpl(const SurfaceConfig& c) : cfg(c) {}
};

Surface::Surface(const SurfaceConfig& cfg) : _impl(std::make_unique<SurfaceImpl>(cfg)) {}
Surface::~Surface() = default;
Surface::Surface(Surface&&) noexcept = default;
Surface& Surface::operator=(Surface&&) noexcept = default;

void Surface::initialize() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, _impl->cfg.resizable ? GLFW_TRUE : GLFW_FALSE);

    _impl->window =
        glfwCreateWindow(_impl->cfg.width, _impl->cfg.height, _impl->cfg.title, nullptr, nullptr);
    if (!_impl->window) throw std::runtime_error("glfwCreateWindow failed");

    glfwMakeContextCurrent(_impl->window);
    glfwSwapInterval(_impl->cfg.vsync ? 1 : 0);
}

bool Surface::shouldClose() const {
    return _impl->window ? glfwWindowShouldClose(_impl->window) : true;
}

void Surface::pollEvents() {
    glfwPollEvents();
}

void Surface::swapBuffers() {
    glfwSwapBuffers(_impl->window);
}

void Surface::destroy() {
    if (_impl->window) {
        glfwDestroyWindow(_impl->window);
        _impl->window = nullptr;
    }
    glfwTerminate();
}

}  // namespace okay
