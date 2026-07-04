#ifndef __PROC_INTERFACE_H__
#define __PROC_INTERFACE_H__

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <sockpp/error.h>
#include <sockpp/platform.h>
#include <sockpp/poller.h>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_connector.h>
#include <sockpp/tcp_socket.h>
#include <span>
#include <vector>

namespace okay::editor {

#ifdef ERROR
#undef ERROR
#endif

enum class ProcMessageKind : std::uint8_t { REQUEST, RESPONSE, ERROR_MESSAGE };

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
    static constexpr in_port_t PORT = 0xBEEF;
    static constexpr std::size_t HEADER_SIZE = 4;
    static constexpr std::size_t MAX_PAYLOAD_SIZE = 4096;

    using RxCallback = std::function<void(ProcMessageHeader, std::span<std::uint8_t>)>;

    ProcInterface();
    ~ProcInterface();

    ProcInterface(const ProcInterface&) = delete;
    ProcInterface& operator=(const ProcInterface&) = delete;

    bool initialize();
    void shutdown();
    void tick();

    ProcInterface& addCallback(ProcContentKind contentKind, ProcInterface::RxCallback callback);

    static ProcMessageHeader parseHeader(const std::uint8_t* data);
    static void writeHeader(std::uint8_t* data,
        ProcMessageKind messageKind,
        ProcContentKind contentKind,
        std::uint16_t payloadLength);

   private:
    sockpp::error_code _accEc{};
    std::unique_ptr<sockpp::tcp_acceptor> _acc;
    sockpp::poller _poller;
    std::list<sockpp::tcp_socket> _connections;

    std::map<std::uint8_t, std::vector<RxCallback>> _rxCallbacks;
};

}  // namespace okay::editor

#endif  // __PROC_INTERFACE_H__
