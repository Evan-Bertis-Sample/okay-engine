#include "swapchain.hpp"

#include <vulkan/vulkan.hpp>

namespace okay::rhi {

static std::vector<vk::SurfaceTransformFlagBitsKHR> VK_TRANSFORM_FLAG_BITS{
    vk::SurfaceTransformFlagBitsKHR::eIdentity,
    vk::SurfaceTransformFlagBitsKHR::eRotate90,
    vk::SurfaceTransformFlagBitsKHR::eRotate180,
    vk::SurfaceTransformFlagBitsKHR::eRotate270,
    vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror,
    vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90,
    vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180,
    vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270,
    vk::SurfaceTransformFlagBitsKHR::eInherit,
};

bool Swapchain::create(vk::SurfaceKHR surface,
    vk::Instance instance,
    vk::PhysicalDevice physicalDevice,
    vk::Device device) {
    vk::SurfaceCapabilitiesKHR surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> supportedFormats =
        physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> supportedPresentModes =
        physicalDevice.getSurfacePresentModesKHR(surface);

    Option<vk::SurfaceFormatKHR> format = _config.imageFormat.select(supportedFormats);
    if (format.isNone()) {
        // log
        return false;
    }

    Option<vk::PresentModeKHR> present = _config.presentMode.select(supportedPresentModes);
    if (present.isNone()) {
        // log
        return false;
    }

    Option<vk::SurfaceTransformFlagBitsKHR> transformFlags =
        _config.preTransform.selectFromVkFlags(surfaceCaps.supportedTransforms);
    if (transformFlags.isNone()) {
        // log
        return false;
    }

    return true;
}

}  // namespace okay::rhi
