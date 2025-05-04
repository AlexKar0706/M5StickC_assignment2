#include "general.h"
#include "ScreensModule.h"
#include "RecordModule.h"
#include "RtcModule.h"

ScreensModule Screens;

void ScreensModule::_ScreenStartUp(void) 
{
    M5.Display.setCursor(32, 15);
    M5.Display.print("ITS8055");
    M5.Display.setCursor(0, 45);
    M5.Display.print("Assignment F");
}

void ScreensModule::_ScreenPrepareFileSystem(void) 
{
    M5.Display.setCursor(24, 15);
    M5.Display.print("Preparing");
    M5.Display.setCursor(16, 45);
    M5.Display.print("File System");
}

void ScreensModule::_ScreenCurrentTime(void) 
{
    M5.Display.setCursor(25, 15);
    M5.Display.print("RTC time:");
    M5.Display.setCursor(30, 45);
    M5.Display.printf("%02d:%02d:%02d", Rtc.GetCurrentDateTime().tm_hour, Rtc.GetCurrentDateTime().tm_min, Rtc.GetCurrentDateTime().tm_sec);
}

void ScreensModule::_ScreenStartRecording(void) 
{
    M5.Display.setCursor(55, 15);
    M5.Display.print("Start");
    M5.Display.setCursor(41, 45);
    M5.Display.print("Record");
}

void ScreensModule::_ScreenPlannedRecording(void) 
{
    M5.Display.setCursor(27, 15);
    M5.Display.print("Record in:");
    M5.Display.setCursor(35, 45);
    M5.Display.printf("  %3d sec  ", Rec.GetTimeUntilStart());
}

void ScreensModule::_ScreenRecordWillBeLost(void) 
{
    M5.Display.setCursor(20, 15);
    M5.Display.print("Record will");
    M5.Display.setCursor(45, 45);
    M5.Display.print("be lost");
}

void ScreensModule::_ScreenDeleteRecord(void) 
{
    M5.Display.setCursor(34, 15);
    M5.Display.print("Deleting");
    M5.Display.setCursor(41, 45);
    M5.Display.print("Record");
}

void ScreensModule::_ScreenRecordRunning(void) 
{
    M5.Display.setCursor(0, 15);
    if (Rec.GetRecType() == RecordModule::RecordType::Flash_raw || Rec.GetRecType() == RecordModule::RecordType::Flash_timed) {
        M5.Display.setCursor(29, 15);
        M5.Display.print("Flash rec");
        M5.Display.setTextFont(2);
        M5.Display.setCursor(43, 45);
        int time_lapsed = Rec.GetRecTimeLapsed();
        uint8_t progress = time_lapsed * 100 / recording_length_sec;
        M5.Display.printf("%3d/%3d sec", time_lapsed, recording_length_sec);
        M5.Display.progressBar(10, 65, 140, 10, static_cast<uint8_t>(progress));
    } else {
        M5.Display.setCursor(28, 15);
        M5.Display.print("UART rec");
    }
}

void ScreensModule::_ScreenRecordSuccessful(void) 
{
    M5.Display.setCursor(23, 15);
    M5.Display.print("Recording");
    M5.Display.setCursor(20, 45);
    M5.Display.print("Successful");
}

void ScreensModule::_ScreenChooseRecordMode(void) 
{
    M5.Display.setCursor(0, 15);
    M5.Display.print("Record mode:");
    M5.Display.fillRect(0, 45, M5.Display.width(), M5.Display.height() - 45);
    if (Rec.GetRecType() == RecordModule::RecordType::Flash_timed) {
        M5.Display.setCursor(10, 45);
        M5.Display.print("Timed Flash");
    } else if (Rec.GetRecType() == RecordModule::RecordType::Flash_raw) {
        M5.Display.setCursor(50, 45);
        M5.Display.print("Flash ");
    } else {
        M5.Display.setCursor(50, 45);
        M5.Display.print("UART");
    }
}

void ScreensModule::_ScreenSendFile(void) 
{
    M5.Display.setCursor(5, 15);
    M5.Display.println("Start sending");
    M5.Display.setCursor(41, 45);
    M5.Display.println("Record");
}

void ScreensModule::_ScreenFileSending(void) 
{
    if (Rec.GetSendState() == RecordModule::SendState::Send_start) {
        M5.Display.setCursor(38, 15);
        M5.Display.print("Waiting");
        M5.Display.setCursor(43, 45);
        M5.Display.print("Device");
    } else {
        M5.Display.setCursor(0, 15);
        M5.Display.print("Samples sent:");
        M5.Display.setTextFont(2);
        M5.Display.setCursor(22, 45);
        size_t samples_sent_print = (Rec.GetSamplesSent() <= recording_samples_max) ? Rec.GetSamplesSent() : recording_samples_max;
        double progress = samples_sent_print / static_cast<double>(recording_samples_max) * 100;
        M5.Display.printf("%7d/%7d", samples_sent_print, recording_samples_max);
        M5.Display.progressBar(10, 65, 140, 10, static_cast<uint8_t>(progress));
    }
}

void ScreensModule::_ScreenFileSendingSuccessful(void) 
{
    M5.Display.setCursor(7, 15);
    M5.Display.print("Rec was sent");
    M5.Display.setCursor(9, 45);
    M5.Display.print("Successfully");
}

