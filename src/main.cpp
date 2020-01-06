#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "ESP8266HTTPClient.h"
#include "time.h"
#include <ESPAsyncTCP.h>
#include "ArduinoOTA.h"
#include <ESPAsyncWebServer.h>
#include "GlobalConfig.h"
#include <LinkedList.h>


#define LED 2
#define pinMotionSensor 4
#define pinRelayShocker 5
// #define pinReserved 16

AsyncWebServer server(80);

class TRPlog
{
public:
  tm detectedDTM;
};

ll::LinkedList<TRPlog> trpLogList = ll::LinkedList<TRPlog>();

struct TRPstatus
{
  int Detected;
  tm lastTimeDetected;
};

TRPstatus trpStatus;

void GetLocalTime(tm *ti)
{
  time_t now;
  struct tm timeinfo;
  time(&now);
  timeinfo = *localtime(&now);
  ti = &timeinfo;
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(pinMotionSensor, INPUT);
  pinMode(pinRelayShocker, OUTPUT);

  IPAddress ip(192, 168, 86, 120);
  IPAddress gateway(192, 168, 86, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(Config.ssid, Config.password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  // configTime(-28800, 0, "pool.ntp.org");
  configTime(-28800, 0, "64.22.253.155");
  int timeSyncCounter = 0;
  while (time(nullptr) <= 100000 && timeSyncCounter < 25)
  {
    Serial.println();
    Serial.print(".");
    timeSyncCounter++;
    delay(300);
  }

  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();

  server.on("/getStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Get requested: ");
    time_t now;
    struct tm *timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    String pageHead = "<html><head></head><body>";

    String pageLog = "";
    for (int i = 0; i < trpLogList.size(); i++)
    {
      TRPlog log = trpLogList.get(i);
      pageLog += "<div>" + String(log.detectedDTM.tm_hour) + ":" + String(log.detectedDTM.tm_min) + ":" + String(log.detectedDTM.tm_sec) + "</div>";

      // pageLog += "<div>" + String(now) + ":" + String(log.detectedDTM.tm_min) + ":" + String(log.detectedDTM.tm_sec) + "</div>";
    }

    String pageBottom = "<div>Time: #time</div>"
                        "<div>Total detected: #totaldetected</div>"
                        "</body</html>";

    pageBottom.replace("#time", String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min) + ":" + String(timeinfo->tm_sec));
    pageBottom.replace("#totaldetected", String(trpStatus.Detected));
    String page;
    page = pageHead + pageLog + pageBottom;

    char timeStringBuff[150]; //50 chars should be enough
    // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", timeinfo);
    request->send(200, "text/html", page);
  });

  server.begin();

  trpStatus.Detected = 0;
}

int firstDetect = -1;
int ShokerOn = 0;
bool IsShokerOn = false;
unsigned long prevMS = 0;
unsigned long startMotionSensorDetMS = 0;
unsigned long startShockerMS = 0;
unsigned long lastMtSensReadMS = 0;
unsigned long lastMtSensReadMS2 = 0;
int ledState = LOW;

void loop()
{
  ArduinoOTA.handle();
  unsigned long currentMS = millis();

  // time_t now;
  // struct tm *timeinfo;
  // time(&now);
  // timeinfo = localtime(&now);
  // unsigned long currentMS = millis();

  if (IsShokerOn)
  {
    if (startShockerMS == 0)
    {
      startShockerMS = currentMS;
    }
    if (currentMS - startShockerMS > 3000)
    {
      IsShokerOn = false;
      startShockerMS = 0;
      // digitalWrite(pinRelayShocker, LOW);
      digitalWrite(LED, HIGH);
    }
  }
  //Adding some delay for reading Sensor, to avoid reading Motion Sensor's value every millisecond
  if (currentMS - lastMtSensReadMS > 500)
  {
    if (digitalRead(pinMotionSensor) == HIGH && IsShokerOn == false)
    {
      if (startMotionSensorDetMS == 0)
      {
        startMotionSensorDetMS = currentMS;
      }

      if (currentMS - startMotionSensorDetMS > 5000)
      {
        IsShokerOn = true;
        startMotionSensorDetMS = 0;
        // digitalWrite(pinRelayShocker, HIGH);
        digitalWrite(LED, LOW);

        time_t now;
        struct tm *timeinfo;
        time(&now);
        timeinfo = localtime(&now);
        trpStatus.Detected++;
        // catcherStatus.lastTimeDetected = &timeinfo;
        TRPlog trpLog;
        trpLog.detectedDTM = *timeinfo;
        trpLogList.add(trpLog);
      }
    }
    else
    {
      startMotionSensorDetMS = 0;
    }
    Serial.println(currentMS);
    Serial.println(lastMtSensReadMS);
    Serial.println("WiFi status: " + String(WiFi.status()));

    time_t now = time(nullptr);
    Serial.println(ctime(&now));
    Serial.println("#######");
    lastMtSensReadMS = currentMS;
  }
}

// ####### Deep Sleep for 30 seconds ###############
// if (currentMS - lastMtSensReadMS2 > 30000)
// {
//   ESP.deepSleep(30e6);
//   lastMtSensReadMS2 = currentMS;
// }