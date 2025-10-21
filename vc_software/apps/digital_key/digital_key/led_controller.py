"""GPIO-based LED indicators for Digital Key status."""
from __future__ import annotations

import logging
import os
from typing import Optional

LOGGER = logging.getLogger(__name__)

try:  # pragma: no cover - hardware optional during testing
    import RPi.GPIO as GPIO  # type: ignore
except ImportError:  # pragma: no cover - fallback on non-RPi environments
    GPIO = None  # type: ignore[assignment]


_LOCK_PIN_ENV = "DK_LED_LOCK_PIN"
_ENGINE_PIN_ENV = "DK_LED_ENGINE_PIN"
_ACTIVE_LOW_ENV = "DK_LED_ACTIVE_LOW"


def _as_bool(value: str | None, default: bool = False) -> bool:
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "on"}


class VehicleLedController:
    """Drive two LEDs that reflect lock/engine state."""

    def __init__(self, lock_pin: int, engine_pin: int, *, active_low: bool = False) -> None:
        self._lock_pin = lock_pin
        self._engine_pin = engine_pin
        self._active_low = active_low
        self._last_lock: Optional[bool] = None
        self._last_engine: Optional[bool] = None
        self._enabled = GPIO is not None

        if not self._enabled:
            LOGGER.warning(
                "VehicleLedController disabled: RPi.GPIO not available. "
                "Set DK_LED_LOCK_PIN/DK_LED_ENGINE_PIN only on hardware targets."
            )
            return

        # Configure GPIO pins for output; use BCM numbering.
        if GPIO.getmode() is None:
            GPIO.setmode(GPIO.BCM)
        for pin in {self._lock_pin, self._engine_pin}:
            GPIO.setup(pin, GPIO.OUT, initial=self._gpio_level(False))

        LOGGER.info(
            "Vehicle LED controller initialized (lock_pin=%s, engine_pin=%s, active_low=%s)",
            self._lock_pin,
            self._engine_pin,
            self._active_low,
        )

    @property
    def enabled(self) -> bool:
        return self._enabled

    def update(self, *, locked: Optional[bool] = None, engine_on: Optional[bool] = None) -> None:
        """Update LEDs based on latest vehicle state."""
        if not self._enabled:
            return

        if locked is not None and locked != self._last_lock:
            self._write(self._lock_pin, locked)
            self._last_lock = locked
            LOGGER.debug("Updated lock LED -> %s", "ON" if locked else "OFF")

        if engine_on is not None and engine_on != self._last_engine:
            self._write(self._engine_pin, engine_on)
            self._last_engine = engine_on
            LOGGER.debug("Updated engine LED -> %s", "ON" if engine_on else "OFF")

    def cleanup(self) -> None:
        if not self._enabled:
            return
        for pin in {self._lock_pin, self._engine_pin}:
            try:
                GPIO.output(pin, self._gpio_level(False))
            except Exception:  # pragma: no cover - best effort cleanup
                LOGGER.debug("Failed to drive pin %s low during cleanup", pin)
        try:
            GPIO.cleanup([self._lock_pin, self._engine_pin])
        except Exception:  # pragma: no cover - best effort cleanup
            LOGGER.debug("GPIO cleanup failed for pins: %s, %s", self._lock_pin, self._engine_pin)

    def _write(self, pin: int, value: bool) -> None:
        level = self._gpio_level(value)
        try:
            GPIO.output(pin, level)
        except Exception as exc:  # pragma: no cover - hardware failure reporting
            LOGGER.error("Failed to drive GPIO pin %s: %s", pin, exc)

    def _gpio_level(self, state: bool) -> int:
        assert GPIO is not None
        return GPIO.LOW if (state ^ (not self._active_low)) else GPIO.HIGH


def build_led_controller_from_env() -> VehicleLedController | None:
    """Instantiate VehicleLedController when environment specifies pin mapping."""
    lock_pin_raw = os.environ.get(_LOCK_PIN_ENV)
    engine_pin_raw = os.environ.get(_ENGINE_PIN_ENV)

    if not lock_pin_raw or not engine_pin_raw:
        LOGGER.debug(
            "LED controller not created: define both %s and %s to enable hardware indicators.",
            _LOCK_PIN_ENV,
            _ENGINE_PIN_ENV,
        )
        return None

    try:
        lock_pin = int(lock_pin_raw, 0)
        engine_pin = int(engine_pin_raw, 0)
    except ValueError as exc:
        LOGGER.error("Invalid GPIO pin value: %s", exc)
        return None

    active_low = _as_bool(os.environ.get(_ACTIVE_LOW_ENV))
    controller = VehicleLedController(lock_pin, engine_pin, active_low=active_low)
    return controller if controller.enabled else None


__all__ = ["VehicleLedController", "build_led_controller_from_env"]