void ScreensModule::_ScreenFileSendingFailed(void) 
{
    M5.Display.setCursor(35, 15);
    M5.Display.println("Sending");
    M5.Display.setCursor(48, 45);
    M5.Display.println("Failed");
}

void ScreensModule::_IndicateRecording(void) 
{
    if (Rec.GetRecState() == RecordModule::RecordState::Record_running) {
        if (m_recording_circle_active) {
            M5.Display.fillCircle(5, 5, 3, M5.Display.color565(255, 0, 0));
        } else {
            M5.Display.fillCircle(5, 5, 3, M5.Display.color565(0, 0, 0));
        }
    }
}

void ScreensModule::_IndicateSending(void) 
{
    if (Rec.GetSendState() == RecordModule::SendState::Send_start || Rec.GetSendState() == RecordModule::SendState::Send_transmit || Rec.GetSendState() == RecordModule::SendState::Send_confirm) {
        M5.Display.setRotation(3);
        M5.Display.setTextSize(1);
        M5.Display.setTextFont(2);
        M5.Display.setCursor(0, 0);
        lgfx::TextStyle previouse_style = M5.Display.getTextStyle();
        if (m_sending_arrow_active) {
            M5.Display.setTextColor(M5.Display.color565(255, 255, 255));
        } else {
            M5.Display.setTextColor(M5.Display.color565(0, 0, 0));
        }
        M5.Display.print("->");
        M5.Display.setTextStyle(previouse_style);
    }
}

void ScreensModule::_IndicateScreenType(void) 
{
    char screen_type_character = '\0';

    switch (m_active_screen) {

        case Rtc_time:
            screen_type_character = 'T';
            break;

        case Record_will_be_lost:
        case Delete_record:
        case Start_record:
        case Planned_record:
        case Record_running:
        case Record_successful:
            screen_type_character = 'R';
            break;

        case Choose_record_mode:
            screen_type_character = 'M';
            break;

        case Send_file:
        case File_sending:
        case File_sending_successful:
        case File_sending_failed:
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

void ScreensModule::_DisplayTimer(TimerHandle_t xTimer) 
{
    static size_t ms10 = 0;

    if ((ms10 % 50) == 0) {
        Screens.m_recording_circle_active ^= 1;
        Screens.m_sending_arrow_active ^= 1;
        
        if (Rec.GetRecState() == RecordModule::RecordState::Record_running || Rec.GetSendState() == RecordModule::SendState::Send_start || 
            Rec.GetSendState() == RecordModule::SendState::Send_transmit || Rec.GetSendState() == RecordModule::SendState::Send_confirm) {
            Screens.RefreshScreen(false);
        }
    }

    if ((ms10 % 100) == 0) {
        if (Screens.m_active_screen == File_sending || Screens.m_active_screen == Planned_record) {
            Screens.RefreshScreen(false);
        }
    }

    if (ms10 >= 1000) ms10 = 0;
    ms10++;
}

void ScreensModule::_DisplayTask(void* arg) 
{
    auto self = (ScreensModule*)arg;
    self->m_xtimer_display = xTimerCreate("DisplayTimer", pdMS_TO_TICKS(10), pdTRUE, (void *)0, ScreensModule::_DisplayTimer);
    xTimerStart(self->m_xtimer_display, 0);
    while (1) {       
        if (self->m_screen_transition) {
            M5.Display.clear();
            self->m_screen_transition = false;
        }
        /* Set default settings */
        M5.Display.setRotation(3);
        M5.Display.setTextSize(1);
        M5.Display.setTextFont(4);
        switch (self->m_active_screen) {

            case Startup:
                self->_ScreenStartUp();
                break;

            case Prepare_file_system:
                self->_ScreenPrepareFileSystem();
                break;

            case Rtc_time:
                self->_ScreenCurrentTime();
                break;

            case Start_record:
                self->_ScreenStartRecording();
                break;

            case Planned_record:
                self->_ScreenPlannedRecording();
                break;

            case Record_will_be_lost:
                self->_ScreenRecordWillBeLost();
                break;

            case Delete_record:
                self->_ScreenDeleteRecord();
                break;

            case Record_running:
                self->_ScreenRecordRunning();
                break;

            case Record_successful:
                self->_ScreenRecordSuccessful();
                break;

            case Choose_record_mode:
                self->_ScreenChooseRecordMode();
                break;
            
            case Send_file:
                self->_ScreenSendFile();
                break;
        
            case File_sending:
                self->_ScreenFileSending();
                break;

            case File_sending_successful:
                self->_ScreenFileSendingSuccessful();
                break;

            case File_sending_failed:
                self->_ScreenFileSendingFailed();
                break;

            default: break;
        
        }
        self->_IndicateRecording();
        self->_IndicateSending();
        self->_IndicateScreenType();
        xSemaphoreTake(self->m_display_binary_semaphore, portMAX_DELAY);
    }
}

void ScreensModule::Begin(void) 
{
    M5.Display.begin();
    m_display_binary_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(_DisplayTask, "DisplayTask", 1024 * 2, this, 4,
                &m_xhandle_display);
}

void ScreensModule::SetScreen(ScreensTypes new_screen) 
{
    if (m_active_screen != new_screen) {
        m_screen_transition = true;
        m_active_screen = new_screen;
    }
    xSemaphoreGive(m_display_binary_semaphore);
}

void ScreensModule::RefreshScreen(bool full_redraw) 
{
    m_screen_transition = full_redraw;
    xSemaphoreGive(m_display_binary_semaphore);
}
