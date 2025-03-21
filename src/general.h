#pragma once

#include <Arduino.h>
#include <M5Unified.h>

#define FIRST_START_UP (0)  // Formate file system, if first start-up
#define USE_NTP_TIME   (1)

typedef enum {
    RECORDING_INACTIVE,
    RECORDING_RUNNING,
    RECORDING_PLANNED,
    RECORDING_STOPPED,
} RecordingState_t;

typedef enum {
    RECORDING_UART,
    RECORDING_FLASH_RAW,
    RECORDING_FLASH_TIMED,
} RecordingType_t;

typedef enum {
    FILE_SENDING_INACTIVE,
    FILE_SENDING_START,
    FILE_SENDING_TRANSMIT,
    FILE_SENDING_CONFIRM,
    FILE_SENDING_INTERRUPTED,
    FILE_SENDING_FINISHED,
} FileSendingState_t;

constexpr const size_t record_number     = 4;
constexpr const size_t record_length     = 256;
constexpr const size_t record_sample_rate = 10000;
constexpr const size_t record_size       = record_number * record_length;
constexpr const size_t record_byte_length = record_length * sizeof(int16_t);
constexpr const size_t record_byte_size = record_number * record_byte_length;
constexpr const size_t recording_length_sec = 30;
constexpr const size_t recording_samples_max = recording_length_sec * record_sample_rate;
constexpr const size_t record_message_length = 8;  // '-' sign + 5 digits (int16_t) + CR + LF
constexpr const size_t record_message_size = record_message_length * record_length;
constexpr const size_t record_file_message_length = 7;  // '-' sign + 5 digits (int16_t) + ',' sign
constexpr const size_t record_file_message_size = 20 + record_file_message_length * record_length + 2;  // YYYY-MM-DD hh:mm:ss: (20 signs) + samples + CR + LF

extern struct tm rtc_timestamp;
extern volatile FileSendingState_t file_sending_state;
extern volatile RecordingState_t recording_state;
extern volatile RecordingType_t record_type;
extern time_t record_planned_time;
extern time_t recording_start_time;
extern volatile bool record_exists;
extern size_t samples_sent;
