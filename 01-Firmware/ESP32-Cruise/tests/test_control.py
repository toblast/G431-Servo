"""Host checks use synthetic calibration, never motorcycle defaults."""
import math
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from settings import Settings
from pid import SpeedPid
from speed import WheelSpeed, SpeedSample
from inputs import GpioInputs, InputSample
from controller import CruiseController
from app import Application


def configured():
    value = Settings()
    value.wheel_circumference_m = 2.0
    value.pulses_per_revolution = 50
    value.minimum_speed_mps, value.maximum_speed_mps = 5.0, 60.0
    value.kp, value.ki, value.kd = 0.1, 0.05, 0.01
    value.speed_pin, value.brake_pin = 13, 14
    value.clutch_pin, value.button_pin = 25, 26
    value.brake_active_level = value.button_active_level = 1
    value.clutch_active_level = 0
    return value


class Clock:
    # Small wrapping period forces realistic ticks_diff use in short tests.
    def __init__(self, now=0):
        self.now = now

    def ticks_ms(self):
        return self.now % 1024

    @staticmethod
    def ticks_diff(new, old):
        return (new - old + 512) % 1024 - 512


class Source:
    def __init__(self):
        self.count = 0
        self.brake = self.clutch = self.button = self.fail = False

    def read(self):
        if self.fail:
            raise OSError("simulated input failure")
        return InputSample(self.count, self.brake, self.clutch, self.button)


class Fixture:
    def __init__(self, now=0):
        self.config, self.source, self.clock = configured(), Source(), Clock(now)
        self.app = Application(self.config, self.source, self.clock)
        self.result = self.app.step()

    def step(self, pulses=5, dt=20):
        self.source.count += pulses
        self.clock.now += dt
        self.result = self.app.step()
        return self.result

    def warm(self):
        for _ in range(10):
            self.step()  # 5 pulses/20 ms at 0.04 m/pulse = 10 m/s.

    def press(self):
        self.source.button = True
        for _ in range(3):
            self.step()
        return self.result

    def release(self):
        self.source.button = False
        for _ in range(3):
            self.step()

    def engage(self):
        self.warm()
        self.press()
        assert self.result.active
        return self


class SettingsTests(unittest.TestCase):
    def test_required_values_and_finite_gains(self):
        with self.assertRaises(ValueError):
            Settings().validate()
        for name in ("wheel_circumference_m", "kp", "ki", "kd", "maximum_speed_mps"):
            for bad in (None, math.nan, math.inf, -1, True):
                settings = configured()
                setattr(settings, name, bad)
                with self.subTest(name=name, bad=bad), self.assertRaises(ValueError):
                    settings.validate()

    def test_pin_and_timing_validation(self):
        configured().validate(require_pins=True)
        for name, bad in (("brake_pin", 13), ("speed_pin", 6), ("clutch_active_level", None),
                          ("counter_id", 8), ("loop_period_ms", 0),
                          ("maximum_loop_gap_ms", 500), ("pulses_per_revolution", 0)):
            settings = configured()
            setattr(settings, name, bad)
            with self.subTest(name=name), self.assertRaises(ValueError):
                settings.validate(require_pins=True)


class PidTests(unittest.TestCase):
    def test_proportional_units_and_limits(self):
        pid = SpeedPid(0.1, 0, 0, 0)
        pid.reset(18)
        self.assertAlmostEqual(pid.update(20, 18, 0.1), 0.2)
        self.assertEqual(pid.update(100, 18, 0.1), 1)
        self.assertEqual(pid.update(0, 18, 0.1), 0)

    def test_integral_uses_elapsed_seconds(self):
        pid = SpeedPid(0, 0.1, 0, 0)
        self.assertAlmostEqual(pid.update(1, 0, 0.5), 0.05)
        self.assertAlmostEqual(pid.update(1, 0, 0.25), 0.075)

    def test_conditional_antiwindup(self):
        pid = SpeedPid(1, 1, 0, 0)
        for _ in range(1000):
            self.assertEqual(pid.update(100, 0, 0.1), 1)
        self.assertEqual(pid.integral, 0)
        self.assertEqual(pid.update(0, 0, 0.1), 0)

    def test_derivative_on_measurement_and_initial_tracking(self):
        pid = SpeedPid(0, 0, 0.1, 0)
        pid.reset(10, 0.5)
        self.assertEqual(pid.update(20, 10, 0.1), 0.5)  # Target step creates no D kick.
        self.assertAlmostEqual(pid.update(20, 10.1, 0.1), 0.4)
        pid.reset(10, 0.25)
        self.assertEqual(pid.output, 0.25)
        self.assertEqual(pid.derivative, 0)

    def test_invalid_samples_do_not_commit_state(self):
        pid = SpeedPid(0.1, 0.1, 0.1, 0.1)
        pid.reset(10, 0.3)
        for target, measured, dt in ((math.nan, 10, 0.1), (10, math.inf, 0.1),
                                      (10, 10, 0), (10, 10, -1)):
            with self.assertRaises(ValueError):
                pid.update(target, measured, dt)
            self.assertEqual((pid.output, pid.previous_measurement), (0.3, 10))
        pid = SpeedPid(1e308, 0, 0, 0)
        with self.assertRaises(ValueError):
            pid.update(100, 0, 0.1)


