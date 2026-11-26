// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// rhi.hpp - Okay Engine

#pragma once

#include <okay/util/logger.hpp>
#include <okay/util/result.hpp>

#include <vulkan/vulkan.hpp>

namespace okay {

struct VkRhiConfig {
    const char* applicationName;
};

class VkRhi {
   public:
    VkRhiConfig config;
    Logger logger;

    VkRhi(const VkRhiConfig& config) : config(config) {}

    Failable initialize();

   private:
    vk::Instance _vkInstance;
    vk::Device _vkDevice;
};

}  // namespace okay
