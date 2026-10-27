"""MicroPython bench entry point: read inputs and print computed demand only."""
from settings import Settings

settings = Settings()
# Fill the required pins, input polarities, wheel calibration, speed range and
# PID gains here after measuring/selecting them. None is intentionally invalid.


def run(config):
    config.validate(require_pins=True)
    import time
    from inputs import GpioInputs
    from app import Application
    inputs = GpioInputs(config)
    last_report_ms = time.ticks_ms()
    last_reason = None
    try:
        application = Application(config, inputs, time)
        while True:
            result = application.step()
            now_ms = time.ticks_ms()
            if result.reason != last_reason or time.ticks_diff(now_ms, last_report_ms) >= 500:
                print(result.reason, "speed_mps", application.speed.speed_mps,
                      "target_mps", result.target_speed_mps, "throttle", result.throttle)
                if application.last_input_error is not None:
                    print("input error:", application.last_input_error)
                last_reason, last_report_ms = result.reason, now_ms
            time.sleep_ms(config.loop_period_ms)
    finally:
        inputs.close()


if __name__ == "__main__":
    try:
        run(settings)
    except ValueError as error:
        print("ESP32 cruise configuration required:", error)
