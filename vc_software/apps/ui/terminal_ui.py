#!/usr/bin/env python3
# 사용법: pip3 install paho-mqtt → 환경변수(MQTT_HOST=PC 브로커 IP 등) 설정 → python3 terminal_ui.py 실행
# 본 스크립트는 OTA 알림과 디지털키 PIN 결과를 터미널에서 확인하고 단일 키 입력(p/y/n/s)으로 응답합니다.
from __future__ import annotations

import json
import os
import random
import select
import socket
import string
import sys
import threading
import time
import termios
import tty
from dataclasses import dataclass
from typing import Any, Optional

import paho.mqtt.client as mqtt


DEFAULT_MQTT_HOST = os.environ.get("MQTT_HOST", "192.168.137.1")  # PC 브로커 IP
DEFAULT_MQTT_PORT = int(os.environ.get("MQTT_PORT", "1883"))
DEFAULT_VIN = os.environ.get("VEHICLE_VIN", "TESTVIN0000000000")


def _default_client_id_suffix() -> str:
    hostname = socket.gethostname()
    rand = "".join(random.choices(string.ascii_lowercase + string.digits, k=6))
    return f"{hostname}-{rand}"


CLIENT_ID_SUFFIX = os.environ.get("CLIENT_ID_SUFFIX", _default_client_id_suffix())
CLIENT_ID = f"vc-terminal-ui-{CLIENT_ID_SUFFIX}"


def prefixed(topic_tail: str, vin: str = DEFAULT_VIN) -> str:
    return f"vc/{vin}/{topic_tail}"


def ensure_json_dict(raw: str) -> dict[str, Any]:
    data = json.loads(raw)
    if not isinstance(data, dict):
        raise ValueError("JSON payload is not an object")
    return data


@dataclass
class NotifyState:
    version: str
    payload: dict[str, Any]
    update_version: Optional[str]
    re_prompt_sec: Optional[int]


class ExponentialBackoff:
    def __init__(self, base: int = 1, factor: int = 2, maximum: int = 30) -> None:
        self._base = base
        self._factor = factor
        self._maximum = maximum
        self._current = base
        self._lock = threading.Lock()

    def reset(self) -> None:
        with self._lock:
            self._current = self._base

    def wait(self) -> None:
        with self._lock:
            delay = self._current
            self._current = min(self._current * self._factor, self._maximum)
        print(f"[MQTT] 재연결 대기 {delay}초...", flush=True)
        time.sleep(delay)


