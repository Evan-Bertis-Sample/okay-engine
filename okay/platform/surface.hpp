// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// surface.hpp - Okay Engine

#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

namespace okay {

struct SurfaceConfig {
    int width{800};
    int height{600};
    const char* title{"Okay Surface"};
    bool resizable{true};
    bool vsync{false};
};

class Surface {
   public:
    explicit Surface(const SurfaceConfig& cfg);
    ~Surface();  // non-virtual dtor is fine (no inheritance)
    Surface(Surface&&) noexcept;
    Surface& operator=(Surface&&) noexcept;

    void initialize();
    std::vector<const char*> requiredVulkanExtensions();
    vk::SurfaceKHR createVulkanSurface(vk::Instance instance);

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();
    void destroy();
    void* getWindow();

   private:
    struct SurfaceImpl;                  // defined in the backend .cpp
    std::unique_ptr<SurfaceImpl> _impl;  // opaque to callers
};

}  // namespace okay
