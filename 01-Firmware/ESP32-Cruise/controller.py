"""Cruise computation state. No CAN ARM, FOC or actuator writes in this module."""
from pid import SpeedPid
from settings import finite


class Button:
    def __init__(self, debounce_ms, ticks_diff):
        self.debounce_ms, self.diff = debounce_ms, ticks_diff
        self.reset()

    def reset(self):
        self.stable = self.candidate = True  # Release before the first engage edge.
        self.since_ms = None

    def pressed(self, raw, now_ms):
        if raw != self.candidate or self.since_ms is None:
            self.candidate, self.since_ms = raw, now_ms
        if raw != self.stable and self.diff(now_ms, self.since_ms) >= self.debounce_ms:
            self.stable = raw
            return raw
        return False


class ControlResult:
    def __init__(self, active, target_speed_mps, throttle, reason):
        self.active, self.target_speed_mps = active, target_speed_mps
        self.throttle, self.reason = throttle, reason


class CruiseController:
    def __init__(self, settings, ticks_diff):
        settings.validate()
        self.minimum_speed_mps = settings.minimum_speed_mps
        self.maximum_speed_mps = settings.maximum_speed_mps
        self.maximum_gap_ms = settings.maximum_loop_gap_ms
        self.diff = ticks_diff
        self.pid = SpeedPid(settings.kp, settings.ki, settings.kd, settings.derivative_filter_s)
        self.button = Button(settings.button_debounce_ms, ticks_diff)
        self.last_ms = None
        self.active, self.target_speed_mps = False, 0.0

    def disable(self, reason, reset_button=False):
        self.active, self.target_speed_mps = False, 0.0
        self.pid.reset(0.0)
        if reset_button:
            self.button.reset()
        return ControlResult(False, 0.0, 0.0, reason)

    def update(self, now_ms, inputs, speed, initial_throttle=0.0):
        elapsed = 0 if self.last_ms is None else self.diff(now_ms, self.last_ms)
        self.last_ms = now_ms
        pressed = self.button.pressed(inputs.button_active, now_ms)
        # Raw brake/clutch assertion cancels immediately, without debounce delay.
        if inputs.brake_active:
            return self.disable("brake", reset_button=True)
        if inputs.clutch_active:
            return self.disable("clutch", reset_button=True)
        if elapsed < 0 or elapsed > self.maximum_gap_ms:
            return self.disable("loop_gap", reset_button=True)
        if (not speed.valid or not finite(speed.speed_mps)
                or not self.minimum_speed_mps <= speed.speed_mps <= self.maximum_speed_mps):
            return self.disable(speed.reason if not speed.valid else "speed_out_of_range",
                                reset_button=True)
        if pressed:
            if self.active:
                return self.disable("button_off")
            if not finite(initial_throttle) or not 0 <= initial_throttle <= 1:
                return self.disable("invalid_initial_throttle")
            self.pid.reset(speed.speed_mps, initial_throttle)
            self.target_speed_mps, self.active = speed.speed_mps, True
            return ControlResult(True, self.target_speed_mps, initial_throttle, "engaged")
        if not self.active:
            return ControlResult(False, 0.0, 0.0, "disabled")
        try:
            output = (self.pid.update(self.target_speed_mps, speed.speed_mps, elapsed / 1000.0)
                      if elapsed else self.pid.output)
        except (ValueError, OverflowError):
            return self.disable("pid_arithmetic", reset_button=True)
        return ControlResult(True, self.target_speed_mps, output, "active")
