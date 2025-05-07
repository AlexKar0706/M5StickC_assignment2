#pragma once

#include "general.h"
#include "RtcModule.h"
#include <SPIFFS.h>

#define UART_RECORD_START_BYTE1 0xAA
#define UART_RECORD_START_BYTE2 0xFF

constexpr const size_t record_number     = 4;
constexpr const size_t record_length     = 256;
constexpr const size_t record_sample_rate = 10000;
constexpr const size_t record_size       = record_number * record_length;
constexpr const size_t record_byte_length = record_length * sizeof(int16_t);
constexpr const size_t record_byte_size = record_number * record_byte_length;
constexpr const size_t recording_length_sec = 60 * 10 ;
constexpr const size_t recording_samples_max = recording_length_sec;
constexpr const size_t record_message_length = sizeof(int16_t);
constexpr const size_t record_message_size = record_message_length * record_length;
constexpr const size_t record_file_message_length = 14;  // ',' sign + '-' sign + 6 digits (double exponent) + 6 digits (double fraction)
constexpr const size_t record_file_message_size = 20 + 1 + record_file_message_length + 2 + 8;  // YYYY-MM-DD hh:mm:ss: (20 signs) + sample + CR + LF + 8 bytes reserv
constexpr const char file_name[] = "/M5StickC/mic.bin";

class RecordModule {

public:

    enum RecordState {
        Record_inactive,
        Record_running,
        Record_planned,
        Record_stopped,
    };

    enum RecordType {
        Uart,
        Flash_raw,
        Flash_timed,
    };

    enum SendState {
        Send_inactive,
        Send_start,
        Send_transmit,
        Send_confirm,
        Send_interrupted,
        Send_finished,
    };

private:

    size_t m_available_index  = 0;
    size_t m_record_index  = 2;
    size_t m_total_samples_readings = 0;
    int16_t *m_record_data;
    double m_spl_data[recording_length_sec + 1];
    size_t m_spl_data_size = 0;
    double m_rms_sum;
    File m_mic_file;
    time_t m_record_planned_time;
    time_t m_recording_start_time;
    time_t m_recording_current_time;
    size_t m_samples_sent;
    size_t m_confirmation_retry = 3;
    volatile SendState m_file_sending_state;
    volatile RecordState m_recording_state;
    volatile RecordType m_record_type = Flash_timed;

    double _GetAverage(int16_t* data, size_t data_size);
    double _GetNormalizedSquareSum(int16_t* data, size_t data_size, double average);
    void _RecordAudio(void);
    void _SendRecord(void);
public:
    RecordModule() {};
    void Begin(void);
    void Start(void);
    void Stop(void);
    void StartSchedule(void);
    void StopSchedule(void);
    void Delete(void);
    void Update(void);
    void StartSending(void);
    void StopSending(bool succesful_stop);
    bool GetRecordExists(void);
    SendState GetSendState(void) { return m_file_sending_state; };
    RecordState GetRecState(void) { return m_recording_state; };
    RecordType GetRecType(void) { return m_record_type; };
    size_t GetSamplesSent(void) { return m_samples_sent; };
    int GetTimeUntilStart(void) { return static_cast<int>(difftime(m_record_planned_time, Rtc.GetCurrentTime())); };
    int GetRecTimeLapsed(void) { return static_cast<int>(difftime(Rtc.GetCurrentTime(), m_recording_start_time)); };

    void SetRecType(RecordType new_type) { m_record_type = new_type; };

    ~RecordModule() {};
};

extern RecordModule Rec;
