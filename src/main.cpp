#include "general.h"
#include "screens.h"
#include <SPIFFS.h>
#if (USE_NTP_TIME)
#include "secret_example.h"
#if (!USE_SECRET_EXAMPLE)
#include "secret.h"
#endif
#include <WiFi.h>
#include <esp_sntp.h>
#endif

const long gmtOffset_sec     = 0;
const int daylightOffset_sec = 3600;

static size_t available_index  = 0;
static size_t record_index  = 2;
static size_t total_record_readings = 0;
static int16_t *record_data;

static const String file_name = "/M5StickC/mic.bin";
static File mic_file;
static size_t confirmation_retry = 3;

static time_t current_time;
static bool rtc_get_time;

TimerHandle_t xtimer_mic;
TimerHandle_t xtimer_rtc;

void RtcTimerCallback(TimerHandle_t xTimer) 
{
    rtc_get_time = true;
}

void StartRTC(void) 
{
    struct tm tm;
#if (USE_NTP_TIME)
    /* Get time from NTP server */
    uint32_t retry_attempts = 3;
    while (retry_attempts--) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        uint8_t wifi_sync_max = 50;
        while (WiFi.status() != WL_CONNECTED && wifi_sync_max != 0) {
            vTaskDelay(pdMS_TO_TICKS((100)));
            wifi_sync_max--;
        }
        if (wifi_sync_max == 0) {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            continue;
        }
        configTime(gmtOffset_sec, daylightOffset_sec, WIFI_NTP_SERVER);
        sntp_sync_status_t sync_status = sntp_get_sync_status();
        uint8_t status_sync_max = 50;
        while (sync_status != SNTP_SYNC_STATUS_COMPLETED && status_sync_max != 0) {
            sync_status = sntp_get_sync_status();
            vTaskDelay(pdMS_TO_TICKS((100)));
            status_sync_max--;
        }
        sntp_stop();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        if (status_sync_max == 0) continue;
        break;
    }
    getLocalTime(&tm);
#else 
    /* Save compile timestamp as RTC start point */
    strptime(__TIMESTAMP__, "%a %b %d %H:%M:%S %Y", &tm);
#endif

    M5.Rtc.begin();
    M5.Rtc.setDateTime(tm);
    rtc_timestamp = M5.Rtc.getDateTime().get_tm();
    rtc_get_time = false;
    xtimer_rtc = xTimerCreate("RtcTimerCallback", pdMS_TO_TICKS(50), pdTRUE, (void *)0, RtcTimerCallback);
    xTimerStart(xtimer_rtc, 0);
}

void CheckTimeRTC(void) 
{
    if (rtc_get_time) {
        rtc_timestamp = M5.Rtc.getDateTime().get_tm();
        current_time = mktime(&rtc_timestamp);
        rtc_get_time = false;
        if (active_screen == SCREEN_RTC_TIME) RefreshScreen(false);
    }
}

void InitRecording(void) 
{
    M5.Mic.begin();
    record_data = reinterpret_cast<int16_t*>(heap_caps_malloc(record_byte_size, MALLOC_CAP_8BIT));
    memset(record_data, '\0', record_byte_size);
    available_index = 0;
    record_index = 2;
    recording_state = RECORDING_INACTIVE;
}

void DeleteRecordingFile(void) 
{
    /* SPIFFS has hard time dealing with previousle opened files (long write time) */
    /* Better to format FS */
    SPIFFS.format();

    mic_file = SPIFFS.open(file_name, "w");
    mic_file.close();
}

void StartRecording(void) 
{
    memset(record_data, '\0', record_byte_size);
    available_index = 0;
    record_index = 2;
    total_record_readings = 0;
    if (record_type == RECORDING_FLASH_RAW || record_type == RECORDING_FLASH_TIMED) {
        mic_file = SPIFFS.open(file_name, "w");
        rtc_timestamp = M5.Rtc.getDateTime().get_tm();
        recording_start_time = mktime(&rtc_timestamp);
    }
    recording_state = RECORDING_RUNNING;
}

