"""Outer road-speed PID; no motor-current or position integral is added."""
from settings import finite


def clamp(value, low, high):
    return max(low, min(high, value))


class SpeedPid:
    def __init__(self, kp, ki, kd, filter_s):
        if any(not finite(x) or x < 0 for x in (kp, ki, kd, filter_s)):
            raise ValueError("Invalid PID gains/filter")
        self.kp, self.ki, self.kd, self.filter_s = kp, ki, kd, filter_s
        self.reset(0.0)

    def reset(self, measurement_mps, initial_output=0.0):
        if not finite(measurement_mps) or not finite(initial_output) or not 0 <= initial_output <= 1:
            raise ValueError("Invalid PID reset/tracking value")
        self.previous_measurement = measurement_mps
        self.derivative = 0.0
        self.integral = initial_output  # Track an explicitly supplied initial throttle.
        self.output = initial_output

    def update(self, target_mps, measured_mps, dt_s):
        if not all(finite(x) for x in (target_mps, measured_mps, dt_s)) or dt_s <= 0:
            raise ValueError("Invalid PID sample/time")
        error = target_mps - measured_mps
        # Derivative on measurement avoids a target-change derivative kick.
        slope = -(measured_mps - self.previous_measurement) / dt_s
        alpha = dt_s / (self.filter_s + dt_s)
        derivative = self.derivative + alpha * (slope - self.derivative)
        integral = self.integral + self.ki * error * dt_s
        proportional = self.kp * error
        candidate = proportional + integral + self.kd * derivative
        if not all(finite(x) for x in (derivative, integral, proportional, candidate)):
            raise ValueError("Nonfinite PID arithmetic")
        # Conditional integration: do not accumulate further into saturation.
        if (candidate > 1.0 and error > 0) or (candidate < 0.0 and error < 0):
            integral = self.integral
        raw_output = proportional + integral + self.kd * derivative
        if not finite(raw_output):
            raise ValueError("Nonfinite PID output")
        output = clamp(raw_output, 0.0, 1.0)
        self.previous_measurement = measured_mps
        self.derivative, self.integral, self.output = derivative, integral, output
        return output
