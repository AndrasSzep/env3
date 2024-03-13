//! ENV-wifi.ino
/*
Copyright (c) 2024 Dr.Andras Szep

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
/*
This is an AI (chatGPT) assisted development for
Arduino ESP32 code to display data from NMEA2000 data bus.
https://shop.m5stack.com/products/atom-canbus-kit-ca-is3050g
through a webserver to be seen on any mobile device for free.

::::: Websockets used to autoupdate the data.

::::: Environmental sensors incorporated and data for the last 24hours stored respectively
in the SPIFFS files /pressure.txt, /temperature.txt, /humidity.txt.
The historical environmental data displayed in the background as charts using https://www.chartjs.org

::::: Local WiFi attributes are stored at SPIFFS in files named /ssid.txt and /password.txt = see initWiFi()
WPS never been tested but assume working.

::::: Implemented OverTheAir update of the data files as well as the code itself on port 8080
(i.e. http://env3.local:8080 ) see config.h .
*** Arduino IDE 2.0 does not support file upload, this makes much simplier uploading updates
especially in the client and stored data files.

ToDo:
::::: LED lights on M5Atom. Still need some ideas of colors and blinking signals
*/

#include "include.h"
const String byme = "by Dr.András Szép v1.0 22.02.2024";
const String whatami = "ENV -> WiFi";

#define LITTLE_FS 0
#define SPIFFS_FS 1
#define FILESYSTYPE SPIFFS_FS
// const char *otahost = "OTASPIFFS";
#define FILEBUFSIZ 4096

#ifndef WM_PORTALTIMEOUT
#define WM_PORTALTIMEOUT 180
#endif


WiFiUDP udp;
NTPClient timeClient(udp, ntpServer, timeZone);
time_t utcTime = 0;


sBoatData stringBD; // Strings with BoatData

Stream *OutputStream;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");
char sn[30];                // to store mDNS name
WebServer servOTA(OTAPORT); // webserver for OnTheAir update on port 8080
bool fsFound = false;
#include "webpages.h"
#include "filecode.h"

double lastTime = 0.0;
String message = "";
String timedate = "1957-04-28 10:01:02"; // guess when the new era was born
String humidity = "50";
String pressure = "760";
String airtemp = "21";
String pressurearray = "760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760,760";

JsonDocument jsonDoc;

void notifyClients()
{
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  // Send the JSON string as a response
  //        request->send(200, "application/json", jsonString);
  //        client->text(jsonString);
  if (jsonString.length() != 0 && jsonString != "null")
  {
#ifdef DEBUG
    Serial.print("notifyClients: ");
    Serial.println(jsonString);
#endif
    ws.textAll(jsonString);
    jsonDoc.clear();
  }
}

void handleRequest(AsyncWebServerRequest *request)
{
  if (request->url() == "/historicdata")
  {
    // Create a JSON document
    JsonDocument jsonDoc;
    // read data
    jsonDoc["histtemp"] = readStoredData("/temperature.txt");
    jsonDoc["histhum"] = readStoredData("/humidity.txt");
    jsonDoc["histpres"] = readStoredData("/pressure.txt");
    notifyClients();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
      // Handle received text message
      data[len] = '\0'; // Null-terminate the received data
      Serial.print("Received message: ");
      Serial.println((char *)data);
      // Check if the request is '/historicdata'
      if (strcmp((char *)data, "/historicdata") == 0)
      {
        // Prepare JSON data
        JsonDocument jsonDoc;
        // read data
        jsonDoc["histtemp"] = readStoredData("/temperature.txt");
        jsonDoc["histhum"] = readStoredData("/humidity.txt");
        jsonDoc["histpres"] = readStoredData("/pressure.txt");
        notifyClients();
      }
      else
      {
        // Handle other requests here
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("\n\n");
  Serial.println("***** ENV -> WEB *****");
  Serial.println(byme);
  Serial.print("\n\n");
  delay(100);
#ifdef ENVSENSOR
    M5.begin();             // Init M5StickC.  初始化M5StickC
    Wire.begin();  // Wire init, adding the I2C bus.  Wire初始化, 加入i2c总线
    qmp6988.init();
    sht30.init();
  Serial.println(F("ENVIII Hat(SHT30 and QMP6988) has initialized "));
#endif

  initFS(false, false); // initialalize file system SPIFFS
  initWiFi();           // init WifI from SPIFFS or WPS
  // Initialize the NTP client
  timeClient.begin();
  stringBD.UTC = getDT(); // store UTC
  
  OutputStream=&Serial;

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });
  // Register the request handler
  server.on("/air", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/temperature.txt", "text/html"); });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/pressure.txt", "text/html"); });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/humidity.txt", "text/html"); });

 // server.on("/historicdata", HTTP_GET, handleRequest);

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();
  Serial.println("WebServer started");
  delay(100);

  strcpy(sn, servername);
  if (!MDNS.begin(sn)) 
    Serial.println("Error setting up MDNS responder!");
  else
    Serial.print("mDNS responder started, OTA: http://");
  Serial.print(sn);

#ifdef OTAPORT
  Serial.printf(".local:%d\n", OTAPORT);
  servOTA.on("/", HTTP_GET, handleMain);

  // upload file to FS. Three callbacks
  servOTA.on(
      "/update", HTTP_POST, []()
      {
    servOTA.sendHeader("Connection", "close");
    servOTA.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = servOTA.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  servOTA.on("/delete", HTTP_GET, handleFileDelete);
  servOTA.on("/main", HTTP_GET, handleMain); // JSON format used by /edit
  // second callback handles file uploads at that location
  servOTA.on(
      "/edit", HTTP_POST, []()
      { servOTA.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>File uploaded. <a href=/main>Back to list</a>"); },
      handleFileUpload);
  servOTA.onNotFound([]()
                     {if(!handleFileRead(servOTA.uri())) servOTA.send(404, "text/plain", "404 FileNotFound"); });

  servOTA.begin();
#endif
  OutputStream->print("Running...");
  storeString("/version.txt", byme + "\n" + whatami + "\n");
  jsonDoc.clear();
  updateEnvironmentalData() ;
}

//*****************************************************************************
void loop() 
{
  jsonDoc.clear();
  updateEnvironmentalData();
#ifdef OTAPORT
  servOTA.handleClient();
#endif
  ws.cleanupClients();
}

void updateEnvironmentalData() {
  #ifdef ENVSENSOR
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= ENVINTERVAL) {
    pres = qmp6988.calcPressure(); // get atmospheric pressure
    if (sht30.get() == 0) { // Obtain the data of SHT30
      tmp = sht30.cTemp; // Store the temperature obtained from SHT30.
      hum = sht30.humidity; // Store the humidity obtained from the SHT30.
    } else {
      tmp = 20, hum = 50; // Default values if SHT30 read fails
    }
    airtemp = String(tmp, 1);
    humidity = String(hum, 1);
    pressure = String((pres / kpaTommHg), 0);
    jsonDoc["airtemp"] = airtemp;
    jsonDoc["humidity"] = humidity;
    jsonDoc["pressure"] = pressure;
    if (currentMillis - previousMillis >= STOREINTERVAL) {
      // Update stored value once per interval
      updateStoredData("/temperature.txt", airtemp.toInt());
      updateStoredData("/humidity.txt", humidity.toInt());
      updateStoredData("/pressure.txt", pressure.toInt());
    }
    notifyClients();
    previousMillis = currentMillis;
  }
  #endif
}