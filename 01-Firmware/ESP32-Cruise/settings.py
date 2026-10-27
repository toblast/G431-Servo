"""Required calibration and explicit bench timing. No guessed motorcycle pins."""
from math import isfinite


def finite(value):
    return type(value) in (int, float) and isfinite(value)


class Settings:
    def __init__(self):
        self.speed_pin = None
        self.brake_pin = None
        self.clutch_pin = None
        self.button_pin = None
        self.brake_active_level = None
        self.clutch_active_level = None
        self.button_active_level = None
        self.counter_id = 0
        self.wheel_circumference_m = None
        self.pulses_per_revolution = None
        self.minimum_speed_mps = None
        self.maximum_speed_mps = None
        self.kp = None  # Normalized throttle / (m/s).
        self.ki = None  # Normalized throttle / ((m/s) * s).
        self.kd = None  # Normalized throttle / (m/s^2).
        self.derivative_filter_s = 0.1
        self.loop_period_ms = 20
        self.maximum_loop_gap_ms = 100
        self.speed_window_ms = 100
        self.speed_timeout_ms = 500
        self.button_debounce_ms = 30

    def validate(self, require_pins=False):
        self.validate_speed()
        self.validate_pid()
        if require_pins:
            self.validate_inputs()

    def validate_speed(self):
        for name in ("wheel_circumference_m", "minimum_speed_mps", "maximum_speed_mps"):
            value = getattr(self, name)
            if not finite(value) or value <= 0:
                raise ValueError("Configure positive " + name)
        if self.minimum_speed_mps >= self.maximum_speed_mps:
            raise ValueError("Minimum speed must be below maximum speed")
        if type(self.pulses_per_revolution) is not int or self.pulses_per_revolution <= 0:
            raise ValueError("Configure positive integer pulses_per_revolution")
        for name in ("loop_period_ms", "maximum_loop_gap_ms", "speed_window_ms",
                     "speed_timeout_ms", "button_debounce_ms"):
            value = getattr(self, name)
            if type(value) is not int or not 0 < value <= 10000:
                raise ValueError("Invalid timing: " + name)
        if not (self.loop_period_ms <= self.maximum_loop_gap_ms < self.speed_timeout_ms
                and self.loop_period_ms <= self.speed_window_ms < self.speed_timeout_ms):
            raise ValueError("Inconsistent loop/window/timeout settings")

    def validate_pid(self):
        for name in ("kp", "ki", "kd", "derivative_filter_s"):
            value = getattr(self, name)
            if not finite(value) or value < 0:
                raise ValueError("Configure nonnegative " + name)
        if self.kp <= 0:
            raise ValueError("Configure positive kp before running the controller")

    def validate_inputs(self):
        # ESP32-WROOM input-capable pins; excludes flash and default REPL UART.
        allowed = (0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23,
                   25, 26, 27, 32, 33, 34, 35, 36, 39)
        pins = (self.speed_pin, self.brake_pin, self.clutch_pin, self.button_pin)
        if any(type(pin) is not int or pin not in allowed for pin in pins):
            raise ValueError("Configure valid ESP32 input GPIOs")
        if len(set(pins)) != len(pins):
            raise ValueError("Input GPIOs must be distinct")
        for name in ("brake_active_level", "clutch_active_level", "button_active_level"):
            value = getattr(self, name)
            if type(value) is not int or value not in (0, 1):
                raise ValueError("Configure polarity: " + name)
        if type(self.counter_id) is not int or not 0 <= self.counter_id <= 7:
            raise ValueError("Invalid ESP32 PCNT id")
