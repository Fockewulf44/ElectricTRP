
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "GlobalConfig.h"

class WiFiScheduler
{
private:
    /* data */
public:
    struct tm WiFiTurnOn;
    struct tm WiFiTurnOff;
    int WifiTurnOnCycleMin;
    int WifiTurnOffCycleMin;
    bool IsWiFiMinCycleTurnOn;
    WiFiScheduler(/* args */);
    void LoopProcessor();
    ~WiFiScheduler();
};

WiFiScheduler::WiFiScheduler(/* args */)
{
    //predefining WiFi turnOn and Off
    WiFiTurnOff.tm_hour = 1;
    WiFiTurnOff.tm_min = 0;
    WiFiTurnOn.tm_hour = 8;
    WiFiTurnOn.tm_min = 0;
    IsWiFiMinCycleTurnOn = false;
    WifiTurnOnCycleMin = 0;
    WifiTurnOffCycleMin = 5;
}

void WiFiScheduler::LoopProcessor()
{
    time_t now;
    struct tm *timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    if (IsWiFiMinCycleTurnOn)
    {
        if (timeinfo->tm_min == WifiTurnOnCycleMin)
        {
            if (!WiFi.isConnected())
            {
                WiFi.begin(Config.ssid, Config.password);
                
            }
        }

        if (timeinfo->tm_min == WifiTurnOffCycleMin)
        {
            if (WiFi.isConnected())
            {
                WiFi.disconnect();
            }
        }
    }
}

WiFiScheduler::~WiFiScheduler()
{
}