void StopRecording(void)
{
    if (record_type == RECORDING_FLASH_RAW || record_type == RECORDING_FLASH_TIMED) {
        mic_file.close();
    }
    recording_state = RECORDING_STOPPED;
}

void ScheduleRecording(void) 
{
    struct tm planned_record_time;
    uint32_t seconds_offset = 1; 

    planned_record_time = rtc_timestamp = M5.Rtc.getDateTime().get_tm();

    /* Schedule recording to the next minute, if less then 10 second remaining */
    if (planned_record_time.tm_sec > 50) {
        seconds_offset += 60;
    }

    /* Schedule recording at the start of new minute */
    planned_record_time.tm_sec = 59;

    record_planned_time = mktime(&planned_record_time);
    record_planned_time += seconds_offset;
    recording_state = RECORDING_PLANNED;
} 

void StopScheduleRecording(void)
{
    recording_state = RECORDING_INACTIVE;
}

void RecordAudio(void) 
{
    if (record_type == RECORDING_FLASH_RAW || record_type == RECORDING_FLASH_TIMED) {
        if (difftime(current_time, recording_start_time) >= static_cast<double>(recording_length_sec)) {
            StopRecording();
            return;
        }
    }

    if (M5.Mic.isEnabled()) {
        if (M5.Mic.record(&record_data[record_index * record_length], record_length, record_sample_rate)) {
            int16_t* record_available_buffer = &record_data[available_index * record_length];

            int16_t block_avg = 0;
            for (size_t i = 0; i < record_length; i++) {
                block_avg += record_available_buffer[i];
            }
            block_avg /= record_length;

            for (size_t i = 0; i < record_length; i++) {
               record_available_buffer[i] -= block_avg;
            }

            if (total_record_readings >= 2) {
                if (record_type == RECORDING_FLASH_RAW || record_type == RECORDING_FLASH_TIMED) {

                    uint8_t* data_buffer = reinterpret_cast<uint8_t*>(&record_available_buffer[0]);

                    uint8_t* time_buffer = reinterpret_cast<uint8_t*>(&current_time);
                    mic_file.write(time_buffer, sizeof(time_t));
                    mic_file.write(data_buffer, record_byte_length);
                } else {         
                    char full_message[record_message_size + 1];
                    char* ptr_message = &full_message[0];
                    for (size_t i = 0; i < record_length; i++) {
                        ptr_message += sprintf(ptr_message, "%d\r\n", record_available_buffer[i]);
                    }
                    *ptr_message = '\0';         
                    Serial.write(full_message, strlen(full_message));
                }
            }

            if (++available_index >= record_number) {
                available_index = 0;
            }
            if (++record_index >= record_number) {
                record_index = 0;
            }

            total_record_readings++;

        }

    }
}

void CheckRecording(void) 
{
    if (recording_state == RECORDING_PLANNED) {
        if (difftime(record_planned_time, current_time) <= 0) {
            StartRecording();
            ChangeScreen(SCREEN_RECORD_RUNNING);
        }
    }

    if (recording_state == RECORDING_RUNNING) {
        RecordAudio();
    } else if (recording_state == RECORDING_STOPPED) {
        ChangeScreen(SCREEN_RECORD_SUCCESSFUL);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ChangeScreen(SCREEN_START_RECORD);
        recording_state = RECORDING_INACTIVE;
    }
}

void StartFileSystem(void) 
{
    SPIFFS.begin();
#if (FIRST_START_UP)
    SPIFFS.format();
    mic_file = SPIFFS.open(file_name, "w");
    mic_file.close();
#endif
}

bool IsRecordExists(void) 
{
    mic_file = SPIFFS.open(file_name, "r");
    bool record_exists = (mic_file.read() != -1) ? true : false;
    mic_file.close();
    return record_exists;
}

void StartSendingRecord(void) 
{
    file_sending_state = FILE_SENDING_START;
    samples_sent = 0;
    mic_file = SPIFFS.open(file_name, "r");
}

