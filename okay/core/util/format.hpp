#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "glm/fwd.hpp"

#include <okay/core/renderer/render_world.hpp>

#include <format>
#include <string>

// format-specifiers for std::format that aren't included in the standard library

template <>
struct std::formatter<glm::vec2> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const glm::vec2& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template <>
struct std::formatter<glm::vec3> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const glm::vec3& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template <>
struct std::formatter<glm::vec4> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const glm::vec4& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};

template <>
struct std::formatter<glm::quat> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const glm::quat& q, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", q.w, q.x, q.y, q.z);
    }
};

template <>
struct std::formatter<glm::mat4> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const glm::mat4& m, FormatContext& ctx) const {
        return std::format_to(ctx.out(),
                              "[[{}, {}, {}, {}], [{}, {}, {}, {}], [{}, {}, {}, {}], [{}, {}, "
                              "{}, {}]]",
                              m[0][0],
                              m[0][1],
                              m[0][2],
                              m[0][3],
                              m[1][0],
                              m[1][1],
                              m[1][2],
                              m[1][3],
                              m[2][0],
                              m[2][1],
                              m[2][2],
                              m[2][3],
                              m[3][0],
                              m[3][1],
                              m[3][2],
                              m[3][3]);
    }
};

template <>
struct std::formatter<okay::Transform> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const okay::Transform& t, FormatContext& ctx) const {
        return std::format_to(
            ctx.out(), "position: {}, scale: {}, rotation: {}", t.position, t.scale, t.rotation);
    }
};

#endif  // __FORMAT_H__