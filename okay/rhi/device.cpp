#include "device.hpp"

#include "vulkan/vulkan.hpp"

#include <okay/platform/surface.hpp>
#include <okay/rhi/texture.hpp>

#include <array>
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace okay::rhi {

static std::vector<const char*> VK_LAYERS = {"VK_LAYER_KHRONOS_validation"};

static std::vector<const char*> VK_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

Failable RenderDevice::initialize() {
    if (_init) {
        return Failable::errorResult("Render device already initialized!");
    }

    // Instance initialization

    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    vk::ApplicationInfo appInfo{
        .pApplicationName = config.applicationName,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "okay_engine_vk",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion12,
    };

    _surface.initialize();
    std::vector<const char*> requiredExtensions = _surface.requiredVulkanExtensions();

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<std::uint32_t>(VK_LAYERS.size()),
        .ppEnabledLayerNames = VK_LAYERS.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    _vkInstance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkInstance);

    std::vector<vk::PhysicalDevice> physicalDevices = _vkInstance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        return Failable::errorResult("No physical devices!");
    }

    // Device initialization

    _vkPhysicalDevice = physicalDevices[0];

    vk::SurfaceKHR surfaceKHR = _surface.createVulkanSurface(_vkInstance);
    vk::SurfaceCapabilitiesKHR surfaceCaps =
        _vkPhysicalDevice.getSurfaceCapabilitiesKHR(surfaceKHR);

    std::vector<vk::QueueFamilyProperties> qFamilyProps =
        _vkPhysicalDevice.getQueueFamilyProperties();
    bool hasGraphicsQueue = false;
    uint32_t queueFamilyIndex = 0;
    for (std::uint32_t i = 0; i < qFamilyProps.size(); ++i) {
        if ((qFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            _vkPhysicalDevice.getSurfaceSupportKHR(i, surfaceKHR)) {
            queueFamilyIndex = i;
            hasGraphicsQueue = true;
            break;
        }
    }

    if (!hasGraphicsQueue) {
        return Failable::errorResult("No graphics queue supported!");
    }

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<std::uint32_t>(VK_DEVICE_EXTENSIONS.size()),
        .ppEnabledExtensionNames = VK_DEVICE_EXTENSIONS.data(),
    };

    _vkDevice = _vkPhysicalDevice.createDevice(deviceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(_vkDevice);

    _vkGraphicsQueue = _vkDevice.getQueue(queueFamilyIndex, 0);

    // Swapchain initialization

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface = surfaceKHR,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = vk::Format::eR8G8B8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = surfaceCaps.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
    };

    _swapchain = _vkDevice.createSwapchainKHR(swapchainCreateInfo);

    _init = true;
    return Failable::ok();
}

void RenderDevice::shutdown() {
    if (!_init)
        return;

    _surface.destroy();
    _vkDevice.destroy();
    _vkInstance.destroy();
}

}  // namespace okay::rhi
