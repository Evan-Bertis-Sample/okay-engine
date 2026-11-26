#include "rhi.hpp"

#include "vulkan/vulkan.hpp"

#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace okay {

Failable VkRhi::initialize() {
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
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkInstance);

    std::vector<vk::PhysicalDevice> physicalDevices = _vkInstance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        return Failable::errorResult("No physical devices!");
    }

    _vkDevice = physicalDevices[0].createDevice({}, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkDevice);
    return Failable::ok();
}

}  // namespace okay
