//! config.h for N2K_DataDisplay_WiFi.ino
/* 
by Dr.András Szép under GNU General Public License (GPL).
*/

#define DEBUG       //additional print of all data on serial

//#define STOREWIFI   // store wifi credentials on the SPIFFS

#define READWIFI    // get Wifi credentials from SPIFFS

#define ENVSENSOR       //environmental sensors connected

#ifdef ENVSENSOR
#define SDA_PIN 26    //be careful - it collides with the CANBUS TX/RX pins on M5AtomU
#define SCL_PIN 32
#endif

//OTA port  - if defined it means we can access the OverTheAir interface to update files on the SPIFFs and the programm itself
#define OTAPORT 8080    
const char*   servername  = "env3";     //nDNS servername - http://servername.local

const char* ntpServer = "europe.pool.ntp.org";
const int timeZone = 0;  // Your time zone in hours
//String  UTC ="2023-07-11 20:30:00";
#define TIMEPERIOD 60.0

const double radToDeg = 180.0 / 3.14159265358979323846;
const double msToKn = 3600.0 / 1852.0;
const double kpaTommHg = 133.322387415;
const double KToC = 273.15;
const double mpsToKn = 1.943844;

#ifdef ENVSENSOR
#include <M5StickC.h>
#include "M5_ENV.h"

SHT3X sht30;
QMP6988 qmp6988;
float tmp = 20.0;
float hum = 50.0;
float pres = 760.0;
#define ENVINTERVAL 360000     // Interval in milliseconds - once/minute
#define STOREINTERVAL 360000 // store env data in SPIFFS ones/hour
#endif
