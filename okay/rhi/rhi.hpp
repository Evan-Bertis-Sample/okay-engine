// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// rhi.hpp - Okay Engine

#pragma once

#include <vulkan/vulkan.hpp>
#include <okay/util/logger.hpp>

namespace okay {

struct VkRhiConfig {
    const char* applicationName;
};

class VkRhi {
   public:
    VkRhiConfig config;


    VkRhi(const VkRhiConfig& config) : config(config) {}

    void initialize();

   private:
    vk::Instance _vkInstance;
    vk::Device _vkDevice;
};

}  // namespace okay
