#include "general.h"
#include "RtcModule.h"
#include "RecordModule.h"
#include "ScreensModule.h"

void CheckButtons(void) 
{
    if (M5.BtnA.wasClicked()) {

        switch (Screens.GetActiveScreen()) {

        case Screen::Rtc_time:
            if (Rec.GetRecState() == RecordModule::RecordState::Record_running) {
                Screens.SetScreen(Screen::Record_running);
            } else if (Rec.GetRecState() == RecordModule::RecordState::Record_planned) {
                Screens.SetScreen(Screen::Planned_record);
            } else if (Rec.GetSendState() != RecordModule::SendState::Send_inactive) {
                Screens.SetScreen(Screen::File_sending);
            } else {
                Screens.SetScreen(Screen::Start_record);
            }
            break;

        case Screen::Start_record:
        case Screen::Record_will_be_lost:
        case Screen::Record_running:
        case Screen::Planned_record:
            if (Rec.GetRecState() == RecordModule::RecordState::Record_running || Rec.GetRecState() == RecordModule::RecordState::Record_planned) {
                Screens.SetScreen(Screen::Rtc_time);
            } else {
                if (Rec.GetRecordExists()) Screens.SetScreen(Screen::Send_file);
                else Screens.SetScreen(Screen::Choose_record_mode);
            }
            break;

        case Screen::Send_file:
        case Screen::File_sending:
            Screens.SetScreen(Screen::Choose_record_mode);
            break;

        case Screen::Choose_record_mode:
            Screens.SetScreen(Screen::Rtc_time);
            break;

        default: break;

        }

    } else if (M5.BtnB.wasClicked()) {

        switch (Screens.GetActiveScreen()) {

        case Screen::Start_record:
            if (Rec.GetRecordExists() && (Rec.GetRecType() == RecordModule::RecordType::Flash_raw || Rec.GetRecType() == RecordModule::RecordType::Flash_timed)) {
                Screens.SetScreen(Screen::Record_will_be_lost);
            } else if (Rec.GetRecType() == RecordModule::RecordType::Flash_timed) {
                Rec.StartSchedule();
                Screens.SetScreen(Screen::Planned_record);
            } else {
                Rec.Start();
                Screens.SetScreen(Screen::Record_running);
            }
            break;

        case Screen::Record_will_be_lost:
            Screens.SetScreen(Screen::Delete_record);
            Rec.Delete();
            Screens.SetScreen(Screen::Start_record);
            break;

        case Screen::Planned_record:
            Rec.StopSchedule();
            Screens.SetScreen(Screen::Start_record);
            break;

        case Screen::Record_running:
            Rec.Stop();
            break;

        case Screen::Choose_record_mode:
            if (Rec.GetRecState() != RecordModule::RecordState::Record_running && Rec.GetRecState() != RecordModule::RecordState::Record_planned) {
                switch (Rec.GetRecType()) {
                case RecordModule::RecordType::Uart:        Rec.SetRecType(RecordModule::RecordType::Flash_raw); break;
                case RecordModule::RecordType::Flash_raw:   Rec.SetRecType(RecordModule::RecordType::Flash_timed); break;
                case RecordModule::RecordType::Flash_timed: Rec.SetRecType(RecordModule::RecordType::Uart); break;
                }
                Screens.RefreshScreen(false);
            }
            break;

        case Screen::Send_file:
        case Screen::File_sending:
            if (Rec.GetSendState() == RecordModule::SendState::Send_inactive) {
                Rec.StartSending();
                Screens.SetScreen(Screen::File_sending);
            } else if (Rec.GetSendState() == RecordModule::SendState::Send_start) {
                Rec.StopSending(false);
            }
            break;

        default: break;

        }

    } else if (M5.BtnPWR.wasClicked()) {
        if (Rec.GetSendState() == RecordModule::SendState::Send_inactive && Rec.GetRecState() == RecordModule::RecordState::Record_inactive) {
            ESP.restart();
        }
    } else if (M5.BtnPWR.wasHold()) {
        if (Rec.GetSendState() == RecordModule::SendState::Send_inactive && Rec.GetRecState() == RecordModule::RecordState::Record_inactive) {
            M5.Power.powerOff();
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

    Screens.Begin();
    Screens.SetScreen(Screen::Startup);

    Rtc.Begin();
    Rec.Begin();

    Screens.SetScreen(Screen::Rtc_time);
}

void loop()
{
    bool use_delay = true;
    M5.update();

    /* Check current rtc time */
    Rtc.Update();

    /* Check recording states */
    Rec.Update();
    if (Rec.GetRecState() == RecordModule::RecordState::Record_running) use_delay = false;
    if (Rec.GetSendState() == RecordModule::SendState::Send_start || Rec.GetSendState() == RecordModule::SendState::Send_transmit || Rec.GetSendState() == RecordModule::SendState::Send_confirm) use_delay = false;

    /* Check buttons presses */
    CheckButtons();

    if (use_delay) vTaskDelay(pdMS_TO_TICKS(10));
}