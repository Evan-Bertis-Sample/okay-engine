#ifndef __TYPE_H__
#define __TYPE_H__

namespace okay {
struct NoneType {};

template <typename>
inline constexpr bool dependent_false_v = false;

};  // namespace okay

#endif  // __TYPE_H__