class SpeedTests(unittest.TestCase):
    def test_calibration_window_and_large_native_counts(self):
        speed = WheelSpeed(configured(), Clock.ticks_diff)
        self.assertFalse(speed.update(2**40, 0).valid)
        for now in (20, 40, 60, 80):
            self.assertFalse(speed.update(2**40 + now // 4, now).valid)
        sample = speed.update(2**40 + 25, 100)
        self.assertTrue(sample.valid)
        self.assertAlmostEqual(sample.speed_mps, 10)

    def test_timeout_retains_last_speed_then_invalidates(self):
        speed = WheelSpeed(configured(), Clock.ticks_diff)
        speed.update(0, 0)
        for now in range(20, 101, 20):
            speed.update(now // 4, now)
        for now in range(120, 600, 20):
            sample = speed.update(25, now)
            self.assertTrue(sample.valid)
            self.assertEqual(sample.speed_mps, 10)
        sample = speed.update(25, 600)
        self.assertFalse(sample.valid)
        self.assertEqual((sample.speed_mps, sample.reason), (0, "speed_timeout"))

    def test_reset_gap_and_glitch(self):
        for kind in ("reset", "gap", "glitch", "invalid"):
            speed = WheelSpeed(configured(), Clock.ticks_diff)
            speed.update(100, 0)
            for now in range(20, 101, 20):
                speed.update(100 + now // 4, now)
            if kind == "reset":
                sample = speed.update(0, 120)
            elif kind == "gap":
                sample = speed.update(150, 250)
            elif kind == "invalid":
                sample = speed.update(None, 120)
            else:
                for now in range(120, 201, 20):
                    sample = speed.update(10000, now)
            self.assertFalse(sample.valid, kind)
            self.assertEqual(sample.speed_mps, 0)

    def test_tick_rollover(self):
        speed = WheelSpeed(configured(), Clock.ticks_diff)
        speed.update(0, 1000)
        for i in range(1, 6):
            sample = speed.update(i * 5, (1000 + i * 20) % 1024)
        self.assertTrue(sample.valid)
        self.assertAlmostEqual(sample.speed_mps, 10)


class ControllerTests(unittest.TestCase):
    def test_startup_and_one_button_capture_toggle(self):
        fixture = Fixture()
        self.assertFalse(fixture.result.active)
        fixture.engage()
        self.assertAlmostEqual(fixture.result.target_speed_mps, 10)
        fixture.release()
        fixture.press()
        self.assertFalse(fixture.result.active)
        self.assertEqual(fixture.result.throttle, 0)

    def test_held_startup_button_never_engages(self):
        fixture = Fixture()
        fixture.source.button = True
        fixture.warm()
        for _ in range(50):
            self.assertFalse(fixture.step().active)
        fixture.release()
        self.assertTrue(fixture.press().active)

    def test_brake_clutch_cancel_and_require_new_press(self):
        for name in ("brake", "clutch"):
            fixture = Fixture().engage()
            setattr(fixture.source, name, True)
            self.assertFalse(fixture.step().active)
            self.assertEqual(fixture.result.reason, name)
            setattr(fixture.source, name, False)
            for _ in range(10):
                self.assertFalse(fixture.step().active)  # Held button cannot resume.
            fixture.release()
            self.assertTrue(fixture.press().active)

    def test_press_started_during_brake_is_not_delayed_engagement(self):
        fixture = Fixture()
        fixture.warm()
        fixture.source.brake = fixture.source.button = True
        fixture.step()
        fixture.source.brake = False  # Less than one debounce interval later.
        for _ in range(10):
            self.assertFalse(fixture.step().active)
        fixture.release()
        self.assertTrue(fixture.press().active)

    def test_speed_loss_counter_reset_and_loop_delay_cancel(self):
        for kind in ("timeout", "reset", "delay"):
            fixture = Fixture().engage()
            if kind == "timeout":
                for _ in range(25):
                    fixture.step(pulses=0)
            elif kind == "reset":
                fixture.source.count = 0
                fixture.step(pulses=0)
            else:
                fixture.step(dt=200)
            self.assertFalse(fixture.result.active, kind)
            self.assertEqual(fixture.result.throttle, 0)
            fixture.warm()
            self.assertFalse(fixture.result.active)

    def test_input_error_is_visible_and_does_not_resume(self):
        fixture = Fixture().engage()
        fixture.source.fail = True
        self.assertEqual(fixture.step().reason, "input_error")
        self.assertIsInstance(fixture.app.last_input_error, OSError)
        fixture.source.fail = False
        fixture.warm()
        self.assertFalse(fixture.result.active)
        fixture.release()
        self.assertTrue(fixture.press().active)

    def test_speed_pid_corrects_error_and_rolls_over(self):
        fixture = Fixture(now=980).engage()
        fixture.release()
        for _ in range(10):
            fixture.step(pulses=4)  # Speed falls from 10 to 8 m/s.
        self.assertTrue(fixture.result.active)
        self.assertGreater(fixture.result.throttle, 0)
        self.assertLessEqual(fixture.result.throttle, 1)
        self.assertEqual(fixture.result.target_speed_mps, 10)

    def test_invalid_speed_and_pid_arithmetic_cancel(self):
        controller = CruiseController(configured(), Clock.ticks_diff)
        inputs = InputSample(10, False, False, False)
        result = controller.update(0, inputs, SpeedSample(math.nan, True, "valid"))
        self.assertFalse(result.active)
        fixture = Fixture().engage()
        fixture.app.controller.pid.kp = math.inf  # Simulated corrupt runtime gain.
        self.assertFalse(fixture.step().active)
        self.assertEqual(fixture.result.reason, "pid_arithmetic")


class FakePin:
    IN = 0
    values = {}
    calls = []

    def __init__(self, number, mode):
        self.number = number
        self.calls.append((number, mode))

    def value(self):
        return self.values[self.number]


class FakeCounter:
    def __init__(self, number, pin):
        self.number, self.pin, self.count = number, pin, 0

    def value(self):
        return self.count

    def deinit(self):
        self.closed = True


class Machine:
    Pin, Counter = FakePin, FakeCounter


class HardwareBoundaryTests(unittest.TestCase):
    def setUp(self):
        FakePin.calls = []
        FakePin.values = {13: 0, 14: 0, 25: 1, 26: 0}

    def test_gpio_mapping_counter_and_polarity(self):
        inputs = GpioInputs(configured(), Machine)
        inputs.counter.count = 123456
        sample = inputs.read()
        self.assertEqual(sample.pulse_count, 123456)
        self.assertFalse(sample.brake_active or sample.clutch_active or sample.button_active)
        FakePin.values[14], FakePin.values[25], FakePin.values[26] = 1, 0, 1
        sample = inputs.read()
        self.assertTrue(sample.brake_active and sample.clutch_active and sample.button_active)
        self.assertEqual(len(FakePin.calls), 4)
        inputs.close()
        self.assertTrue(inputs.counter.closed)

    def test_configuration_and_counter_capability_checked_before_gpio(self):
        with self.assertRaises(ValueError):
            GpioInputs(Settings(), Machine)
        class NoCounter:
            Pin = FakePin
        with self.assertRaises(RuntimeError):
            GpioInputs(configured(), NoCounter)
        self.assertEqual(FakePin.calls, [])

    def test_bad_input_levels_are_rejected(self):
        inputs = GpioInputs(configured(), Machine)
        FakePin.values[14] = None
        with self.assertRaises(ValueError):
            inputs.read()
        with self.assertRaises(ValueError):
            InputSample(0, None, False, False)

    def test_input_and_speed_reading_do_not_require_pid_tuning(self):
        settings = configured()
        settings.kp = settings.ki = settings.kd = None
        self.assertIsInstance(GpioInputs(settings, Machine).read(), InputSample)
        self.assertFalse(WheelSpeed(settings, Clock.ticks_diff).update(0, 0).valid)
        with self.assertRaises(ValueError):
            CruiseController(settings, Clock.ticks_diff)


if __name__ == "__main__":
    unittest.main()
