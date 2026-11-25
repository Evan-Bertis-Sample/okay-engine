#include "rhi.hpp"

#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace okay {

void VkRhi::initialize() {
    vk::ApplicationInfo appInfo{
        .pApplicationName = config.applicationName,
        .applicationVersion = 1,
        .pEngineName = "okay_engine_vk",
        .engineVersion = 1,
        .apiVersion = vk::ApiVersion12,
    };

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &appInfo,
    };

    _vkInstance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkInstance);

    // std::vector<vk::PhysicalDevice> physicalDevices = _vkInstance.enumeratePhysicalDevices();
}

}  // namespace okay
