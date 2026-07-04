#ifndef __PROC_INTERFACE_H__
#define __PROC_INTERFACE_H__

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <vector>

namespace okay::editor {

enum class ProcMessageKind : std::uint8_t { REQUEST, RESPONSE, ERROR };

enum class ProcContentKind : std::uint8_t { HOT_RELOAD_ASSETS, HOT_RELOAD_CODE };

struct ProcMessageHeader {
    ProcMessageKind kind;
    std::uint8_t payloadKind;
    std::uint16_t payloadLength;
};

struct ProcMessage {
    ProcMessageHeader header;
    std::uint8_t* payload;
};

class ProcInterface {
   public:
    static constexpr std::uint16_t PORT = 0xBEEF;
    static constexpr std::size_t HEADER_SIZE = 4;
    static constexpr std::size_t MAX_PAYLOAD_SIZE = 4096;

    using RxCallback = std::function<void(std::span<std::uint8_t>)>;

    ProcInterface();
    ~ProcInterface();

    ProcInterface(const ProcInterface&) = delete;
    ProcInterface& operator=(const ProcInterface&) = delete;

    bool initialize(std::uint16_t port = PORT);
    bool initalize(std::uint16_t port = PORT);

    void shutdown();
    void tick();

    ProcInterface& addCallback(ProcContentKind contentKind, ProcInterface::RxCallback callback);

    bool send(ProcMessageKind messageKind,
        ProcContentKind contentKind,
        std::span<const std::uint8_t> payload = {});

    bool sendRequest(ProcContentKind contentKind, std::span<const std::uint8_t> payload = {});

    bool sendResponse(ProcContentKind contentKind, std::span<const std::uint8_t> payload = {});

    bool sendError(ProcContentKind contentKind, std::span<const std::uint8_t> payload = {});

    static ProcMessageHeader parseHeader(const std::uint8_t* data);
    static void writeHeader(std::uint8_t* data,
        ProcMessageKind messageKind,
        ProcContentKind contentKind,
        std::uint16_t payloadLength);

    std::map<std::uint8_t, std::vector<RxCallback>> _rxCallbacks;
};

}  // namespace okay::editor

#endif  // __PROC_INTERFACE_H__
