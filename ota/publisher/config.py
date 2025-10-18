import json
import os
from typing import Any, Dict

# MQTT broker connection details
BROKER_HOST = "192.168.137.1"
BROKER_PORT = 1883

# Vehicle-specific notify topic template
TOPIC_TEMPLATE = "vc/{vin}/ota/vehicle_control/notify"
ACK_TOPIC_TEMPLATE = "vc/{vin}/ota/vehicle_control/ack"

# Environment variable names
VIN_ENV_VAR = "VC_VIN"
RE_PROMPT_ENV_VAR = "VC_OTA_REPROMPT_SEC"
META_ENV_VAR = "VC_OTA_META"

# Defaults for notify payload
DEFAULT_RE_PROMPT_SEC = 30
DEFAULT_META: Dict[str, Any] = {}


def get_notify_topic(vin: str) -> str:
    """Build the MQTT notify topic for the given VIN."""
    return TOPIC_TEMPLATE.format(vin=vin)


def get_ack_topic(vin: str) -> str:
    """Build the MQTT acknowledgment topic for the given VIN."""
    return ACK_TOPIC_TEMPLATE.format(vin=vin)


def resolve_vin(explicit_vin: str | None = None) -> str:
    """
    Determine the VIN to target. Prefer explicit CLI input and fall back to env.
    """
    vin = explicit_vin or os.environ.get(VIN_ENV_VAR)
    if not vin:
        raise RuntimeError(
            f"VIN was not provided. Pass --vin or set the {VIN_ENV_VAR} environment variable."
        )
    return vin


def resolve_re_prompt_sec(explicit_value: int | None = None) -> int:
    """Pick the re-prompt interval, preferring CLI override, then env, then default."""
    if explicit_value is not None:
        return explicit_value

    env_value = os.environ.get(RE_PROMPT_ENV_VAR)
    if env_value is None:
        return DEFAULT_RE_PROMPT_SEC

    try:
        return int(env_value)
    except ValueError as exc:
        raise RuntimeError(
            f"Invalid integer for {RE_PROMPT_ENV_VAR}: {env_value}"
        ) from exc


def resolve_meta(explicit_meta: Dict[str, Any] | None = None) -> Dict[str, Any]:
    """
    Pick the meta document. CLI override wins, then JSON string in env, then default.
    """
    if explicit_meta is not None:
        merged = dict(DEFAULT_META)
        merged.update(explicit_meta)
        return merged

    env_value = os.environ.get(META_ENV_VAR)
    if not env_value:
        return dict(DEFAULT_META)

    try:
        parsed = json.loads(env_value)
    except json.JSONDecodeError as exc:
        raise RuntimeError(
            f"Invalid JSON found in {META_ENV_VAR}: {env_value}"
        ) from exc
    if not isinstance(parsed, dict):
        raise RuntimeError(
            f"Expected {META_ENV_VAR} to contain a JSON object, got {type(parsed).__name__}"
        )
    merged = dict(DEFAULT_META)
    merged.update(parsed)
    return merged


# Local OTA asset directory
BASE_DIR = os.path.dirname(__file__)
