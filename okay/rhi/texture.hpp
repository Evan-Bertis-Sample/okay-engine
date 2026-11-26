// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// texture.hpp - Okay Engine

#pragma once

#include <okay/util/logger.hpp>
#include <okay/util/result.hpp>

#include <vulkan/vulkan.hpp>

namespace okay::rhi {

enum class TextureFormat : std::uint8_t {
    // srgb (nonlinear)
    RGBA8 = static_cast<uint8_t>(vk::Format::eR8G8B8A8Srgb),
    RGBA16 = static_cast<uint8_t>(vk::Format::eR16G16B16A16Sfloat),

    // unorm [0.0, 1.0]
    RGBA8_UNORM = static_cast<uint8_t>(vk::Format::eR8G8B8A8Unorm),
    RGBA16_UNORM = static_cast<uint8_t>(vk::Format::eR8G8B8A8Unorm),

    // snorm [-1.0, 1.0]
    RGBA8_SNORM = static_cast<uint8_t>(vk::Format::eR8G8B8A8Snorm),
    RGBA16_SNORM = static_cast<uint8_t>(vk::Format::eR8G8B8A8Snorm),
};

struct TextureDescription {
    TextureFormat format;
    std::size_t width, height;
};

class Texture {
   private:
    vk::Image _image;
};

}  // namespace okay::rhi
