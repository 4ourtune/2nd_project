# digital_key Service

Lightweight BLE digital key prototype for Raspberry Pi. Key entry points:

- `scripts/install_system_deps.sh`: install BlueZ/DBus prerequisites (run once per Pi).
- `scripts/setup_ble_env.sh`: create/update the Python virtualenv used for BLE services.
- `scripts/init_vehicle.sh`: bootstrap environment, auto-approve pairing, and launch the BLE peripheral.
- `digital_key/vehicle_ble.py`: BLE server implementation.

Runtime artifacts (logs, session exports) are written into `logs/` and cache data lives in `~/.cache/dks`.
