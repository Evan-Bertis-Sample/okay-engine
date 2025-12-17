#include <okay/core/util/string.hpp>

#include <cctype>

using namespace okay;

std::string StringUtils::ToLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string StringUtils::LTrim(std::string s) {
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    s.erase(0, i);
    return s;
}

bool StringUtils::StartsWith(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

std::vector<std::string_view> StringUtils::Split(std::string_view s, char delim, bool keepEmpty) {
    std::vector<std::string_view> out;
    std::size_t start = 0;

    while (start <= s.size()) {
        std::size_t end = s.find(delim, start);
        if (end == std::string_view::npos) end = s.size();

        std::string_view part = s.substr(start, end - start);
        if (keepEmpty || !part.empty()) out.push_back(part);

        if (end == s.size()) break;
        start = end + 1;
    }

    return out;
}
