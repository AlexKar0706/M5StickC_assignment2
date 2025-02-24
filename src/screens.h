#pragma once

typedef enum {
    SCREEN_STARTUP,
    SCREEN_PREPARE_FILE_SYSTEM,
    SCREEN_RTC_TIME,
    SCREEN_RECORD_WILL_BE_LOST,
    SCREEN_DELETE_RECORD,
    SCREEN_START_RECORD,
    SCREEN_RECORD_RUNNING,
    SCREEN_RECORD_SUCCESSFUL,
    SCREEN_CHOOSE_RECORD_MODE,
    SCREEN_SEND_FILE,
    SCREEN_FILE_SENDING,
    SCREEN_FILE_SENDING_SUCCESSFUL,
    SCREEN_FILE_SENDING_FAILED,
} Screens_t;

extern Screens_t active_screen;

void ScreensInit(void);
void ChangeScreen(Screens_t new_screen);
void RefreshScreen(bool full_redraw);
