// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// swapchain.hpp - Okay Engine

#pragma once

#include <okay/platform/platform_option.hpp>
#include <okay/rhi/texture.hpp>
#include <okay/util/logger.hpp>
#include <okay/util/object_pool.hpp>
#include <okay/util/result.hpp>

#include <vulkan/vulkan.hpp>

namespace okay::rhi {

struct SwapchainConfig {
    PlatformOption<vk::SurfaceFormatKHR> imageFormat{
        PlatformOptions::onlyAccept((vk::SurfaceFormatKHR){.format = vk::Format::eR8G8B8A8Snorm,
            .colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear})};

    PlatformOption<vk::SurfaceTransformFlagBitsKHR> preTransform{
        PlatformOptions::onlyAccept(vk::SurfaceTransformFlagBitsKHR::eIdentity)};

    PlatformOption<vk::CompositeAlphaFlagBitsKHR> compositeAlpha{
        PlatformOptions::onlyAccept(vk::CompositeAlphaFlagBitsKHR::eOpaque)};

    PlatformOption<vk::PresentModeKHR> presentMode{
        PlatformOptions::onlyAccept(vk::PresentModeKHR::eFifo)};
};

class Swapchain {
   public:
    Swapchain(SwapchainConfig config) : _config(config) {}

    bool create(vk::SurfaceKHR surface,
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        vk::Device device);

   private:
    SwapchainConfig _config;
};

}  // namespace okay::rhi
