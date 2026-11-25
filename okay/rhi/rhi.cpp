#include "rhi.hpp"

#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace okay {

void VkRhi::initialize() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    vk::ApplicationInfo appInfo{
        .pApplicationName = config.applicationName,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "okay_engine_vk",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion12,
    };

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &appInfo,
    };

    _vkInstance = vk::createInstance(instanceCreateInfo);
    std::vector<vk::PhysicalDevice> physicalDevices = _vkInstance.enumeratePhysicalDevices();

    for (vk::PhysicalDevice &pd : physicalDevices) {

    }


    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkInstance);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkDevice);
}

}  // namespace okay