void StopSendingRecord(bool succesful_stop) 
{
    if (succesful_stop) file_sending_state = FILE_SENDING_FINISHED;
    else file_sending_state = FILE_SENDING_INTERRUPTED;
    mic_file.close();
}

void SendRecord(void) 
{
    if (file_sending_state == FILE_SENDING_START) {
        Serial.println('s');
        String confirmation = Serial.readString();
        if (confirmation[0] == 's') {
            confirmation_retry = 3;
            file_sending_state = FILE_SENDING_TRANSMIT;
            RefreshScreen(true);
        }
    } else if (file_sending_state == FILE_SENDING_TRANSMIT) {

        char message_buffer[record_file_message_size + 1];
        char* message_ptr = &message_buffer[0];

        uint8_t time_bytes[sizeof(time_t)];
        if (mic_file.read(&time_bytes[0], sizeof(time_t)) != sizeof(time_t)) {
            /* No data available */
            Serial.println('e');
            StopSendingRecord(true);
            return;
        }

        time_t samples_time = *(reinterpret_cast<time_t*>(&time_bytes));
        struct tm* samples_time_struct = localtime(&samples_time);
        message_ptr += strftime(message_ptr, 22, "%Y-%m-%d %T:", samples_time_struct);

        for (size_t i = 0; i < record_length; i++) {
            uint8_t sample_bytes[sizeof(int16_t)];
            mic_file.read(&sample_bytes[0], sizeof(int16_t));
            message_ptr += sprintf(message_ptr, "%d,", *(reinterpret_cast<int16_t*>(&sample_bytes[0])));
        }
        samples_sent += record_length;

        /* Remove last ',' sign */
        message_ptr--;

        sprintf(message_ptr, "\r\n");
        Serial.write(message_buffer, strlen(message_buffer));
        file_sending_state = FILE_SENDING_CONFIRM;

    } else if (file_sending_state == FILE_SENDING_CONFIRM) {
        String confirmation = Serial.readString();
        if (confirmation[0] == 'k') {
            confirmation_retry = 3;
            file_sending_state = FILE_SENDING_TRANSMIT;
        } else {
            confirmation_retry--;
        }
        if (confirmation_retry == 0) StopSendingRecord(false);
    }
}

void CheckRecordSend(void) 
{
    if (file_sending_state == FILE_SENDING_START || file_sending_state == FILE_SENDING_TRANSMIT || file_sending_state == FILE_SENDING_CONFIRM) {
        SendRecord();
    } else if (file_sending_state == FILE_SENDING_FINISHED) {
        ChangeScreen(SCREEN_FILE_SENDING_SUCCESSFUL);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ChangeScreen(SCREEN_SEND_FILE);
        file_sending_state = FILE_SENDING_INACTIVE;
    } else if (file_sending_state == FILE_SENDING_INTERRUPTED) {
        ChangeScreen(SCREEN_FILE_SENDING_FAILED);
        vTaskDelay(pdMS_TO_TICKS(2000));
        ChangeScreen(SCREEN_SEND_FILE);
        file_sending_state = FILE_SENDING_INACTIVE;
    }
}