class TerminalUI:
    def __init__(self) -> None:
        self._client = mqtt.Client(client_id=CLIENT_ID, clean_session=True)
        self._client.enable_logger(None)
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message
        self._backoff = ExponentialBackoff()
        self._disconnect_lock = threading.Lock()
        self._reconnecting = False
        self._stop_event = threading.Event()
        self._pending_notify: Optional[NotifyState] = None
        self._pending_lock = threading.Lock()

    def _on_connect(self, client: mqtt.Client, userdata: Any, flags: dict[str, Any], rc: int) -> None:
        if rc == 0:
            print(f"[MQTT] 연결 성공 → {DEFAULT_MQTT_HOST}:{DEFAULT_MQTT_PORT}", flush=True)
            self._backoff.reset()
            notify_topic = prefixed("ota/vehicle_control/notify")
            pin_topic = prefixed("digital_key/pairing/pin")
            status_topic = prefixed("digital_key/pairing/status")
            client.subscribe([(notify_topic, 1), (pin_topic, 1), (status_topic, 1)])
            print(f"[MQTT] 구독: {notify_topic}, {pin_topic}, {status_topic}", flush=True)
        else:
            print(f"[MQTT] 연결 실패 rc={rc}", flush=True)

    def _on_disconnect(self, client: mqtt.Client, userdata: Any, rc: int) -> None:
        if self._stop_event.is_set():
            return
        if rc != 0:
            print(f"[MQTT] 예기치 않은 연결 종료 rc={rc}", flush=True)
        self._schedule_reconnect()

    def _schedule_reconnect(self) -> None:
        with self._disconnect_lock:
            if self._reconnecting or self._stop_event.is_set():
                return
            self._reconnecting = True

        def _worker() -> None:
            while not self._stop_event.is_set():
                try:
                    print("[MQTT] 재연결 시도...", flush=True)
                    self._client.reconnect()
                    print("[MQTT] 재연결 성공", flush=True)
                    self._backoff.reset()
                    break
                except Exception as exc:  # pylint: disable=broad-except
                    print(f"[MQTT] 재연결 실패: {exc}", flush=True)
                    self._backoff.wait()
            with self._disconnect_lock:
                self._reconnecting = False

        threading.Thread(target=_worker, daemon=True).start()

    def _on_message(self, client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage) -> None:
        try:
            payload = msg.payload.decode("utf-8")
        except UnicodeDecodeError:
            print(f"[MQTT] 디코딩 실패: {msg.topic}", flush=True)
            return

        print(f"[MQTT] 수신 {msg.topic}: {payload}", flush=True)
        try:
            data = ensure_json_dict(payload)
        except Exception as exc:  # pylint: disable=broad-except
            print(f"[MQTT] JSON 파싱 실패: {exc}", flush=True)
            return

        notify_topic = prefixed("ota/vehicle_control/notify")
        if msg.topic == notify_topic:
            version = str(data.get("version", "unknown"))
            update = data.get("update")
            update_version = None
            if isinstance(update, dict):
                update_version = str(update.get("version", version))
            re_prompt_raw = data.get("re_prompt_sec")
            re_prompt_sec = None
            if isinstance(re_prompt_raw, (int, float)):
                re_prompt_sec = int(re_prompt_raw)
            with self._pending_lock:
                self._pending_notify = NotifyState(
                    version=version,
                    payload=data,
                    update_version=update_version,
                    re_prompt_sec=re_prompt_sec,
                )
            display_version = update_version or version
            print(f"=== OTA 업데이트 감지: v{display_version} ===", flush=True)
            if re_prompt_sec is not None:
                print(f"(재알림 간격: {re_prompt_sec}초)", flush=True)
            print("적용하시겠습니까? (y:예 / n:아니오 / s:세부보기)", flush=True)
            return

        pin_topic = prefixed("digital_key/pairing/pin")
        if msg.topic == pin_topic:
            self._handle_pin_response(data)
            return

        status_topic = prefixed("digital_key/pairing/status")
        if msg.topic == status_topic:
            self._handle_status(data)

    def _handle_pin_response(self, data: dict[str, Any]) -> None:
        vin = data.get("vin", DEFAULT_VIN)
        if "error" in data:
            print("=== 디지털키 PIN 발급 실패 ===", flush=True)
            print(f"VIN: {vin}", flush=True)
            print(f"사유: {data.get('error')}", flush=True)
            return

        pin = data.get("pin")
        ttl = data.get("ttl_sec")
        if not pin:
            print("[PIN] 응답에 pin 필드가 없습니다.", flush=True)
            return

        print("=== 디지털키 PIN 발급 ===", flush=True)
        print(f"VIN: {vin}", flush=True)
        print(f"PIN: {pin}", flush=True)
        if ttl is not None:
            print(f"유효시간: {ttl}초", flush=True)

    def _handle_status(self, data: dict[str, Any]) -> None:
        status = data.get("status")
        if status == "registered":
            print("차량 등록되었습니다!", flush=True)
        else:
            reason = data.get("reason", "")
            msg = f"디지털키 상태 변경: {status}"
            if reason:
                msg += f" (사유: {reason})"
            print(msg, flush=True)

    def publish_pin_request(self) -> None:
        request_topic = prefixed("ui/dk/pin/request")
        request = {
            "vin": DEFAULT_VIN,
            "ttl_sec": 180,
            "req_id": f"req-{int(time.time())}",
        }
        payload = json.dumps(request, ensure_ascii=False)
        result = self._client.publish(request_topic, payload=payload, qos=1, retain=False)
        result.wait_for_publish()
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"[PIN] PIN 요청 발행 → {request_topic}", flush=True)
        else:
            print(f"[PIN] 발행 실패 rc={result.rc}", flush=True)

    def _publish_ack(self, decision: str, pending: NotifyState) -> None:
        topic = prefixed("ota/vehicle_control/ack")
        payload = {
            "decision": decision,
            "version": pending.update_version or pending.version,
        }
        result = self._client.publish(topic, payload=json.dumps(payload, ensure_ascii=False), qos=1, retain=False)
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"[ACK] 발행 완료({decision}) → {topic}", flush=True)
        else:
            print(f"[ACK] 발행 실패 rc={result.rc}", flush=True)

    def publish_decision(self, decision: str) -> None:
        with self._pending_lock:
            pending = self._pending_notify
        if not pending:
            print("[OTA] 대기 중인 알림이 없습니다.", flush=True)
            return
        if decision not in {"yes", "no"}:
            print(f"[OTA] 알 수 없는 결정: {decision}", flush=True)
            return

        topic = prefixed("ui/ota/decision")
        payload = json.dumps(
            {"decision": decision, "version": pending.version},
            ensure_ascii=False,
        )
        result = self._client.publish(topic, payload=payload, qos=1, retain=False)
        result.wait_for_publish()
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"[OTA] 결정 발행({decision}) → {topic}", flush=True)
        else:
            print(f"[OTA] 결정 발행 실패 rc={result.rc}", flush=True)

        ack_decision = "approved" if decision == "yes" else "declined"
        self._publish_ack(ack_decision, pending)

        with self._pending_lock:
            self._pending_notify = None

        if decision == "no":
            print("재알림은 나중에 notify가 다시 올 때 뜹니다.", flush=True)

    def show_pending(self) -> None:
        with self._pending_lock:
            pending = self._pending_notify
        if not pending:
            print("[OTA] 대기 중 알림이 없습니다.", flush=True)
            return
        print("=== 현재 대기 중 OTA 알림 ===", flush=True)
        display_version = pending.update_version or pending.version
        print(f"버전: {display_version}", flush=True)
        if pending.re_prompt_sec is not None:
            print(f"재알림 간격: {pending.re_prompt_sec}초", flush=True)
        print(json.dumps(pending.payload, ensure_ascii=False, indent=2, sort_keys=True), flush=True)

    def connect(self) -> None:
        delay = 1
        while not self._stop_event.is_set():
            try:
                print(f"[MQTT] 연결 시도 → {DEFAULT_MQTT_HOST}:{DEFAULT_MQTT_PORT}", flush=True)
                self._client.connect(DEFAULT_MQTT_HOST, DEFAULT_MQTT_PORT, keepalive=60)
                print("[MQTT] 연결 완료, 수신 대기 시작", flush=True)
                self._backoff.reset()
                break
            except Exception as exc:  # pylint: disable=broad-except
                print(f"[MQTT] 연결 실패: {exc}", flush=True)
                print(f"[MQTT] {delay}초 후 재시도", flush=True)
                time.sleep(delay)
                delay = min(delay * 2, 30)

        self._client.loop_start()

    def stop(self) -> None:
        self._stop_event.set()
        try:
            self._client.disconnect()
        except Exception:  # pylint: disable=broad-except
            pass
        self._client.loop_stop()


