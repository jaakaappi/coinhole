#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/13, /* data=*/11, /* cs=*/10, /* dc=*/9, /* reset=*/8);

const char *ssid = "replace";
const char *password = "replace";
String serverName = "replace";
const int serverCheckIntervalMs = 2 * 1000;

const int lightSensorThreshold = 1000;
const int lightSensorPin = A0;
const int buttonPin = GPIO_NUM_39;

const int ledPin = A18;
const int ledPwmChannel = 0;
const int ledPwmResolution = 8;
const int ledPwmFrequency = 1000;

enum WIFI_STATUS
{
  WIFI_DISCONNECTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED
};

enum SERVER_STATUS
{
  SERVER_DISCONNECTED,
  SERVER_CONNECTING,
  SERVER_READY,
  SERVER_PLAYING
};

WIFI_STATUS wifiStatus = WIFI_DISCONNECTED;
SERVER_STATUS serverStatus = SERVER_DISCONNECTED;

int lastServerCheck = 0;
int lastLedFade = 0;
int ledFadeStep = 5;
int ledBrightness = 0;

void updateDisplay()
{
  u8g2.clearBuffer();
  switch (wifiStatus)
  {
  case WIFI_DISCONNECTED:
    u8g2.drawStr(0, 0, "Wifi disconnected");
    break;
  case WIFI_CONNECTING:
    u8g2.drawStr(0, 0, "Wifi connecting");
    break;
  case WIFI_CONNECTED:
    u8g2.drawStr(0, 0, "Wifi ok");
    break;
  }
  switch (serverStatus)
  {
  case SERVER_DISCONNECTED:
    u8g2.drawStr(0, 0, "Server disconnected");
    break;
  case SERVER_CONNECTING:
    u8g2.drawStr(0, 10, "Server connecting");
    break;
  case SERVER_READY:
    u8g2.drawStr(0, 10, "Server ready");
    break;
  case SERVER_PLAYING:
    u8g2.drawStr(0, 10, "Server playing");
    break;
  }
  u8g2.sendBuffer();
}

void sendCoin()
{
  Serial.println("Sending coin");
  HTTPClient http;

  String serverPath = serverName + "/coin";
  http.begin(serverPath.c_str());
  // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  int httpResponseCode = http.POST("");
  Serial.print("Server coin response");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0 && httpResponseCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    Serial.println(payload);
    if (httpResponseCode == HTTP_CODE_CONFLICT || httpResponseCode == HTTP_CODE_OK)
    {
      serverStatus = SERVER_PLAYING;
    }
    else
    {
      serverStatus = SERVER_DISCONNECTED;
    }
  }
  else
  {
    serverStatus = SERVER_DISCONNECTED;
  }
  http.end();
}

void checkWifi()
{
  if (WiFi.status() != WL_CONNECTED && wifiStatus != WIFI_CONNECTING)
  {
    Serial.println("Wifi disconnected");
    wifiStatus = WIFI_CONNECTING;
    Serial.println("Wifi connecting");
    updateDisplay();
    WiFi.begin(ssid, password);
  }
  else
  {
    Serial.println("Wifi connected");
    wifiStatus = WIFI_CONNECTED;
  }
}

void checkServer()
{
  serverStatus = SERVER_CONNECTING;
  HTTPClient http;

  String serverPath = serverName + "/status";
  http.begin(serverPath.c_str());
  // http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  int httpResponseCode = http.GET();
  Serial.print("Server status response");
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
    if (httpResponseCode == HTTP_CODE_CONFLICT)
    {
      serverStatus = SERVER_PLAYING;
    }
    else if (httpResponseCode == HTTP_CODE_OK)
    {
      serverStatus = SERVER_READY;
    }
    else
    {
      serverStatus = SERVER_DISCONNECTED;
    }
  }
  else
  {
    serverStatus = SERVER_DISCONNECTED;
  }
  http.end();
}

void setup()
{
  Serial.begin(115200);

  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  wifiStatus = WIFI_CONNECTING;
  updateDisplay();
  WiFi.begin(ssid, password);
  Serial.println("Wifi connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  wifiStatus = WIFI_CONNECTED;
  updateDisplay();

  ledcAttachPin(ledPin, ledPwmChannel);
  ledcSetup(ledPwmChannel, ledPwmFrequency, ledPwmResolution);
}

void loop()
{
  checkWifi();
  if (millis() >= lastServerCheck + serverCheckIntervalMs || serverStatus == SERVER_DISCONNECTED)
  {
    checkServer();
    lastServerCheck = millis();
  }
  if (serverStatus == SERVER_READY)
  {
    if ((analogRead(lightSensorPin) >= lightSensorThreshold || digitalRead(buttonPin) == HIGH))
    {
      sendCoin();
    }

    if (millis() >= lastLedFade + 30)
    {
      ledcWrite(ledPwmChannel, ledBrightness);
      ledBrightness = ledBrightness + ledFadeStep;
      if (ledBrightness <= 0 || ledBrightness >= 255)
      {
        ledFadeStep = -ledFadeStep;
      }
      lastLedFade = millis();
    }
  }
  else
  {
    ledcWrite(ledPwmChannel, 0);
  }
}