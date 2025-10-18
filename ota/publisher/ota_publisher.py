import argparse
import json
import threading
import time
from typing import Any, Dict, Optional

import paho.mqtt.client as mqtt

from config import (
    BROKER_HOST,
    BROKER_PORT,
    DEFAULT_META,
    get_ack_topic,
    get_notify_topic,
    resolve_meta,
    resolve_re_prompt_sec,
    resolve_vin,
)


def build_notify_payload(
    update_payload: Dict[str, Any],
    *,
    version: str | None,
    re_prompt_sec: int,
    meta: Dict[str, Any],
) -> Dict[str, Any]:
    """
    Compose the notify payload sent to vc/<VIN>/ota/vehicle_control/notify.
    """
    notify_version = version or update_payload.get("version")
    if not notify_version:
        raise ValueError("No version found for the notification payload.")

    return {
        "version": notify_version,
        "re_prompt_sec": re_prompt_sec,
        "update": update_payload,
        "meta": meta,
    }


APPROVED_VALUES = {"approved", "approve", "accepted", "accept", "ok", "yes", "true"}


def _extract_decision(message: Any) -> Optional[str]:
    """
    Try to extract a textual decision from an ack message payload.
    Returns the lower-cased string if it maps to a known decision.
    """
    if isinstance(message, dict):
        for key in ("decision", "status", "result", "state", "response"):
            value = message.get(key)
            if isinstance(value, bool):
                return "true" if value else "false"
            if isinstance(value, str):
                return value.strip().lower()
    elif isinstance(message, bool):
        return "true" if message else "false"
    elif isinstance(message, str):
        return message.strip().lower()
    return None


def _decode_payload(raw: bytes) -> Any:
    text = raw.decode("utf-8", errors="ignore").strip()
    if not text:
        return ""
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return text


def publish_ota_message(
    update_payload: Dict[str, Any],
    *,
    vin: str,
    version: str | None = None,
    re_prompt_sec: int | None = None,
    meta: Dict[str, Any] | None = None,
    repeat_until_ack: bool = True,
    max_repeat: Optional[int] = None,
) -> None:
    """
    Publish the notify payload to the VIN-scoped topic.
    """
    payload_meta = resolve_meta(meta)
    prompt_interval = resolve_re_prompt_sec(re_prompt_sec)

    notify_payload = build_notify_payload(
        update_payload,
        version=version,
        re_prompt_sec=prompt_interval,
        meta=payload_meta,
    )

    topic = get_notify_topic(vin)
    ack_topic = get_ack_topic(vin)
    serialized = json.dumps(notify_payload, ensure_ascii=False)

    client = mqtt.Client(protocol=mqtt.MQTTv311)
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=30)

    ack_event = threading.Event()

    def handle_ack(_client: mqtt.Client, _userdata, msg: mqtt.MQTTMessage):
        payload = _decode_payload(msg.payload)
        decision = _extract_decision(payload)
        if decision in APPROVED_VALUES:
            print(f"[MQTT] 승인 응답 수신: {payload}")
            ack_event.set()
        else:
            print(f"[MQTT] 승인 대기 중, 응답 수신: {payload}")

    if repeat_until_ack:
        client.message_callback_add(ack_topic, handle_ack)
        client.subscribe(ack_topic, qos=1)

    client.loop_start()
    try:
        publish_count = 0
        while True:
            info = client.publish(topic, payload=serialized, qos=1, retain=False)
            info.wait_for_publish()
            publish_count += 1
            print(
                f"[MQTT] {publish_count}회 발행 완료 -> {BROKER_HOST}:{BROKER_PORT} {topic}"
            )

            if not repeat_until_ack:
                break

            client.subscribe(ack_topic, qos=1)

            if ack_event.wait(timeout=prompt_interval):
                break

            if max_repeat is not None and publish_count >= max_repeat:
                print("[MQTT] 최대 재알림 횟수에 도달하여 종료합니다.")
                break

            print(f"[MQTT] 승인 미수신, {prompt_interval}초 후 재발행 예정")
    finally:
        client.loop_stop()
        client.disconnect()


def _load_json(path: str) -> Dict[str, Any]:
    with open(path, "r", encoding="utf-8") as fp:
        return json.load(fp)


def parse_meta_argument(meta_arg: str | None) -> Dict[str, Any] | None:
    if meta_arg is None:
        return None

    # Allow passing a path or an inline JSON document.
    try:
        with open(meta_arg, "r", encoding="utf-8") as fp:
            return json.load(fp)
    except FileNotFoundError:
        pass

    parsed = json.loads(meta_arg)
    if not isinstance(parsed, dict):
        raise ValueError("--meta must describe a JSON object.")
    return parsed


def main() -> None:
    parser = argparse.ArgumentParser(description="Publish OTA notify payloads.")
    parser.add_argument("json_path", help="Path to the update payload JSON file.")
    parser.add_argument("--vin", help="Target VIN. Falls back to VC_VIN env var.")
    parser.add_argument(
        "--version",
        help="Override the version field sent in the notify payload (defaults to update version).",
    )
    parser.add_argument(
        "--re-prompt-sec",
        type=int,
        help=(
            "Seconds before the bridge re-prompts for approval. "
            "Defaults to VC_OTA_REPROMPT_SEC or configured default."
        ),
    )
    parser.add_argument(
        "--meta",
        help=(
            "Inline JSON object or path to JSON file merged into the meta field. "
            f"Defaults to {DEFAULT_META} or VC_OTA_META."
        ),
    )
    parser.add_argument(
        "--no-repeat",
        action="store_true",
        help="알림을 한 번만 발행하고 종료합니다.",
    )
    parser.add_argument(
        "--max-repeat",
        type=int,
        help="재알림 최대 횟수(승인 대기 시). 기본값은 무제한입니다.",
    )

    args = parser.parse_args()

    update_payload = _load_json(args.json_path)
    try:
        meta = parse_meta_argument(args.meta)
    except (ValueError, json.JSONDecodeError) as exc:
        parser.error(f"Invalid --meta value: {exc}")

    try:
        vin = resolve_vin(args.vin)
    except RuntimeError as exc:
        parser.error(str(exc))
    publish_ota_message(
        update_payload,
        vin=vin,
        version=args.version,
        re_prompt_sec=args.re_prompt_sec,
        meta=meta,
        repeat_until_ack=not args.no_repeat,
        max_repeat=args.max_repeat,
    )
    print(f"[OTA] Notify payload published for VIN {vin}.")


if __name__ == "__main__":
    main()