def configure_terminal() -> termios.tcgetattr:
    fd = sys.stdin.fileno()
    original_attr = termios.tcgetattr(fd)
    tty.setcbreak(fd)
    return original_attr


def restore_terminal(attr: termios.tcgetattr) -> None:
    fd = sys.stdin.fileno()
    termios.tcsetattr(fd, termios.TCSADRAIN, attr)


def main() -> None:
    print("키 입력 도움말: p=PIN 요청, y=OTA 승인, n=OTA 거절, s=알림 상세 보기", flush=True)
    ui = TerminalUI()
    original_terminal = configure_terminal()
    try:
        ui.connect()
        while True:
            if ui._stop_event.is_set():  # pylint: disable=protected-access
                break
            if select.select([sys.stdin], [], [], 0.2)[0]:
                ch = sys.stdin.read(1)
                if ch == "p":
                    ui.publish_pin_request()
                elif ch == "y":
                    ui.publish_decision("yes")
                elif ch == "n":
                    ui.publish_decision("no")
                elif ch == "s":
                    ui.show_pending()
                elif ch in ("\x03", "\x04"):
                    raise KeyboardInterrupt
            time.sleep(0.05)
    except KeyboardInterrupt:
        print("\n[시스템] 사용자 종료 요청(Ctrl+C)", flush=True)
    finally:
        ui.stop()
        restore_terminal(original_terminal)
        print("[시스템] 터미널 UI 종료", flush=True)


if __name__ == "__main__":
    main()
