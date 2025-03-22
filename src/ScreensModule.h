#pragma once

#include "general.h"

class ScreensModule {

public:

    enum ScreensTypes {
        Startup,
        Prepare_file_system,
        Rtc_time,
        Record_will_be_lost,
        Delete_record,
        Start_record,
        Planned_record,
        Record_running,
        Record_successful,
        Choose_record_mode,
        Send_file,
        File_sending,
        File_sending_successful,
        File_sending_failed,
    };

private:

    TimerHandle_t m_xtimer_display;
    TaskHandle_t m_xhandle_display = NULL;
    bool m_recording_circle_active = false;
    bool m_sending_arrow_active = false;
    volatile SemaphoreHandle_t m_display_binary_semaphore  = NULL;
    volatile bool m_screen_transition;
    ScreensTypes m_active_screen;

    static void _DisplayTimer(TimerHandle_t xTimer);
    static void _DisplayTask(void* arg);

    void _ScreenStartUp(void);
    void _ScreenPrepareFileSystem(void);
    void _ScreenCurrentTime(void);
    void _ScreenStartRecording(void);
    void _ScreenPlannedRecording(void);
    void _ScreenRecordWillBeLost(void);
    void _ScreenDeleteRecord(void);
    void _ScreenRecordRunning(void);
    void _ScreenRecordSuccessful(void);
    void _ScreenChooseRecordMode(void);
    void _ScreenSendFile(void);
    void _ScreenFileSending(void);
    void _ScreenFileSendingSuccessful(void);
    void _ScreenFileSendingFailed(void);
    void _IndicateRecording(void);
    void _IndicateSending(void);
    void _IndicateScreenType(void);

public:

    ScreensModule() {};
    void Begin(void);
    void SetScreen(ScreensTypes new_screen);
    void RefreshScreen(bool full_redraw);
    ScreensTypes GetActiveScreen(void) { return m_active_screen; };
    ~ScreensModule() {};
};

using Screen = ScreensModule::ScreensTypes;

extern ScreensModule Screens;
