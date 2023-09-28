#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#define MotorPin 4
#include <Wire.h>
#include "Adafruit_AS726x.h"
#include "colorname.h"
#ifdef ESP32
#include <esp_wifi.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#endif
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiManager.h>

#include <time.h>

#define SENSOR_MAX 1000

// Declare sensor object
Adafruit_AS726x ams;
float calibratedValues[AS726x_NUM_CHANNELS];

// Create AsyncWebServer object on port 80
#ifdef ESP32
AsyncWebServer server(80);
#define serverRequest AsyncWebServerRequest
#else
ESP8266WebServer server(80);

#endif

int max_reading = 1000;

unsigned int targets[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
byte targetIndex = 0;
byte lastTarget = 0;
byte currentTarget = targets[targetIndex];
long lastTargetTime = 0;

long startCircle = millis();

int sector = -999;
String color = "";
byte count_color = 0;
bool targetFound = false;
int lastSectorMillis = 0;
bool processQueue = true;
bool clockmode = false;

void trackSectors();
void moveToTarget();
void processClock();

void printLocalTime()
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
  else
  {
    Serial.println("Failed to obtain time");
  }
}

void setup_time()
{
  // Initialize SNTP
  configTime(0, 0, "pool.ntp.org");              // GMT with no daylight saving time
  setenv("TZ", "IST-2IDT,M3.4.4/26,M10.5.0", 1); // Set time zone to GMT (modify this to your time zone)

  // Wait for time synchronization
  while (time(nullptr) < 1000000ul)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();

  // Print the current time
  printLocalTime();
}

void clockOn()
{
  clockmode = true;
  processQueue = false;

  Serial.println("clock mode");
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;

  res = wm.autoConnect("AutoConnectAP"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected)");
  }
  setup_time();
  Wire.begin();

  Serial.println("Prop Clock");
  if (!ams.begin())
  {
    Serial.println("could not connect to sensor! Please check your wiring.");
    while (1)
      ;
  }
  ams.setConversionType(MODE_2);
  ams.setDrvCurrent(LIMIT_1MA);
  pinMode(MotorPin, OUTPUT);
#ifdef ESP32
  server.on("/clock", HTTP_GET, [](serverRequest *request)
            {
             clockOn();
              Serial.println(request->url());
              request->send(200, "text/plain", "clock mode"); });
#else
  server.on("/clock", []()
            {
              clockOn();
              Serial.println(server.uri());
              server.send(200, "text/plain", "clock mode"); });
#endif
#ifdef ESP32
  server.on("/reset", HTTP_GET, [](serverRequest *request)
            {
      // reset the device
      request->send(200, "text/plain", "resetting");
      delay(4000);
      ESP. restart(); });
#else
  server.on("/reset", []()
            {
      // reset the device
      server.send(200, "text/plain", "resetting");
      delay(4000);
      ESP.restart(); });
#endif
#ifdef ESP32
  server.on("/go", HTTP_GET, [](serverRequest *request)
            {
              Serial.println(request->url());
              clockmode = false;
              processQueue = true;
              if (request->hasParam("n"))
              {
                String number = request->getParam("n")->value();
                int n = number.toInt();
                if (n >= 1 && n <= 16)
                {
                  request->send(200, "text/plain", "moving to sector " + number);
                  currentTarget = n;
                  lastTarget = 0;
                  targetFound = false;
                  Serial.print("target from URL: ");
                  Serial.println(currentTarget);
                }
              } });
#else
  server.on("/go", []()
            {
              Serial.println(server.uri());
              clockmode = false;
              processQueue = true;
              if (server.hasArg("n"))
              {
                String number = server.arg("n");
                int n = number.toInt();
                if (n >= 1 && n <= 16)
                {
                  server.send(200, "text/plain", "moving to sector " + number);
                  currentTarget = n;
                  lastTarget = 0;
                  targetFound = false;
                  Serial.print("target from URL: ");
                  Serial.println(currentTarget);
                }
              } });
#endif
  server.begin(); // Start the server
  Serial.println("wifi server started at: " + WiFi.localIP().toString());

  clockOn();
}

long lastTimeUpdate = 0;
void loop()
{

  trackSectors();
  if (clockmode)
    processClock();

  moveToTarget();

  yield();
}

void processClock()
{
  if (millis() - lastTimeUpdate > 2000)
  {
    lastTimeUpdate = millis();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      int hour = timeinfo.tm_hour;
      if (hour > 12)
        hour -= 12;
      currentTarget = hour;
    }
  }
}

long switchTime = 0;
// process sensor
void trackSectors()
{
  if (ams.dataReady())
  {

    // read the values!
    ams.readCalibratedValues(calibratedValues);
    for (int i = 0; i < AS726x_NUM_CHANNELS; i++)
    {
      if (calibratedValues[i] > max_reading)
      {
        max_reading = calibratedValues[i];
      }
    }

    ams.setDrvCurrent(LIMIT_12MA5);
    ams.drvOn();

    int r = (ams.readCalibratedRed() + ams.readCalibratedOrange()) / 2;
    int g = (ams.readCalibratedGreen() + ams.readCalibratedYellow()) / 2;
    int b = (ams.readCalibratedBlue() + ams.readCalibratedViolet()) / 2;
    r = map(r, 0, max_reading, 0, 255);
    g = map(g, 0, max_reading, 0, 255);
    b = map(b, 0, max_reading, 0, 255);

    String currentColor = ColorNameString(r, g, b);
    if (currentColor == "Green")
      currentColor = "Yellow";
    if (currentColor == "Indigo")
      currentColor = "White";

    if (currentColor != color)
    {
      count_color++;
    }
    // Serial.print(color);
    // Serial.print(" ");
    if (count_color > 1)
    {
      Serial.print(sector);
      Serial.print(" ");

      lastTargetTime = millis() - switchTime;
      switchTime = millis();
      count_color = 0;
      color = currentColor;

      if (color == "White")
      {
        sector = 12;
      }
      else
        sector++;
      if (sector > 12)
        sector = 1;
    }
  }
}

long targetEnteredTime = 0;
bool move = false;
// moves to the next target. if found, waits 10 seconds before moving on to the next in the list
void moveToTarget()
{
  // set motor state
  // if white was not found, keep rotating
  if (sector < 0)
  {
    digitalWrite(MotorPin, HIGH);
  }
  else
  {
    // if we are on target, stop rotating else keep rotating
    digitalWrite(MotorPin, move);
    if (sector == currentTarget) // if we are on target
    {
      if (!targetFound) // just once
      {
        targetEnteredTime = millis();
        lastTargetTime = millis();
        Serial.print("Found target: ");
        Serial.println(currentTarget);
        targetFound = true;
      }
      move = (millis() - targetEnteredTime) <= lastSectorMillis / 2;

      // if target was found, wait 10 seconds before moving on
      // keeping lastTarget allows to skip the previous target sector
      if (millis() - lastTargetTime > 10000 && lastTarget != sector)
      {
        if (processQueue)
        {
          targetIndex++; // move to next target
          if (targetIndex >= sizeof(targets) / sizeof(targets[0]))
            targetIndex = 0;
          lastTarget = currentTarget; // save last target so we dont immediately move on
          currentTarget = targets[targetIndex];
          Serial.print("New target: ");
          Serial.println(currentTarget);
          targetFound = false;
        }
      }
    }
    else
      move = true;
  }
}