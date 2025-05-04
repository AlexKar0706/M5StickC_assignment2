#include "general.h"
#include "RecordModule.h"
#include "RtcModule.h"
#include "ScreensModule.h"
#include <SPIFFS.h>

RecordModule Rec;

double RecordModule::_GetAverage(int16_t* data, size_t data_size) 
{
    double sum = 0;

    for (size_t i = 0; i < data_size; i++) {
        sum += static_cast<double>(data[i]);
    }

    return sum / static_cast<double>(data_size);
}

double RecordModule::_GetNormalizedSquareSum(int16_t* data, size_t data_size, double average) 
{
    double norm_square_sum = 0;

    for (size_t i = 0; i < data_size; i++) {
        double norm = (static_cast<double>(data[i]) - average) / 32768.0;
        norm_square_sum += norm*norm; 
    }

    return norm_square_sum;
}

void RecordModule::_RecordAudio(void) 
{
    if (m_record_type == RecordType::Flash_raw || m_record_type == RecordType::Flash_timed) {
        if (difftime(Rtc.GetCurrentTime(), m_recording_start_time) > static_cast<double>(recording_length_sec)) {
            RecordModule::Stop();
            return;
        }

        if (difftime(Rtc.GetCurrentTime(), m_recording_current_time) != 0) {
            double rms = sqrt(m_rms_sum / static_cast<double>(m_total_samples_readings));
            m_spl_data[m_spl_data_size++] = static_cast<int16_t>(20 * log10(rms) + 94);
            m_recording_current_time = Rtc.GetCurrentTime();
            m_total_samples_readings = 0;
            m_rms_sum = 0;
        }
    }

    if (M5.Mic.isEnabled()) {
        if (M5.Mic.record(&m_record_data[m_record_index * record_length], record_length, record_sample_rate)) {
            int16_t* record_available_buffer = &m_record_data[m_available_index * record_length];
            m_total_samples_readings += record_length;

            if (m_record_type == RecordType::Flash_raw || m_record_type == RecordType::Flash_timed) {
                m_rms_sum += _GetNormalizedSquareSum(record_available_buffer, record_length, _GetAverage(record_available_buffer, record_length));
            } else {         

                uint8_t* data_buffer = reinterpret_cast<uint8_t*>(&record_available_buffer[0]);

                Serial.write(UART_RECORD_START_BYTE1);
                Serial.write(UART_RECORD_START_BYTE2);      
                Serial.write(data_buffer, record_message_size);
            }

            if (++m_available_index >= record_number) {
                m_available_index = 0;
            }
            if (++m_record_index >= record_number) {
                m_record_index = 0;
            }

        }

    }
}

void RecordModule::_SendRecord(void) 
{
    if (m_file_sending_state == SendState::Send_start) {
        Serial.println('s');
        String confirmation = Serial.readString();
        if (confirmation[0] == 's') {
            m_confirmation_retry = 3;
            m_file_sending_state = SendState::Send_transmit;
            Screens.RefreshScreen(true);
        }
    } else if (m_file_sending_state == SendState::Send_transmit) {

        char message_buffer[record_file_message_size];
        char* message_ptr =  &message_buffer[0];

        if (m_spl_data_size == m_samples_sent) {
            /* No data available */
            Serial.println('e');
            RecordModule::StopSending(true);
            return;
        }

        m_recording_current_time++;

        struct tm* samples_time_struct = localtime(&m_recording_current_time);
        message_ptr += strftime(message_ptr, 22, "%Y-%m-%d %T:", samples_time_struct);
        
        uint8_t sample_bytes[sizeof(int16_t)];
        m_mic_file.read(&sample_bytes[0], sizeof(int16_t));
        sprintf(message_ptr, " %d\r\n", *(reinterpret_cast<int16_t*>(&sample_bytes[0])));

        Serial.write(message_buffer, strlen(message_buffer));
        m_samples_sent++;
        m_file_sending_state = SendState::Send_confirm;

    } else if (m_file_sending_state == SendState::Send_confirm) {
        String confirmation = Serial.readString();
        if (confirmation[0] == 'k') {
            m_confirmation_retry = 3;
            m_file_sending_state = SendState::Send_transmit;
        } else {
            m_confirmation_retry--;
        }
        if (m_confirmation_retry == 0) RecordModule::StopSending(false);
    }
}

void RecordModule::Begin(void) 
{
    M5.Mic.begin();
    m_record_data = reinterpret_cast<int16_t*>(heap_caps_malloc(record_byte_size, MALLOC_CAP_8BIT));
    memset(m_record_data, '\0', record_byte_size);
    m_available_index = 0;
    m_record_index = 2;
    m_recording_state = RecordState::Record_inactive;

    Screens.SetScreen(Screen::Prepare_file_system);

    SPIFFS.begin();
#if (FIRST_START_UP)
    SPIFFS.format();
    m_mic_file = SPIFFS.open(file_name, "w");
    m_mic_file.close();
#endif
}

