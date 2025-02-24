#include "general.h"
#include "screens.h"

Screens_t active_screen;

static TimerHandle_t xtimer_display;
static TaskHandle_t xhandle_display = NULL;
static bool recording_circle_active = false;
static bool sending_arrow_active = false;
static volatile SemaphoreHandle_t display_binary_semaphore  = NULL;
static volatile bool screen_transition;

static void DisplayTimer(TimerHandle_t xTimer) ;
static void DisplayTask(void* arg);

void ScreensInit(void) 
{
    M5.Display.begin();
    display_binary_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(DisplayTask, "DisplayTask", 1024 * 2, (void *)0, 4,
                &xhandle_display);
}

void ChangeScreen(Screens_t new_screen) 
{
    if (active_screen != new_screen) {
        screen_transition = true;
        active_screen = new_screen;
    }
    xSemaphoreGive(display_binary_semaphore);
}

void RefreshScreen(bool full_redraw) 
{
    screen_transition = full_redraw;
    xSemaphoreGive(display_binary_semaphore);
}

static void ScreenStartUp(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(32, 15);
    M5.Display.print("ITS8055");
    M5.Display.setCursor(0, 45);
    M5.Display.print("Assignment 2");
}

static void ScreenPrepareFileSystem(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(24, 15);
    M5.Display.print("Preparing");
    M5.Display.setCursor(16, 45);
    M5.Display.print("File System");
}

static void ScreenCurrentTime(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(25, 15);
    M5.Display.print("RTC time:");
    M5.Display.setCursor(30, 45);
    M5.Display.printf("%02d:%02d:%02d", rtc_timestamp.tm_hour, rtc_timestamp.tm_min, rtc_timestamp.tm_sec);
}

static void ScreenStartRecording(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(55, 15);
    M5.Display.print("Start");
    M5.Display.setCursor(41, 45);
    M5.Display.print("Record");
}

static void ScreenRecordWillBeLost(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(20, 15);
    M5.Display.print("Record will");
    M5.Display.setCursor(45, 45);
    M5.Display.print("be lost");
}

static void ScreenDeleteRecord(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(34, 15);
    M5.Display.print("Deleting");
    M5.Display.setCursor(41, 45);
    M5.Display.print("Record");
}

static void ScreenRecordRunning(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(0, 15);
    if (record_in_flash) {
        M5.Display.setCursor(29, 15);
        M5.Display.print("Flash rec");
        M5.Display.setTextFont(2);
        M5.Display.setCursor(43, 45);
        double time_lapsed = difftime(mktime(&rtc_timestamp), recording_start_time);
        double progress = time_lapsed / static_cast<double>(recording_length_sec) * 100;
        M5.Display.printf("%3d/%3d sec", static_cast<int>(time_lapsed), recording_length_sec);
        M5.Display.progressBar(10, 65, 140, 10, static_cast<uint8_t>(progress));
    } else {
        M5.Display.setCursor(28, 15);
        M5.Display.print("UART rec");
    }
}

static void ScreenRecordSuccessful(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);

    M5.Display.setCursor(23, 15);
    M5.Display.print("Recording");
    M5.Display.setCursor(20, 45);
    M5.Display.print("Successful");
}

static void ScreenChooseRecordMode(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(0, 15);
    M5.Display.print("Record mode:");
    if (record_in_flash) {
        M5.Display.setCursor(50, 45);
        M5.Display.print("Flash ");
    } else {
        M5.Display.setCursor(50, 45);
        M5.Display.print("UART");
    }
}

static void ScreenSendFile(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(5, 15);
    M5.Display.println("Start sending");
    M5.Display.setCursor(41, 45);
    M5.Display.println("Record");
}

static void ScreenFileSending(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);

    if (file_sending_state == FILE_SENDING_START) {
        M5.Display.setCursor(38, 15);
        M5.Display.print("Waiting");
        M5.Display.setCursor(43, 45);
        M5.Display.print("Device");
    } else {
        M5.Display.setCursor(0, 15);
        M5.Display.print("Samples sent:");
        M5.Display.setTextFont(2);
        M5.Display.setCursor(22, 45);
        size_t samples_sent_print = (samples_sent <= recording_samples_max) ? samples_sent : recording_samples_max;
        double progress = samples_sent_print / static_cast<double>(recording_samples_max) * 100;
        M5.Display.printf("%7d/%7d", samples_sent_print, recording_samples_max);
        M5.Display.progressBar(10, 65, 140, 10, static_cast<uint8_t>(progress));
    }
}

static void ScreenFileSendingSuccessful(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(7, 15);
    M5.Display.print("Rec was sent");
    M5.Display.setCursor(9, 45);
    M5.Display.print("Successfully");
}

static void ScreenFileSendingFailed(void) 
{
    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(4);
    M5.Display.setCursor(35, 15);
    M5.Display.println("Sending");
    M5.Display.setCursor(48, 45);
    M5.Display.println("Failed");
}

