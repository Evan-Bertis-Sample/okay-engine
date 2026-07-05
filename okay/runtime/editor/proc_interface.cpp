#include "proc_interface.hpp"

#include <okay/runtime/runtime.hpp>

#include <chrono>
#include <memory>
#include <sockpp/inet_address.h>
#include <sockpp/socket.h>

namespace okay::editor {

ProcInterface::ProcInterface() = default;

ProcInterface::~ProcInterface() {
    shutdown();
}

bool ProcInterface::initialize() {
    shutdown();
    sockpp::initialize();
    _acc = std::make_unique<sockpp::tcp_acceptor>(
        ProcInterface::PORT, sockpp::tcp_acceptor::REUSE, _accEc);
    _poller.add(*_acc, sockpp::poller::POLL_READ);
    return true;
}

void ProcInterface::tick() {
    auto conCheck = _poller.wait(std::chrono::milliseconds{0});

    if (!conCheck) {
        return;
    }

    for (auto& event : conCheck.value()) {
        if (_acc && event.sock == _acc.get()) {
            sockpp::inet_address peer;

            if (auto connRes = _acc->accept(&peer); connRes) {
                Runtime.logger.debug("Proc connection formed from {}", peer.to_string());

                auto& sock = _connections.emplace_back(connRes.release());
                _poller.add(sock, sockpp::poller::POLL_READ);
            } else {
                Runtime.logger.error(
                    "An error occurred while accepting a connection: {}", connRes.error_message());
            }

            continue;
        }

        auto* sock = static_cast<sockpp::stream_socket*>(event.sock);
        std::uint8_t buf[ProcInterface::MAX_PAYLOAD_SIZE];

        auto n = sock->read(buf, sizeof(buf));

        if (!n || n.value() == 0) {
            auto it = std::find_if(_connections.begin(), _connections.end(), [sock](auto& s) {
                return &s == sock;
            });

            if (it != _connections.end()) {
                Runtime.logger.debug("Proc connection closed");
                _poller.remove(*sock);
                _connections.erase(it);
            }

            continue;
        }

        if (n.value() < ProcInterface::HEADER_SIZE) {
            Runtime.logger.warn("Received incomplete proc header: {} bytes", n.value());
            continue;
        }

        ProcMessageHeader header = parseHeader(buf);

        if (header.payloadLength > ProcInterface::MAX_PAYLOAD_SIZE - ProcInterface::HEADER_SIZE) {
            Runtime.logger.warn("Received oversized proc payload: {}", header.payloadLength);
            continue;
        }

        if (n.value() < ProcInterface::HEADER_SIZE + header.payloadLength) {
            Runtime.logger.warn("Received incomplete proc message: got {} bytes, expected {}",
                n.value(),
                ProcInterface::HEADER_SIZE + header.payloadLength);
            continue;
        }

        Runtime.logger.debug("Received proc message kind={}, payloadKind={}, payloadLength={}",
            static_cast<std::uint8_t>(header.kind),
            header.payloadKind,
            header.payloadLength);

        if (_rxCallbacks.contains(header.payloadKind)) {
            for (auto& cb : _rxCallbacks[header.payloadKind]) {
                cb(header, std::span(buf + HEADER_SIZE, header.payloadLength));
            }
        }
    }
}

void ProcInterface::shutdown() {
    for (auto& sock : _connections) {
        _poller.remove(sock);
        sock.close();
    }

    _connections.clear();

    if (_acc) {
        _poller.remove(*_acc);
        _acc->close();
        _acc.reset();
    }
}

ProcInterface& ProcInterface::addCallback(ProcContentKind contentKind, RxCallback callback) {
    _rxCallbacks[static_cast<std::uint8_t>(contentKind)].push_back(callback);
    return *this;
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
