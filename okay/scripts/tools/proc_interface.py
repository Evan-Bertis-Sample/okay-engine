import enum
import socket

from tools.tool_util import OkayLogger, OkayLogType


class OkayProcMessageKind(enum.IntEnum):
    REQUEST = 0
    RESPONSE = 1
    ERROR = 2


class OkayProcContentKind(enum.IntEnum):
    HOT_RELOAD_ASSETS = 0
    HOT_RELOAD_CODE = 1


class OkayProcUtil:
    PORT = 0xBEEF
    HOST = "127.0.0.1"

    HEADER_SIZE = 4
    MAX_PAYLOAD_SIZE = 4096
    TIMEOUT_SECONDS = 0.25

    @staticmethod
    def write_header(
        message_kind: OkayProcMessageKind,
        content_kind: OkayProcContentKind,
        payload_length: int,
    ) -> bytes:
        if payload_length < 0 or payload_length > 0xFFFF:
            raise ValueError(f"Payload length does not fit uint16_t: {payload_length}")

        return bytes(
            [
                int(message_kind) & 0xFF,
                int(content_kind) & 0xFF,
                (payload_length >> 8) & 0xFF,
                payload_length & 0xFF,
            ]
        )

    @staticmethod
    def send(
        message_kind: OkayProcMessageKind,
        content_kind: OkayProcContentKind,
        payload: bytes = b"",
        host: str = HOST,
        port: int = PORT,
    ) -> bool:
        if payload is None:
            payload = b""

        if isinstance(payload, str):
            payload = payload.encode("utf-8")

        if len(payload) > OkayProcUtil.MAX_PAYLOAD_SIZE:
            OkayLogger.log(
                f"Proc payload too large: {len(payload)} > {OkayProcUtil.MAX_PAYLOAD_SIZE}",
                OkayLogType.ERROR,
            )
            return False

        header = OkayProcUtil.write_header(message_kind, content_kind, len(payload))
        packet = header + payload

        try:
            with socket.create_connection(
                (host, port),
                timeout=OkayProcUtil.TIMEOUT_SECONDS,
            ) as sock:
                sock.sendall(packet)

            return True

        except OSError as e:
            OkayLogger.log(
                f"Failed to send proc message to {host}:{port}: {e}",
                OkayLogType.WARNING,
            )
            return False

    @staticmethod
    def send_request(
        content_kind: OkayProcContentKind,
        payload: bytes = b"",
        host: str = HOST,
        port: int = PORT,
    ) -> bool:
        return OkayProcUtil.send(
            OkayProcMessageKind.REQUEST,
            content_kind,
            payload,
            host,
            port,
        )

    @staticmethod
    def send_response(
        content_kind: OkayProcContentKind,
        payload: bytes = b"",
        host: str = HOST,
        port: int = PORT,
    ) -> bool:
        return OkayProcUtil.send(
            OkayProcMessageKind.RESPONSE,
            content_kind,
            payload,
            host,
            port,
        )

    @staticmethod
    def send_error(
        content_kind: OkayProcContentKind,
        payload: bytes = b"",
        host: str = HOST,
        port: int = PORT,
    ) -> bool:
        return OkayProcUtil.send(
            OkayProcMessageKind.ERROR,
            content_kind,
            payload,
            host,
            port,
        )

    @staticmethod
    def send_hot_reload_assets(payload: bytes = b"") -> bool:
        return OkayProcUtil.send_request(
            OkayProcContentKind.HOT_RELOAD_ASSETS,
            payload,
        )

    @staticmethod
    def send_hot_reload_code(payload: bytes = b"") -> bool:
        return OkayProcUtil.send_request(
            OkayProcContentKind.HOT_RELOAD_CODE,
            payload,
        )
