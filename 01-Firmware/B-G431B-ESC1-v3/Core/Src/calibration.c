#include "calibration.h"
#include "foc.h"
#include "drive_faults.h"
#include "motor_config.h"
#include "position_sensor.h"
#include "configuration.h"
#include "math_tool.h"
#include <string.h>

typedef enum {
    CAL_IDLE,
    CAL_COAST,
    CAL_SETTLE,
    CAL_FORWARD,
    CAL_FORWARD_SETTLE,
    CAL_CAPTURE_FORWARD,
    CAL_REVERSE,
    CAL_REVERSE_SETTLE,
    CAL_CAPTURE_REVERSE,
    CAL_ALIGN,
    CAL_CAPTURE_ALIGNMENT,
    CAL_SAVE
} CalibrationPhase;

static struct {
    CalibrationPhase phase;
    uint32_t phase_started_ms;
    int step;
    float mid_angle_rad, pole_pairs;
    MotorAlignment original, alignment;
    CalibrationResult result;
    bool result_pending;
} calibration;

void Calibration_Init(void) { memset(&calibration, 0, sizeof calibration); }

static void enter_phase(CalibrationPhase phase)
{
    calibration.phase = phase;
    // Start each dwell after publishing its target; never compress a dwell to
    // catch up with time spent on acquisition, serial traffic or other work.
    calibration.phase_started_ms = HAL_GetTick();
}

bool Calibration_Start(void)
{
    if (calibration.phase != CAL_IDLE || calibration.result_pending ||
        !API_FOC_Calibration_Begin()) return false;
    calibration.original = control_read_motor_alignment();
    calibration.alignment = (MotorAlignment){ .phase_offset_deg = 0, .invert_phase_motor = 0 };
    enter_phase(CAL_COAST);
    return true;
}

static void complete(CalibrationResult result)
{
    uint32_t const primask = __get_PRIMASK();
    __disable_irq();
    API_FOC_Calibration_Idle();
    if (result != CALIBRATION_STORAGE_ERROR && (API_FOC_Calibration_Cancelled() ||
        hardware_fault_status() != 0 || !positionSensor_isReady())) {
        result = CALIBRATION_CANCELLED;
    }
    if (result != CALIBRATION_SUCCESS) control_write_motor_alignment(&calibration.original);
    calibration.result = result;
    calibration.result_pending = true;
    calibration.phase = CAL_IDLE;
    // Keep ARM inhibited until all cleanup, rollback and result publication are
    // complete. Nothing may write motor/alignment state after this guard ends.
    API_FOC_Calibration_End();
    __set_PRIMASK(primask);
}

static void excite(float angle_rad, CalibrationPhase phase)
{
    if (!API_FOC_Calibration_SetFlux(angle_rad, 3.0f)) {
        complete(CALIBRATION_CANCELLED);
        return;
    }
    enter_phase(phase);
}

void Calibration_Update(bool acquisition_serviced)
{
    if (calibration.phase == CAL_IDLE) return;
    // Recheck the shared watchdog after potentially slow foreground work too.
    if (!API_FOC_Check_Watchdog() || API_FOC_Calibration_Cancelled() ||
        hardware_fault_status() != 0 || !positionSensor_isReady()) {
        complete(CALIBRATION_CANCELLED);
        return;
    }
    uint32_t const elapsed_ms = (uint32_t)(HAL_GetTick() - calibration.phase_started_ms);
    switch (calibration.phase) {
    case CAL_COAST:
        if (elapsed_ms < 200U) return;
        control_write_motor_alignment(&calibration.alignment);
        excite(M_3PI_2, CAL_SETTLE);
        return;
    case CAL_SETTLE:
        if (elapsed_ms < 100U) return;
        calibration.step = 0;
        excite(M_3PI_2, CAL_FORWARD);
        return;
    case CAL_FORWARD:
        if (elapsed_ms < 2U) return;
        if (calibration.step == 500) {
            enter_phase(CAL_FORWARD_SETTLE);
            return;
        }
        ++calibration.step;
        excite(M_3PI_2 + M_2PI * calibration.step / 500.0f, CAL_FORWARD);
        return;
    case CAL_FORWARD_SETTLE:
        if (elapsed_ms >= 200U) enter_phase(CAL_CAPTURE_FORWARD);
        return;
    case CAL_CAPTURE_FORWARD:
        if (!acquisition_serviced) return;
        calibration.mid_angle_rad = positionSensor_getRadiansMultiturn();
        calibration.step = 500;
        excite(M_3PI_2 + M_2PI * calibration.step / 500.0f, CAL_REVERSE);
        return;
    case CAL_REVERSE:
        if (elapsed_ms < 2U) return;
        if (calibration.step == 0) {
            enter_phase(CAL_REVERSE_SETTLE);
            return;
        }
        --calibration.step;
        excite(M_3PI_2 + M_2PI * calibration.step / 500.0f, CAL_REVERSE);
        return;
    case CAL_REVERSE_SETTLE:
        if (elapsed_ms >= 200U) enter_phase(CAL_CAPTURE_REVERSE);
        return;
    case CAL_CAPTURE_REVERSE: {
        if (!acquisition_serviced) return;
        float const end_angle_rad = positionSensor_getRadiansMultiturn();
        API_FOC_Calibration_Idle();
        float const delta_angle_rad = calibration.mid_angle_rad - end_angle_rad;
        if (!isfinite(delta_angle_rad) || fabsf(delta_angle_rad) < 0.1f) {
            complete(CALIBRATION_NO_MOTION);
            return;
        }
        calibration.alignment.invert_phase_motor = delta_angle_rad > 0.0f ? 0 : 1;
        control_write_motor_alignment(&calibration.alignment);
        MotorControlConfig const config = control_read_motor_config();
        calibration.pole_pairs = config.motor_pole_pairs;
        if (fabsf(fabsf(delta_angle_rad) * calibration.pole_pairs - M_2PI) > 0.5f) {
            complete(CALIBRATION_POLE_MISMATCH);
            return;
        }
        excite(0.0f, CAL_ALIGN);
        return;
    }
    case CAL_ALIGN:
        if (elapsed_ms >= 1000U) enter_phase(CAL_CAPTURE_ALIGNMENT);
        return;
    case CAL_CAPTURE_ALIGNMENT: {
        if (!acquisition_serviced) return;
        float const reverse = calibration.alignment.invert_phase_motor == 0 ? 1.0f : -1.0f;
        float const offset_rad = normalize_angle(-positionSensor_getRadiansMultiturn() * calibration.pole_pairs * reverse);
        calibration.alignment.phase_offset_deg = (int16_t)(int)RADIANS_TO_DEGREES(offset_rad);
        control_write_motor_alignment(&calibration.alignment);
        API_FOC_Calibration_Idle();
        enter_phase(CAL_SAVE);
        return;
    }
    case CAL_SAVE:
        complete(Config_Save() ? CALIBRATION_SUCCESS : CALIBRATION_STORAGE_ERROR);
        return;
    case CAL_IDLE:
        return;
    }
}

bool Calibration_TakeResult(CalibrationResult *result)
{
    if (!calibration.result_pending || result == NULL) return false;
    *result = calibration.result;
    calibration.result_pending = false;
    return true;
}
