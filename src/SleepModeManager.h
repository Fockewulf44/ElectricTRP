
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "GlobalConfig.h"

class SleepModeManager
{
private:
    bool HasWiFiTurningOnStarted;
    bool HasWiFiTurningOffStarted;

public:
    struct tm WiFiTurnOn;
    struct tm WiFiTurnOff;
    int WifiTurnOnCycleMin;
    int WifiTurnOffCycleMin;
    bool IsWiFiMinCycleTurnOn;
    SleepModeManager(/* args */);
    void LoopProcessor();
    ~SleepModeManager();
};

SleepModeManager::SleepModeManager(/* args */)
{
    //predefining WiFi turnOn and Off
    WiFiTurnOff.tm_hour = 1;
    WiFiTurnOff.tm_min = 0;
    WiFiTurnOn.tm_hour = 8;
    WiFiTurnOn.tm_min = 0;
    IsWiFiMinCycleTurnOn = false;
    WifiTurnOnCycleMin = 10;
    WifiTurnOffCycleMin = 55;
    HasWiFiTurningOnStarted = false;
    HasWiFiTurningOffStarted = false;
}

void SleepModeManager::LoopProcessor()
{
    time_t now;
    struct tm *timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    if (IsWiFiMinCycleTurnOn)
    {
        if (timeinfo->tm_sec == WifiTurnOnCycleMin)
        {
            if (HasWiFiTurningOnStarted == false)
            {
                HasWiFiTurningOnStarted = true;
                if (WiFi.status() != WL_CONNECTED)
                {
                    Serial.println("Waking up...connecting");
                    WiFi.setSleepMode(WIFI_NONE_SLEEP);
                    WiFi.begin(Config.ssid, Config.password);
                    Serial.println("Woke up, connected");
                }
            }
        }
        else
        {
            HasWiFiTurningOnStarted == false;
        }

        if (timeinfo->tm_sec == WifiTurnOffCycleMin)
        {
            if (HasWiFiTurningOffStarted == false)
            {
                HasWiFiTurningOffStarted = true;
                Serial.println("Going light sleep");
                if (WiFi.status() == WL_CONNECTED)
                {
                    WiFi.disconnect();
                }
                WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
                Serial.println("Light sleep is on");
            }
        }
        else
        {
            HasWiFiTurningOffStarted == false;
        }
    }
}

SleepModeManager::~SleepModeManager()
{
}
