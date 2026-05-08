#ifndef __TYPE_H__
#define __TYPE_H__

#include <string_view>
#include <tuple>

namespace okay {
struct NoneType {};

template <typename>
inline constexpr bool dependent_false_v = false;

template <std::size_t N>
struct FixedString {
    char value[N]{};

    constexpr FixedString(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i)
            value[i] = str[i];
    }

    constexpr std::string_view sv() const {
        return {value, N - 1};
    }  // drop '\0'
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;

template <class Tuple, class Fn, std::size_t... I>
static void tupleForEachImpl(Tuple&& t, Fn&& fn, std::index_sequence<I...>) {
    (fn(std::get<I>(t)), ...);
}

template <class Tuple, class Fn>
static void tupleForEach(Tuple&& t, Fn&& fn) {
    constexpr std::size_t N = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    tupleForEachImpl(std::forward<Tuple>(t), std::forward<Fn>(fn), std::make_index_sequence<N>{});
}

};  // namespace okay

#endif  // __TYPE_H__