void RecordModule::Start(void) 
{
    memset(m_record_data, '\0', record_byte_size);
    m_available_index = 0;
    m_record_index = 2;
    m_total_samples_readings = 0;
    if (m_record_type == RecordType::Flash_raw || m_record_type == RecordType::Flash_timed) {
        Rtc.Update(true);
        m_rms_sum = 0;
        m_spl_data_size = 0;
        m_recording_current_time = m_recording_start_time = Rtc.GetCurrentTime();
    }
    m_recording_state = RecordState::Record_running;
}

void RecordModule::Stop(void)
{
    if (m_record_type == RecordType::Flash_raw || m_record_type == RecordType::Flash_timed) {
        
        if (m_spl_data_size > 0) {
            m_mic_file = SPIFFS.open(file_name, "w");

            uint8_t* time_buffer = reinterpret_cast<uint8_t*>(&m_recording_start_time);
            uint8_t* data_size_buffer = reinterpret_cast<uint8_t*>(&m_spl_data_size);
            uint8_t* data_buffer = reinterpret_cast<uint8_t*>(&m_spl_data[0]);
            m_mic_file.write(time_buffer, sizeof(time_t));
            m_mic_file.write(data_size_buffer, sizeof(size_t));
            m_mic_file.write(data_buffer, m_spl_data_size * sizeof(int16_t));

            m_mic_file.close();
        }

    }
    m_recording_state = RecordState::Record_stopped;
}

void RecordModule::Delete(void) 
{
    /* SPIFFS has hard time dealing with previousle opened files (long write time) */
    /* Better to format FS */
    SPIFFS.format();

    m_mic_file = SPIFFS.open(file_name, "w");
    m_mic_file.close();
}

void RecordModule::StartSchedule(void) 
{
    struct tm planned_record_time;
    uint32_t seconds_offset = 1; 

    Rtc.Update(true);
    planned_record_time = Rtc.GetCurrentDateTime();

    /* Schedule recording to the next minute, if less then 10 second remaining */
    if (planned_record_time.tm_sec > 50) {
        seconds_offset += 60;
    }

    /* Schedule recording at the last second of current minute */
    planned_record_time.tm_sec = 58;

    m_record_planned_time = mktime(&planned_record_time);
    m_record_planned_time += seconds_offset;
    m_recording_state = RecordState::Record_planned;
} 

void RecordModule::StopSchedule(void)
{
    m_recording_state = RecordState::Record_inactive;
}

bool RecordModule::GetRecordExists(void) 
{
    m_mic_file = SPIFFS.open(file_name, "r");
    bool record_exists = (m_mic_file.read() != -1) ? true : false;
    m_mic_file.close();
    return record_exists;
}

void RecordModule::StartSending(void) 
{
    m_file_sending_state = SendState::Send_start;
    m_samples_sent = 0;
    m_mic_file = SPIFFS.open(file_name, "r");
    uint8_t time_bytes[sizeof(time_t)];
    uint8_t size_bytes[sizeof(time_t)];
    if (m_mic_file.read(&time_bytes[0], sizeof(time_t)) != sizeof(time_t)) {
        /* Data corrupted */
        RecordModule::StopSending(false);
        return;
    }
    if (m_mic_file.read(&size_bytes[0], sizeof(size_t)) != sizeof(size_t)) {
        /* Data corrupted */
        RecordModule::StopSending(false);
        return;
    }

    m_recording_current_time = *(reinterpret_cast<time_t*>(&time_bytes));
    m_spl_data_size = *(reinterpret_cast<size_t*>(&size_bytes));

    if (m_spl_data_size == 0) {
        /* No samples available */
        RecordModule::StopSending(false);
        return;
    }
}

void RecordModule::StopSending(bool succesful_stop) 
{
    if (succesful_stop) m_file_sending_state = SendState::Send_finished;
    else m_file_sending_state = SendState::Send_interrupted;
    m_mic_file.close();
}

void RecordModule::Update(void) 
{
    if (m_recording_state == Record_planned) {
        if (RecordModule::GetTimeUntilStart() <= 0) {
            RecordModule::Start();
            Screens.SetScreen(Screen::Record_running);
        }
    }

    if (m_recording_state == Record_running) {
        _RecordAudio();
    } else if (m_recording_state == Record_stopped) {
        Screens.SetScreen(Screen::Record_successful);
        vTaskDelay(pdMS_TO_TICKS(2000));
        Screens.SetScreen(Screen::Start_record);
        m_recording_state = Record_inactive;
    }

    if (m_file_sending_state == SendState::Send_start || m_file_sending_state == SendState::Send_transmit || m_file_sending_state == SendState::Send_confirm) {
        _SendRecord();
    } else if (m_file_sending_state == SendState::Send_finished) {
        Screens.SetScreen(Screen::File_sending_successful);
        vTaskDelay(pdMS_TO_TICKS(2000));
        Screens.SetScreen(Screen::Send_file);
        m_file_sending_state = SendState::Send_inactive;
    } else if (m_file_sending_state == SendState::Send_interrupted) {
        Screens.SetScreen(Screen::File_sending_failed);
        vTaskDelay(pdMS_TO_TICKS(2000));
        Screens.SetScreen(Screen::Send_file);
        m_file_sending_state = SendState::Send_inactive;
    }
}
