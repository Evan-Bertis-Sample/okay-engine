// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// device.hpp - Okay Engine

#pragma once

#include <okay/platform/surface.hpp>
#include <okay/rhi/texture.hpp>
#include <okay/util/logger.hpp>
#include <okay/util/object_pool.hpp>
#include <okay/util/result.hpp>

#include <vulkan/vulkan.hpp>

namespace okay::rhi {

struct RenderDeviceConfig {
    const char* applicationName{"Okay Application"};
    SurfaceConfig surfaceConfig;
};

using TextureHandle = ObjectPoolHandle;

class RenderDevice {
   public:
    RenderDeviceConfig config;
    Logger logger;

    RenderDevice(const RenderDeviceConfig& config)
        : config(config), _surface(config.surfaceConfig) {}

    // lifetime
    Failable initialize();
    void shutdown();

    TextureHandle createTexture(const TextureDescription& desc) {
        return TextureHandle::invalidHandle();
    }  // TODO

   private:
    vk::Instance _vkInstance;
    vk::PhysicalDevice _vkPhysicalDevice;
    vk::Device _vkDevice;
    vk::Queue _vkGraphicsQueue;
    vk::SwapchainKHR _swapchain;
    Surface _surface;

    bool _init{false};
    ObjectPool<Texture> _texturePool;
};

}  // namespace okay::rhi
