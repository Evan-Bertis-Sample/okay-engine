#ifndef __GLFW_SURFACE_H__
#define __GLFW_SURFACE_H__

#include <okay/core/graphics/okay_surface.hpp>
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace okay::glfw {
    class GLFWSurface : public SurfaceDriver {
       public:
        GLFWSurface(const SurfaceConfig& config) : SurfaceDriver(config) {}

        void initialize() override {
            if (!glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW.");
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            _window = glfwCreateWindow(_config.width, _config.height, _config.title, nullptr, nullptr);
            if (!_window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window.");
            }
            glfwMakeContextCurrent(_window);
        }

        bool shouldClose() const override {
            return glfwWindowShouldClose(_window);
        }

        void pollEvents() override {
            glfwPollEvents();
        }

        void swapBuffers() override {
            glfwSwapBuffers(_window);
        }

        void destroy() override {
            glfwDestroyWindow(_window);
            glfwTerminate();
        }

       private:
        GLFWwindow* _window = nullptr;
};


#endif // __GLFW_SURFACE_H__