"""Pulse-count windows -> motorcycle speed. The counter is monotonic, not reset."""
from settings import finite


class SpeedSample:
    def __init__(self, speed_mps, valid, reason):
        self.speed_mps, self.valid, self.reason = speed_mps, valid, reason


class WheelSpeed:
    def __init__(self, settings, ticks_diff):
        settings.validate_speed()
        self.distance_per_pulse_m = settings.wheel_circumference_m / settings.pulses_per_revolution
        self.window_ms = settings.speed_window_ms
        self.timeout_ms = settings.speed_timeout_ms
        self.maximum_gap_ms = settings.maximum_loop_gap_ms
        self.maximum_speed_mps = settings.maximum_speed_mps
        self.diff = ticks_diff
        self.reset()

    def reset(self):
        """Discard previous measurements and require a new pulse window."""
        self.last_ms = None
        self.valid = False
        self.speed_mps = 0.0

    def _begin(self, count, now_ms, reason):
        self.last_count = self.window_count = count
        self.last_ms = self.window_start_ms = self.last_motion_ms = now_ms
        self.valid, self.speed_mps = False, 0.0
        return SpeedSample(0.0, False, reason)

    def update(self, count, now_ms):
        if type(count) is not int or count < 0:
            self.reset()
            return SpeedSample(0.0, False, "invalid_counter")
        if self.last_ms is None:
            return self._begin(count, now_ms, "warming_up")
        elapsed = self.diff(now_ms, self.last_ms)
        if elapsed < 0 or elapsed > self.maximum_gap_ms:
            return self._begin(count, now_ms, "sample_gap")
        if count < self.last_count:
            return self._begin(count, now_ms, "counter_reset")
        if count > self.last_count:
            self.last_motion_ms = now_ms
        self.last_count, self.last_ms = count, now_ms
        if self.diff(now_ms, self.last_motion_ms) >= self.timeout_ms:
            return self._begin(count, now_ms, "speed_timeout")
        window = self.diff(now_ms, self.window_start_ms)
        if window >= self.window_ms:
            pulses = count - self.window_count
            self.window_count, self.window_start_ms = count, now_ms
            if pulses:
                speed = pulses * self.distance_per_pulse_m * 1000.0 / window
                if not finite(speed) or speed > self.maximum_speed_mps:
                    return self._begin(count, now_ms, "implausible_speed")
                self.speed_mps, self.valid = speed, True
        return SpeedSample(self.speed_mps, self.valid, "valid" if self.valid else "warming_up")
