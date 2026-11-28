// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// platform_option.hpp - Okay Engine

#pragma once

#include <okay/util/option.hpp>

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace okay {

enum class PreferredFallback { USE_ACCEPTABLE, REQUIRE_PREFERRED };

enum class UnavailableFallback { USE_ANY_SUPPORTED, FAIL };

template <typename T>
struct PlatformOption {
    T preferred;
    std::vector<T> acceptable;

    PreferredFallback onPreferredUnavailable{PreferredFallback::USE_ACCEPTABLE};
    UnavailableFallback onNoneAvailable{UnavailableFallback::FAIL};

    Option<T> select(std::span<const T> supported, Option<T> fallback = Option<T>::none()) const {
        auto contains = [&](const T& value) {
            return std::find(supported.begin(), supported.end(), value) != supported.end();
        };

        if (fallback.isNone() && !supported.empty()) {
            fallback = supported.front();
        }

        return selectImpl(contains, fallback);
    }

    Option<T> selectFromVkFlags(
        vk::Flags<T> supported, Option<T> fallback = Option<T>::none()) const {
        auto contains = [&](T value) {
            return (supported & value) != vk::Flags<T>{};
        };

        if (fallback.isNone() && supported) {
            using Mask = typename vk::Flags<T>::MaskType;

            Mask bits = static_cast<Mask>(supported);
            Mask lowestBit = bits & (Mask{0} - bits);

            fallback = static_cast<T>(lowestBit);
        }

        return selectImpl(contains, fallback);
    }

   private:
    template <typename Func>
    Option<T> selectImpl(Func&& contains, Option<T> fallback) const {
        if (contains(preferred)) {
            return preferred;
        }

        if (onPreferredUnavailable == PreferredFallback::REQUIRE_PREFERRED) {
            return Option<T>::none();
        }

        for (const T& option : acceptable) {
            if (contains(option)) {
                return option;
            }
        }

        if (onNoneAvailable == UnavailableFallback::USE_ANY_SUPPORTED) {
            return fallback;
        }

        return Option<T>::none();
    }
};

namespace PlatformOptions {

template <typename T>
PlatformOption<T> onlyAccept(T value) {
    return {
        .preferred = value,
        .acceptable = {},
        .onPreferredUnavailable = PreferredFallback::REQUIRE_PREFERRED,
        .onNoneAvailable = UnavailableFallback::FAIL,
    };
}

}  // namespace PlatformOptions

}  // namespace okay