void CheckButtons(void) 
{
    if (M5.BtnA.wasClicked()) {

        switch (active_screen) {

        case SCREEN_RTC_TIME:
            if (recording_state == RECORDING_RUNNING) {
                ChangeScreen(SCREEN_RECORD_RUNNING);
            } else if (recording_state == RECORDING_PLANNED) {
                ChangeScreen(SCREEN_PLANNED_RECORD);
            } else if (file_sending_state != FILE_SENDING_INACTIVE) {
                ChangeScreen(SCREEN_FILE_SENDING);
            } else {
                ChangeScreen(SCREEN_START_RECORD);
            }
            break;

        case SCREEN_START_RECORD:
        case SCREEN_RECORD_WILL_BE_LOST:
        case SCREEN_RECORD_RUNNING:
        case SCREEN_PLANNED_RECORD:
            if (recording_state == RECORDING_RUNNING || recording_state == RECORDING_PLANNED) {
                ChangeScreen(SCREEN_RTC_TIME);
            } else {
                record_exists = IsRecordExists();
                if (record_exists) ChangeScreen(SCREEN_SEND_FILE);
                else ChangeScreen(SCREEN_CHOOSE_RECORD_MODE);
            }
            break;

        case SCREEN_SEND_FILE:
        case SCREEN_FILE_SENDING:
            ChangeScreen(SCREEN_CHOOSE_RECORD_MODE);
            break;

        case SCREEN_CHOOSE_RECORD_MODE:
            ChangeScreen(SCREEN_RTC_TIME);
            break;

        default: break;

        }

    } else if (M5.BtnB.wasClicked()) {

        switch (active_screen) {

        case SCREEN_START_RECORD:
            record_exists = IsRecordExists();
            if (record_exists && (record_type == RECORDING_FLASH_RAW || record_type == RECORDING_FLASH_TIMED)) {
                ChangeScreen(SCREEN_RECORD_WILL_BE_LOST);
            } else if (record_type == RECORDING_FLASH_TIMED) {
                ScheduleRecording();
                ChangeScreen(SCREEN_PLANNED_RECORD);
            } else {
                StartRecording();
                ChangeScreen(SCREEN_RECORD_RUNNING);
            }
            break;

        case SCREEN_RECORD_WILL_BE_LOST:
            ChangeScreen(SCREEN_DELETE_RECORD);
            DeleteRecordingFile();
            ChangeScreen(SCREEN_START_RECORD);
            break;

        case SCREEN_PLANNED_RECORD:
            StopScheduleRecording();
            ChangeScreen(SCREEN_START_RECORD);
            break;

        case SCREEN_RECORD_RUNNING:
            StopRecording();
            break;

        case SCREEN_CHOOSE_RECORD_MODE:
            if (recording_state != RECORDING_RUNNING && recording_state != RECORDING_PLANNED) {
                switch (record_type) {
                case RECORDING_UART:        record_type = RECORDING_FLASH_RAW; break;
                case RECORDING_FLASH_RAW:   record_type = RECORDING_FLASH_TIMED; break;
                case RECORDING_FLASH_TIMED: record_type = RECORDING_UART; break;
                }
                RefreshScreen(false);
            }
            break;

        case SCREEN_SEND_FILE:
        case SCREEN_FILE_SENDING:
            if (file_sending_state == FILE_SENDING_INACTIVE) {
                StartSendingRecord();
                ChangeScreen(SCREEN_FILE_SENDING);
            } else if (file_sending_state == FILE_SENDING_START) {
                StopSendingRecord(false);
            }
            break;

        default: break;

        }

    }
}

void setup() 
{
    M5.begin();
    M5.update();
    /* Set UART buffer enough to send async data */
    Serial.begin(500000);
    Serial.setTxBufferSize(record_message_size + 64);
    Serial.setTimeout(10);

    ScreensInit();
    ChangeScreen(SCREEN_STARTUP);

    StartRTC();
    InitRecording();

    ChangeScreen(SCREEN_PREPARE_FILE_SYSTEM);
    StartFileSystem();
    ChangeScreen(SCREEN_RTC_TIME);
}

void loop()
{
    bool use_delay = true;
    M5.update();

    /* Check current rtc time */
    CheckTimeRTC();

    /* Check recording states */
    CheckRecording();
    if (recording_state == RECORDING_RUNNING) use_delay = false;

    /* Check record sending states */
    CheckRecordSend();
    if (file_sending_state == FILE_SENDING_START || file_sending_state == FILE_SENDING_TRANSMIT || file_sending_state == FILE_SENDING_CONFIRM) use_delay = false;

    /* Check buttons presses */
    CheckButtons();

    if (use_delay) vTaskDelay(pdMS_TO_TICKS(10));
}