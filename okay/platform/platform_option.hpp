// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// platform_option.hpp - Okay Engine

#pragma once

#include <okay/util/option.hpp>

#include <algorithm>
#include <array>
#include <span>

namespace okay {

enum class PreferredFallback { USE_ACCEPTABLE, REQUIRE_PREFERRED };

enum class UnavailableFallback { USE_ANY_SUPPORTED, FAIL };

template <typename T, std::size_t N>
struct PlatformOption {
    T preferred;
    std::array<T, N> acceptable;

    PreferredFallback onPreferredUnavailable{PreferredFallback::USE_ACCEPTABLE};
    UnavailableFallback onNoneAvailable{UnavailableFallback::FAIL};

    Option<T> select(std::span<const T> supported) const {
        auto contains = [&](const T& value) {
            return std::find(supported.begin(), supported.end(), value) != supported.end();
        };

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

        if (onNoneAvailable == UnavailableFallback::USE_ANY_SUPPORTED && !supported.empty()) {
            return supported.front();
        }

        return Option<T>::none();
    }
};

}  // namespace okay
