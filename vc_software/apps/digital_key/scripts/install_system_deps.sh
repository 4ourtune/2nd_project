#!/usr/bin/env bash
# Install system-level packages required by the digital_key BLE stack.
set -euo pipefail

REQUIRED_PACKAGES=(
  python3
  python3-venv
  python3-dbus
  python3-gi
  gir1.2-glib-2.0
  bluez
  bluez-tools
  bluetooth
)

log() {
  printf '[install-system-deps] %s\n' "$1"
}

run_as_root() {
  if [[ $EUID -ne 0 ]]; then
    if command -v sudo >/dev/null 2>&1; then
      sudo "$@"
    else
      log "sudo가 필요하지만 찾을 수 없습니다. root 사용자로 다시 실행하세요."
      exit 1
    fi
  else
    "$@"
  fi
}

log "Updating apt package index"
run_as_root apt-get update -y

log "Installing required packages: ${REQUIRED_PACKAGES[*]}"
run_as_root apt-get install -y "${REQUIRED_PACKAGES[@]}"

log "Enabling bluetooth service"
run_as_root systemctl enable --now bluetooth.service

log "System dependencies installed. Reboot 후 bluetooth 상태를 확인하세요."
