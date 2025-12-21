#ifndef __TYPE_H__
#define __TYPE_H__

namespace okay {
struct NoneType {};

template <typename>
inline constexpr bool dependent_false_v = false;

template <std::size_t N>
struct FixedString {
    char value[N]{};

    constexpr FixedString(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) value[i] = str[i];
    }

    constexpr std::string_view sv() const { return {value, N - 1}; }  // drop '\0'
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

};  // namespace okay

#endif  // __TYPE_H__