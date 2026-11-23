// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// vk_rhi.hpp - Okay Engine

#pragma once

#include <okay/core/engine/engine.hpp>
#include <okay/rhi/vk/vk_api.hpp>

namespace okay {

struct VkRhiConfig {
    const char* applicationName;
};

class VkRhi {
   public:
    VkRhiConfig config;

    VkRhi(const VkRhiConfig& config) : config(config) {}

    void initialize() {
        vk::ApplicationInfo appInfo{.pApplicationName = config.applicationName,
            .applicationVersion = 1,
            .pEngineName = "okay_engine_vk",
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_2};

        vk::InstanceCreateInfo instanceCreateInfo{
            .pApplicationInfo = &appInfo,
        };

        _vkInstance = vk::createInstance(instanceCreateInfo);
        Engine.logger.info("Created vk instance!");
    }

   private:
    vk::Instance _vkInstance;
};

}  // namespace okay
