#ifndef INC_CALIBRATION_H_
#define INC_CALIBRATION_H_

#include <stdbool.h>

typedef enum {
    CALIBRATION_SUCCESS = 0,
    CALIBRATION_NO_MOTION = 1,
    CALIBRATION_POLE_MISMATCH = 2,
    CALIBRATION_CANCELLED = 3,
    CALIBRATION_STORAGE_ERROR = 4
} CalibrationResult;

// One foreground-owned job; initialize once at startup. No automatic motion.
void Calibration_Init(void);
// Called through the drive supervisor. FOC repeats the atomic entry guard.
bool Calibration_Start(void);
// One bounded state transition per pass; no acquisition, service or wait loops.
// acquisition_serviced identifies this pass's normal encoder acquisition; angle
// captures require a subsequent acquisition after each settling deadline.
// Final flash saving remains synchronous, with the motor coasting.
void Calibration_Update(bool acquisition_serviced);
// Consume the completion once. An unread result prevents another start.
bool Calibration_TakeResult(CalibrationResult *result);

#endif
