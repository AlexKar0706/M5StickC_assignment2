#include "general.h"
#include "RtcModule.h"
#include "ScreensModule.h"
#if (USE_NTP_TIME)
#include "secret_example.h"
#if (!USE_SECRET_EXAMPLE)
#include "secret.h"
#endif
#include <WiFi.h>
#include <esp_sntp.h>
#endif

RtcModule Rtc;

void RtcModule::_TimerCallback(TimerHandle_t xTimer) 
{
    Rtc.m_rtc_get_time = true;
}

void RtcModule::Begin(void) 
{
    struct tm tm;
#if (USE_NTP_TIME)
    /* Get time from NTP server */
    uint32_t retry_attempts = 3;
    while (retry_attempts--) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        uint8_t wifi_sync_max = 100;
        while (WiFi.status() != WL_CONNECTED && wifi_sync_max != 0) {
            vTaskDelay(pdMS_TO_TICKS((100)));
            wifi_sync_max--;
        }
        if (wifi_sync_max == 0) {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            continue;
        }
        configTime(m_gmtOffset_sec, m_daylightOffset_sec, WIFI_NTP_SERVER);
        sntp_sync_status_t sync_status = sntp_get_sync_status();
        uint8_t status_sync_max = 100;
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
    m_rtc_timestamp = M5.Rtc.getDateTime().get_tm();
    m_rtc_get_time = false;
    m_xtimer_rtc = xTimerCreate("RtcTimerCallback", pdMS_TO_TICKS(50), pdTRUE, (void *)0, _TimerCallback);
    xTimerStart(m_xtimer_rtc, 0);
}

void RtcModule::Update(bool force_update) 
{
    if (m_rtc_get_time || force_update) {
        m_rtc_timestamp = M5.Rtc.getDateTime().get_tm();
        m_current_time = mktime(&m_rtc_timestamp);
        m_rtc_get_time = false;
        if (Screens.GetActiveScreen() == Screen::Rtc_time) Screens.RefreshScreen(false);
    }
}
