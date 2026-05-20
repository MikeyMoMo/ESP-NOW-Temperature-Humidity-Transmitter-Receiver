#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// ------------------------------------------------------------
// NOTE: This sketch uses a relative include:
//   #include "../CommonFile_vn.h"  // "n" is the version number.
// Arduino IDE 2.x supports this, so the file can live
// one level up in the sketchbook folder.
// If compilation ever fails, move CommonFile.h into
// libraries/CommonStuff/ and include it as <CommonFile.h>.
// ------------------------------------------------------------
#include "../CommonFile_v4.h"  // pull in shared definitions

// Define the enum
enum {
  INSIDE,     // 
  OUTSIDE,    // 1
  ENUM_COUNT  //
};

// Use ENUM_COUNT to size your arrays
const char *localesL[ENUM_COUNT] = {
  "Inside",
  "Outside"
};

const char *localesS[ENUM_COUNT] = {
  "In",
  "Out"
};

// This code currently supports 8 OUTSIDE transmitters
//  plus 1 INSIDE transmitter.
#define MAX_OUTSIDE_SENSORS 8  // 1-8  For more or less, just change this.
// For array allocation 8 + 1
#define MAX_SENSORS MAX_OUTSIDE_SENSORS + 1
// Slot #0 is the inside sensor.
#define INSIDE_SLOT INSIDE

typedef struct {
  uint8_t macAddr[6];        // Binary MAC address (no ":"s)
  struct_message xferData;   // Structured payload
  unsigned long lastUpdate;  // millis() timestamp of last packet
  char boardDesc[25];        // Text description of the device
} SenderSlot;
SenderSlot slots[MAX_SENSORS];

/*****************************************************************************/
// Define a struct for each sensor slot
struct ArrivalTracking {
  bool newPacket;          // flag set in callback
  uint8_t MAC[6];          // MAC address
  time_t arrivalTime;      // captured at callback
  String desc;             // human-friendly description
};
ArrivalTracking arrivals[MAX_SENSORS];  // Array of records for all sensors

#include <esp_now.h>
#include <WiFi.h>
#include "esp_sntp.h"      // Get UTC epoch here.
#include "TimeLib.h"       // Nice utilities for time manipulation/display.
#include <vector>          // Required for std::vector

#include <TFT_eSPI.h>      // Remember to change the User_Setup_Select.h
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
bool useSprite = true;  // Until proven false.
#include <XPT2046_Touchscreen.h>
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

#define CYD_LED_RED    4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE  17

#include "OpenFontRender.h" // Include after M5Stack.h / M5Core2.h
OpenFontRender ofr;
// Area to return version information (from library)
char vers[ofr.FT_VERSION_STRING_SIZE];
// Areas to return credits information (from library)
char cred[ofr.CREDIT_STRING_SIZE];
int  rc;

#define RGB565(r,g,b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#define MACs_EQUAL(a, b) (memcmp((a), (b), 6) == 0)
#define OutsideBlue RGB565(118, 242, 255)
#define InsideRed   RGB565(255, 159, 161)
#define HiLoGreen   RGB565(161, 255, 183)

#include "Fonts.h"

// Code to let Brownout detector be disabled. (Not currently used.)
//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

float  lowSlotsOutsideTemp = 1000.;
float  RH[MAX_SENSORS];  // Humidity from the transmitters.
int    prevPktSeq[MAX_SENSORS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1};
const  char * sIniting = "Initializing...";
const  char * WiFiConnected = "Initialized.";
const  char * na = "n/a";
char   input;
bool   intervalReceives[ENUM_COUNT] = {false, false};
//bool   newPacketReceived[ENUM_COUNT] = {false, false};
//bool   newPacketReceived[MAX_SENSORS] = {false};  // Same for all 8.
// Allocate space for 9 MAC addresses (each 6 bytes)
//uint8_t newPacketMACs[MAX_SENSORS][6] = {0};   // initialized to zero
//time_t packetTimes[MAX_SENSORS];

int    infoLine[6] = {0, 20, 60, 100, 140, 180};
int    lowestTempSlot, hightestTempSlot;
double lamda;
double denom;
const  double DPTol = 1e-3;  // DP tolerance (very small number, near 0)
double dewPoint;
// The lower (lowest) temp from the current cycle.
const unsigned long STALE_TIMEOUT = 900000; // 15 minutes
// A special string to summon reboot.
const char *REBOOT_MSG = "asdcponaohuedsrasdf";

