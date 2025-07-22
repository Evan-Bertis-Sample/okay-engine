#ifndef __OKAY_H__
#define __OKAY_H__

#include <iostream>
#include <string>

#include <GLFW/glfw3.h>

// global window
GLFWwindow* g_window = nullptr;

class OkayEngine {
   public:
    static void initialize() {
        std::cout << "Okay Engine initialized." << std::endl;

        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW." << std::endl;
            return;
        }

        // hook it up with OpenGL
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // make a window
        g_window = glfwCreateWindow(800, 600, "Okay Engine", nullptr, nullptr);
        if (!g_window) {
            std::cerr << "Failed to create GLFW window." << std::endl;
            glfwTerminate();
            return;
        }
    }
    static void run() {
        std::cout << "Okay Engine is running." << std::endl;

        while (!glfwWindowShouldClose(g_window)) {
            // Poll for and process events
            glfwPollEvents();

            // Render here (placeholder)
            // glClear(GL_COLOR_BUFFER_BIT);

            // Swap front and back buffers
            glfwSwapBuffers(g_window);
        }
    }
    static void shutdown() {
        std::cout << "Okay Engine shutdown." << std::endl;
        glfwDestroyWindow(g_window);
        glfwTerminate();
    }
};

#endif  // __OKAY_H__