#include "proc_interface.hpp"

#include <okay/runtime/runtime.hpp>

#include <sockpp/socket.h>

namespace okay::editor {

ProcInterface::ProcInterface() = default;

ProcInterface::~ProcInterface() {
    shutdown();
}

bool ProcInterface::initialize() {
    shutdown();
    sockpp::initialize();
    return true;
}

void ProcInterface::shutdown() {}

void ProcInterface::tick() {
    // listen to new client connections
    if (auto res = _acc.accept(); !res) {
        // error
        Runtime.logger.error("Error accepting connection! {}", _accEc.message());
    } else {
        Runtime.logger.debug("Accepted connection!");
    }
}

ProcInterface& ProcInterface::addCallback(ProcContentKind contentKind, RxCallback callback) {
    _rxCallbacks[static_cast<std::uint8_t>(contentKind)].push_back(callback);
    return *this;
}

bool ProcInterface::send(ProcMessageKind messageKind,
    ProcContentKind contentKind,
    std::span<const std::uint8_t> payload) {
    return false;
}

bool ProcInterface::sendRequest(
    ProcContentKind contentKind, std::span<const std::uint8_t> payload) {
    return send(ProcMessageKind::REQUEST, contentKind, payload);
}

bool ProcInterface::sendResponse(
    ProcContentKind contentKind, std::span<const std::uint8_t> payload) {
    return send(ProcMessageKind::RESPONSE, contentKind, payload);
}

bool ProcInterface::sendError(ProcContentKind contentKind, std::span<const std::uint8_t> payload) {
    return send(ProcMessageKind::ERROR_MESSAGE, contentKind, payload);
}

ProcMessageHeader ProcInterface::parseHeader(const std::uint8_t* data) {
    ProcMessageHeader header{};
    header.kind = static_cast<ProcMessageKind>(data[0]);
    header.payloadKind = data[1];
    header.payloadLength = static_cast<std::uint16_t>((data[2] << 8) | data[3]);
    return header;
}

void ProcInterface::writeHeader(std::uint8_t* data,
    ProcMessageKind messageKind,
    ProcContentKind contentKind,
    std::uint16_t payloadLength) {
    data[0] = static_cast<std::uint8_t>(messageKind);
    data[1] = static_cast<std::uint8_t>(contentKind);
    data[2] = static_cast<std::uint8_t>(payloadLength >> 8);
    data[3] = static_cast<std::uint8_t>(payloadLength & 0xFF);
}

}  // namespace okay::editor
