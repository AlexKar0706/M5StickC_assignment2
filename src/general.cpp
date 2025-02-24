#include "general.h"

struct tm rtc_timestamp;
volatile FileSendingState_t file_sending_state;
volatile RecordingState_t recording_state;
volatile bool record_in_flash = true;
time_t recording_start_time;
volatile bool record_exists;
size_t samples_sent;
