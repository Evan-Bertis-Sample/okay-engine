#include "proc_interface.hpp"

namespace okay::editor {

ProcInterface::ProcInterface() = default;

ProcInterface::~ProcInterface() {
    shutdown();
}

bool ProcInterface::initialize(std::uint16_t port) {
    shutdown();
    return true;
}

bool ProcInterface::initalize(std::uint16_t port) {
    return initialize(port);
}

void ProcInterface::shutdown() {}

void ProcInterface::tick() {}

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
    return send(ProcMessageKind::ERROR, contentKind, payload);
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
