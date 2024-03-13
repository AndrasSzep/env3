//! include.h

#include "config.h"
#include "mywifi.h"
#include "BoatData.h"
#include "aux_functions.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiServer.h>
#include <NTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <math.h>
#include <time.h>

unsigned long previousMillis = 0;       // Variable to store the previous time
unsigned long storedMillis = 0;         // time of last stored env.data
unsigned long previousPacketMillis = 0; // Variable to store the previous packet reception time
