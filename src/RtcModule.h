#pragma once

#include "general.h"

class RtcModule {
private:
    const long m_gmtOffset_sec     = 3600;
    const int m_daylightOffset_sec = 3600;
    TimerHandle_t m_xtimer_rtc;
    time_t m_current_time;
    struct tm m_rtc_timestamp;
    bool m_rtc_get_time;
    
    static void _TimerCallback(TimerHandle_t xTimer);
public:
    RtcModule() {};
    void Begin(void);
    void Update(bool force_update = false);
    time_t GetCurrentTime(void) { return m_current_time; } 
    struct tm GetCurrentDateTime(void) { return m_rtc_timestamp; } 
    ~RtcModule() {};
};

extern RtcModule Rtc;
