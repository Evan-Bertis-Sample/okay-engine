#ifndef __PROC_INTERFACE_H__
#define __PROC_INTERFACE_H__

#include <cstdint>

namespace okay::editor {

enum class ProcMessageType : std::uint8_t {

};

struct ProcMessageHeader {
    ProcMessageType type;
};

}  // namespace okay::editor

#endif  // __PROC_INTERFACE_H__