int minTemp, maxTemp;
// Set this when the temp is not valid.
#define BAD_TEMP_HI_SENTINEL  100000.
// Use this to check for invalid temp.
#define BAD_TEMP_HI_CHECK      90000.
// Set this when the temp is not valid.
#define BAD_TEMP_LO_SENTINEL -100000.
// Use this to check for invalid temp.
#define BAD_TEMP_LO_CHECK     -90000.

// To save display updates and power, we will go into a lower update rate
//  in some hours.

// Set low rate update starting at midnight, ending at 10am.
#define LowRateStart 0  // Go into slower display update rate at this hour.
// Use computed rate (including backOff) starting at this hour.
#define LowRateEnd   9

bool isMAC_Allowed;

typedef struct {
  uint8_t MAC[6];
  const char *desc;
} AllowedDevice;

AllowedDevice allowedDevices[] = {
  // 0 (Inside)
  { {0x2C, 0xBC, 0xBB, 0xA8, 0xC7, 0xCC}, "T5 (Inside)"},
  // 0 (Outside 1)
  { {0x4C, 0x11, 0xAE, 0xA6, 0x9F, 0x34}, "T-Display (Back)"},
  // 1 (Outside 2)
  { {0xC4, 0x4F, 0x33, 0x56, 0x4C, 0x71}, "T7 v1.4 (Front)"},
  // 2 (Outside 3)
  { {0xB4, 0xE6, 0x2D, 0x8D, 0x1B, 0x85}, "Repeater"},
  // 3 (Outside 4)
  { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "None Registered"},
  // 4 (Outside 5)
  { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "None Registered"},
  // 5 (Outside 6)
  { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "None Registered"},
  // 6 (Outside 7)
  { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "None Registered"},
  // 7 (Outside 8)
  { {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "None Registered"},
};
#define MAX_MACs (sizeof(allowedDevices) / sizeof(allowedDevices[0]) + 1)

uint8_t zeroMac[6] = {0, 0, 0, 0, 0, 0};

#ifdef USE_F_TEMPS
const char* tempType = "F";
#else
const char* tempType = "C";
#endif

char   cCharWork[200];
char   cDST[10];
int    iYear, iMonth, iDay, iHomeOffset, iTempOffset;
int    iCurrHour, iPrevHour = 100, iCurrSecond, iCurrDOW;
int    iCurrMinute, iPrevMinute = 100, iPrevSecond = 100;
int    iCurrMonth, iCurrDay, iCurrYear;
int    localCRC8;
int    x, y;
uint16_t tsX, tsY;
uint8_t  tsZ;
int    idx;
int    lowOutTempSlot;  //, hiOutTempSlot;
int    min24Temp, max24Temp;
int    currSlot;
const  int tempY =  56;
const  int RHY   = 114;
time_t workTime, UTC, lastReportTime;
struct tm * timeinfo;
int    queued_slot;
double DPtempC;
double DPRH;
double rhFraction;
int ldr;
// Averaging buffer
const int NUM_SAMPLES = 40;
int  samples[NUM_SAMPLES];
int  ringIndex = 0;
long sum = 0;
int  startLDR = 1500;
int  newBrightness, prevBrightness;
int  avgLdr;
// Pinning + slew parameters
int pinnedBrightness = -1;
int prevPinnedBrightness = -100;
int DEAD_BAND = 5;          // threshold for ignoring small changes
int SLEW_STEP = 2;          // max change per loop
unsigned long SNAP_TIMEOUT = 4000; // ms before forcing target
unsigned long lastChangeTime = 0;
unsigned long lastRun = 0;
unsigned long millisNow;

#define SNAP_DELTA   12    // brightness units
//#define DEAD_BAND    4

int    prevOutTemp = -1000;

int sensorID;
#define SENSOR_NOT_FOUND -1
#define SENSOR_TABLE_FULL -2

unsigned long reportInterval = sendTime * 1000;
int16_t tbx, tby; //uint16_t tbw, tbh;
//bool OnDataRecv_Running = false;

// After this many milliseconds, clear temp and RH
//  from that transmitter.
time_t staleDelay = 720000;  // If the data is this old (ms), clear it.

// And, now, the Vector setups.
const unsigned long DAY_SECONDS = 24UL * 60UL * 60UL;
struct OutTempReading {
  unsigned long timestamp;   // epoch time or millis()
  int temperature;         // the actual reading
};

int ihourlyBrilliance[] = {
#if defined CONFIG4MIKE
  // Time-controlled display brightness.
  //                           0    1    2    3    4    5            Hours
  40,  20,  20,  10,  10,  10,      //  0- 5
  //6   7    8    9   10   11
  30,  50,  70,  80, 120, 130,      //  6-11
  //12  13   14   15   16   17
  140, 140, 140, 140, 140, 140,     // 12-17
  //18  19   20   21   22  23
  120, 100,  80,  70,  60, 50   // 18-23
#else
  //                          0    1    2    3    4    5            Hours
  70,  60,  50,  40,  30,  30,      //  0- 5
  //6   7    8    9   10   11
  50,  60,  70,  80,  80, 100,      //  6-11
  //12  13   14   15   16   17
  160, 160, 160, 160, 160, 160,     // 12-17
  //18   19    20    21    22  23
  160,  160,  160,  160,  120, 80   // 18-23
#endif
};

//*******************************************************************
// In both of these, the high and low values are not order dependent.
// This one is inclusive of both ends of the range.
//SleepTime = xIN_RANGE(12, 10, 12);  // Should return true
#define iIN_RANGE(v, low, high) \
  (((low) <= (high)) ? \
   ((v) >= (low) && (v) <= (high)) : \
   ((v) >= (low) || (v) <= (high)))

// This one is exclusive of both ends of the range.
//SleepTime = xIN_RANGE(12, 23, 10);  // Should return false
#define xIN_RANGE(v, low, high) \
  (((low) <= (high)) ? \
   ((v) > (low) && (v) < (high)) : \
   ((v) > (low) || (v) < (high)))
//*******************************************************************
/*
    Vector Functions & Descriptions

    https://en.cppreference.com/w/cpp/container/vector.html

    Construction & Assignment
    - vector() – constructors (default, copy, move, initializer list)
    - operator= – assignment (copy, move, initializer list)
    - assign() – replace contents with new values

    Element Access
    - at() – bounds-checked access
    - operator[] – unchecked access
    - front() – first element
    - back() – last element
    - data() – pointer to underlying array

    Iterators
    - begin(), cbegin() – iterator to first element
    - end(), cend() – iterator past last element
    - rbegin(), crbegin() – reverse iterator to last element
    - rend(), crend() – reverse iterator past first element

    Capacity
    - empty() – check if vector is empty
    - size() – number of elements
    - max_size() – maximum possible size
    - reserve() – request capacity
    - capacity() – current reserved capacity
    - shrink_to_fit() – reduce capacity to fit size

    Modifiers
    - clear() – remove all elements
    - insert() – insert elements
    - emplace() – construct element in place
    - erase() – remove elements
    - push_back() – add element at end
    - emplace_back() – construct element at end
    - pop_back() – remove last element
    - resize() – change size
    - swap() – swap contents with another vector

    Allocator
    - get_allocator() – return allocator used

    Notes & Tips
    - reserve() + capacity() are the duo that make capacity() meaningful.
      Without reserve(), capacity is just whatever the implementation
      decided to allocate.
    - Iterators are invalidated when the vector reallocates,
      so knowing capacity can help avoid surprises.
    - Modifiers like insert() and erase() can be expensive if
       they move many elements, since vectors store data contiguously.

    Limitations & Gotchas
    - Growth strategy (how much capacity increases) is implementation-defined.
      Typically it’s about 1.5x or 2x, but you can’t rely on exact numbers.
    - shrink_to_fit() is non-binding: the implementation may ignore it.
    - max_size() is usually huge (close to size_t limits) and
      not practically useful.

    size()     returns the number of elements present in the vector
    clear()    removes all the elements of the vector
    front()    returns the first element of the vector
    back()     returns the last element of the vector
    empty()    returns 1 (true) if the vector is empty
    capacity() check the overall size of a vector
*/

// Global vector of readings
std::vector<OutTempReading> outReadings;
const int iExpectedSize = 300;

#endif
