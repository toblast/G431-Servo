"""Conditioned digital inputs only. Hardware access is isolated from control."""


class InputSample:
    def __init__(self, pulse_count, brake_active, clutch_active, button_active):
        if not all(type(value) is bool for value in (brake_active, clutch_active, button_active)):
            raise ValueError("Input active states must be boolean")
        self.pulse_count = pulse_count
        self.brake_active = brake_active
        self.clutch_active = clutch_active
        self.button_active = button_active


class GpioInputs:
    def __init__(self, settings, machine_module=None):
        settings.validate_inputs()  # Before any GPIO side effects; independent of PID tuning.
        if machine_module is None:
            import machine as machine_module
        counter_type = getattr(machine_module, "Counter", None)
        if counter_type is None:
            raise RuntimeError("This MicroPython build needs machine.Counter/ESP32 PCNT support")
        self.settings = settings
        pin = machine_module.Pin
        # No inferred pull/polarity: external conditioning supplies defined levels.
        self.speed_pin = pin(settings.speed_pin, pin.IN)
        self.brake_pin = pin(settings.brake_pin, pin.IN)
        self.clutch_pin = pin(settings.clutch_pin, pin.IN)
        self.button_pin = pin(settings.button_pin, pin.IN)
        self.counter = counter_type(settings.counter_id, self.speed_pin)

    @staticmethod
    def _active(pin, level):
        value = pin.value()
        if value not in (0, 1):
            raise ValueError("Invalid conditioned input level")
        return value == level

    def read(self):
        # Native hardware counter captures rising pulses while Python is busy.
        count = self.counter.value()
        return InputSample(
            count, self._active(self.brake_pin, self.settings.brake_active_level),
            self._active(self.clutch_pin, self.settings.clutch_active_level),
            self._active(self.button_pin, self.settings.button_active_level))

    def close(self):
        """Release the native counter when the bench loop stops."""
        self.counter.deinit()
