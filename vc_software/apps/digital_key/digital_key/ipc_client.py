"""Lightweight Unix-domain socket client for realtime vehicle control."""
from __future__ import annotations

import socket
import time
from typing import Tuple

# Socket exposed by realtime ipc_server.cpp. Permissions are managed so that
# members of the vc group (digital_key, ota, etc.) can access it.
SOCK_PATH = "/run/vc/ipc/realtime.sock"


def send_cmd(cmd: str, src: str, *, timeout: float = 0.3) -> Tuple[bool, str]:
    """Send a command to the realtime process.

    Args:
        cmd: One of LOCK, UNLOCK, START, GET_ALL.
        src: Caller identifier (e.g., "DK" for digital key).
        timeout: Socket timeout in seconds.

    Returns:
        Tuple of (success flag, raw response string).
    """
    req = int(time.time() * 1000)
    message = f"CMD={cmd};REQ={req};SRC={src}\n".encode()

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.settimeout(timeout)
    try:
        sock.connect(SOCK_PATH)
        sock.send(message)
        response = sock.recv(256).decode()
        ok = response.startswith("OK;") and f"REQ={req}" in response
        return ok, response.strip()
    except Exception as exc:  # pragma: no cover - thin IPC wrapper
        return False, f"ERR;{exc}"
    finally:
        sock.close()
