#include <okay/core/renderer/gl.hpp>
#include <okay/platform/surface.hpp>

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

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
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, _impl->cfg.resizable ? GLFW_TRUE : GLFW_FALSE);

    _impl->window =
        glfwCreateWindow(_impl->cfg.width, _impl->cfg.height, _impl->cfg.title, nullptr, nullptr);

    if (!_impl->window) {
        throw std::runtime_error("glfwCreateWindow failed");
    }
}

vk::SurfaceKHR Surface::createVulkanSurface(vk::Instance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(instance, _impl->window, nullptr, &surface);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
    return vk::SurfaceKHR{surface};
}

std::vector<const char*> Surface::requiredVulkanExtensions() {
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    if (!extensions) {
        const char* description = nullptr;
        int error = glfwGetError(&description);

        throw std::runtime_error("Vulkan extensions unavailable (GLFW error " +
                                 std::to_string(error) +
                                 "): " + (description ? description : "Unknown"));
    }

    return {extensions, extensions + count};
}

void* Surface::getWindow() {
    return _impl->window;
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
