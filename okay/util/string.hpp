// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// string.hpp - Okay Engine

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace okay {

class StringUtils final {
   public:
    static std::string ToLower(std::string s);

    // Trim leading whitespace
    static std::string LTrim(std::string s);

    // StartsWith for std::string_view
    static bool StartsWith(std::string_view s, std::string_view prefix);

    // Split by a delimiter (keeps empty fields if keepEmpty=true)
    static std::vector<std::string_view> Split(
        std::string_view s, char delim, bool keepEmpty = true);
};

}  // namespace okay
