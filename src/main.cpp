#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "ESP8266HTTPClient.h"
#include "time.h"
#include <ESPAsyncTCP.h>
#include "ArduinoOTA.h"
#include <ESPAsyncWebServer.h>
#include "GlobalConfig.h"

// const char *ssid = "Sonoma #3 Google Wifi";
// const char *password = "word_dawg1999";

#define LED 2
#define pinMotionSensor 16
#define pinRelayShocker 5
#define pinReserved 4

AsyncWebServer server(80);

struct CatcherStatus
{
  int Detected;
  tm lastTimeDetected;
};

CatcherStatus catcherStatus;

void GetLocalTime(tm *ti)
{
  time_t now;
  struct tm *timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  ti = timeinfo;
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(pinMotionSensor, INPUT);
  pinMode(pinRelayShocker, OUTPUT);

  WiFi.begin(Config.ssid, Config.password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  configTime(-28800, 0, "north-america.pool.ntp.org");

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
    struct tm *timeinfo;
    GetLocalTime(timeinfo);
    // try
    // {
    //   time(&now);
    //   timeinfo = localtime(&now);
    // }
    // catch (exception err)
    // {
    //   Serial.println(err.what());
    // }
    char timeStringBuff[150]; //50 chars should be enough
    // strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", timeinfo);
    request->send(200, "text/plain", "Hello, Local Time: " + String(timeStringBuff) + " Detected: " + String(catcherStatus.Detected) + " LastTimeDetected: " + String(catcherStatus.lastTimeDetected.tm_hour) + ":" + String(catcherStatus.lastTimeDetected.tm_min));
  });

  server.begin();

  catcherStatus.Detected = 0;
}

bool IsSchedule1Activated = false;
int counter = 0;

int firstDetect = -1;
int ShokerOn = 0;
bool IsShokerOn = false;
unsigned long prevMS = 0;
unsigned long startMotionSensorDetMS = 0;
unsigned long startShockerMS = 0;
unsigned long lastMtSensReadMS = 0;
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
      digitalWrite(pinRelayShocker, LOW);
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
        digitalWrite(pinRelayShocker, HIGH);
        digitalWrite(LED, LOW);

        struct tm *timeinfo;
        GetLocalTime(timeinfo);
        catcherStatus.Detected++;
        // catcherStatus.lastTimeDetected = &timeinfo;
      }
    }
    else
    {
      startMotionSensorDetMS = 0;
    }
    lastMtSensReadMS = currentMS;
  }
}