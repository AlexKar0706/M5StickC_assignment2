#include "general.h"

struct tm rtc_timestamp;
volatile FileSendingState_t file_sending_state;
volatile RecordingState_t recording_state;
volatile RecordingType_t record_type = RECORDING_FLASH_TIMED;
time_t record_planned_time;
time_t recording_start_time;
volatile bool record_exists;
size_t samples_sent;
