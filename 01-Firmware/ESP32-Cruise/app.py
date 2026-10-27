"""One input/control pass. Outputs are diagnostic demand, not motor commands."""
from speed import WheelSpeed
from controller import CruiseController


class Application:
    def __init__(self, settings, input_source, clock):
        self.inputs, self.clock = input_source, clock
        self.speed = WheelSpeed(settings, clock.ticks_diff)
        self.controller = CruiseController(settings, clock.ticks_diff)
        self.last_input_error = None

    def step(self):
        try:
            inputs = self.inputs.read()
        except Exception as error:
            self.last_input_error = error
            self.speed.reset()
            return self.controller.disable("input_error", reset_button=True)
        now_ms = self.clock.ticks_ms()
        sample = self.speed.update(inputs.pulse_count, now_ms)
        self.last_input_error = None
        return self.controller.update(now_ms, inputs, sample)
