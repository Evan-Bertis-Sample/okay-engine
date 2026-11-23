#pragma once

namespace okay {

// Credit to https://en.cppreference.com/w/cpp/utility/variant/visit2.html
template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace okay
