import argparse
import hashlib
import json
import os
import sys

from config import VIN_ENV_VAR, resolve_vin
from ota_publisher import parse_meta_argument, publish_ota_message

BASE_DIR = os.path.dirname(__file__)
BUILD_DIR = os.path.join(BASE_DIR, "build")


def calc_checksum(file_path: str) -> str:
    """Calculate the SHA-256 checksum for a local file."""
    hasher = hashlib.sha256()
    with open(file_path, "rb") as fp:
        for chunk in iter(lambda: fp.read(8192), b""):
            hasher.update(chunk)
    return hasher.hexdigest()


def update_checksum_in_json(json_path: str) -> dict[str, object]:
    """
    Update the 'checksum' field in the update payload and persist it back to disk.
    Returns the in-memory update payload dictionary.
    """
    with open(json_path, "r", encoding="utf-8") as fp:
        update_payload = json.load(fp)

    source_name = os.path.basename(update_payload["target"]["source_path"])
    local_source = os.path.join(BUILD_DIR, source_name)

    checksum = calc_checksum(local_source)
    print(f"[Checksum] {source_name} -> {checksum}")

    update_payload["checksum"] = checksum

    with open(json_path, "w", encoding="utf-8") as fp:
        json.dump(update_payload, fp, ensure_ascii=False, indent=2)

    print(f"[JSON] Updated checksum in {os.path.basename(json_path)}.")
    return update_payload


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compute checksum and publish OTA notify payload."
    )
    parser.add_argument("json_path", help="Path to the update payload JSON file.")
    parser.add_argument("--vin", help=f"Target VIN. Falls back to {VIN_ENV_VAR}.")
    parser.add_argument(
        "--version",
        help="Override the notify message version (defaults to update['version']).",
    )
    parser.add_argument(
        "--re-prompt-sec",
        type=int,
        help="Seconds before the bridge re-prompts the user (optional).",
    )
    parser.add_argument(
        "--meta",
        help="Inline JSON object or path to JSON file merged into the notify meta field.",
    )
    parser.add_argument(
        "--no-repeat",
        action="store_true",
        help="알림을 한 번만 발행하고 종료합니다.",
    )
    parser.add_argument(
        "--max-repeat",
        type=int,
        help="승인 대기 중 재발행 최대 횟수(기본값 무제한).",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    if not os.path.exists(args.json_path):
        parser.error(f"Update payload file not found: {args.json_path}")

    print(f"[OTA] Preparing payload from {args.json_path}")
    update_payload = update_checksum_in_json(args.json_path)

    try:
        vin = resolve_vin(args.vin)
    except RuntimeError as exc:
        parser.error(str(exc))

    try:
        meta = parse_meta_argument(args.meta)
    except (ValueError, json.JSONDecodeError) as exc:
        parser.error(f"Invalid --meta value: {exc}")

    publish_ota_message(
        update_payload,
        vin=vin,
        version=args.version,
        re_prompt_sec=args.re_prompt_sec,
        meta=meta,
        repeat_until_ack=not args.no_repeat,
        max_repeat=args.max_repeat,
    )
    print(f"[OTA] Notify published for VIN {vin}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