static void IndicateRecording(void) 
{
    if (recording_state == RECORDING_RUNNING) {
        if (recording_circle_active) {
            M5.Display.fillCircle(5, 5, 3, M5.Display.color565(255, 0, 0));
        } else {
            M5.Display.fillCircle(5, 5, 3, M5.Display.color565(0, 0, 0));
        }
    }
}

static void IndicateSending(void) 
{
    if (file_sending_state == FILE_SENDING_START || file_sending_state == FILE_SENDING_TRANSMIT || file_sending_state == FILE_SENDING_CONFIRM) {
        M5.Display.setRotation(3);
        M5.Display.setTextSize(1);
        M5.Display.setTextFont(2);
        M5.Display.setCursor(0, 0);
        lgfx::TextStyle previouse_style = M5.Display.getTextStyle();
        if (sending_arrow_active) {
            M5.Display.setTextColor(M5.Display.color565(255, 255, 255));
        } else {
            M5.Display.setTextColor(M5.Display.color565(0, 0, 0));
        }
        M5.Display.print("->");
        M5.Display.setTextStyle(previouse_style);
    }
}

static void IndicateScreenType(void) 
{
    char screen_type_character = '\0';

    switch (active_screen) {

        case SCREEN_RTC_TIME:
            screen_type_character = 'T';
            break;

        case SCREEN_RECORD_WILL_BE_LOST:
        case SCREEN_DELETE_RECORD:
        case SCREEN_START_RECORD:
        case SCREEN_RECORD_RUNNING:
        case SCREEN_RECORD_SUCCESSFUL:
            screen_type_character = 'R';
            break;

        case SCREEN_CHOOSE_RECORD_MODE:
            screen_type_character = 'M';
            break;

        case SCREEN_SEND_FILE:
        case SCREEN_FILE_SENDING:
        case SCREEN_FILE_SENDING_SUCCESSFUL:
        case SCREEN_FILE_SENDING_FAILED:
            screen_type_character = 'F';
            break;

        default: return;

    }

    M5.Display.setRotation(3);
    M5.Display.setTextSize(1);
    M5.Display.setTextFont(2);
    M5.Display.setCursor(150, 0);
    M5.Display.print(screen_type_character);
}

static void DisplayTimer(TimerHandle_t xTimer) 
{
    static size_t ms10 = 0;

    if ((ms10 % 50) == 0) {
        recording_circle_active ^= 1;
        sending_arrow_active ^= 1;
        
        if (recording_state == RECORDING_RUNNING || file_sending_state == FILE_SENDING_START || file_sending_state == FILE_SENDING_TRANSMIT || file_sending_state == FILE_SENDING_CONFIRM) {
            RefreshScreen(false);
        }
    }

    if ((ms10 % 100) == 0) {
        if (active_screen == SCREEN_FILE_SENDING) {
            RefreshScreen(false);
        }
    }

    if (ms10 >= 1000) ms10 = 0;
    ms10++;
}

static void DisplayTask(void* arg) 
{
    xtimer_display = xTimerCreate("DisplayTimer", pdMS_TO_TICKS(10), pdTRUE, (void *)0, DisplayTimer);
    xTimerStart(xtimer_display, 0);
    while (1) {       
        if (screen_transition) {
            M5.Display.clear();
            screen_transition = false;
        }
        switch (active_screen) {

            case SCREEN_STARTUP:
                ScreenStartUp();
                break;

            case SCREEN_PREPARE_FILE_SYSTEM:
                ScreenPrepareFileSystem();
                break;

            case SCREEN_RTC_TIME:
                ScreenCurrentTime();
                break;

            case SCREEN_START_RECORD:
                ScreenStartRecording();
                break;

            case SCREEN_RECORD_WILL_BE_LOST:
                ScreenRecordWillBeLost();
                break;

            case SCREEN_DELETE_RECORD:
                ScreenDeleteRecord();
                break;

            case SCREEN_RECORD_RUNNING:
                ScreenRecordRunning();
                break;

            case SCREEN_RECORD_SUCCESSFUL:
                ScreenRecordSuccessful();
                break;

            case SCREEN_CHOOSE_RECORD_MODE:
                ScreenChooseRecordMode();
                break;
            
            case SCREEN_SEND_FILE:
                ScreenSendFile();
                break;
        
            case SCREEN_FILE_SENDING:
                ScreenFileSending();
                break;

            case SCREEN_FILE_SENDING_SUCCESSFUL:
                ScreenFileSendingSuccessful();
                break;

            case SCREEN_FILE_SENDING_FAILED:
                ScreenFileSendingFailed();
                break;

            default: break;
        
        }
        IndicateRecording();
        IndicateSending();
        IndicateScreenType();
        xSemaphoreTake(display_binary_semaphore, portMAX_DELAY);
    }
}
