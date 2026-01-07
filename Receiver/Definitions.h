// This code currently supports 2 transmitters but I added one to each
//  array so the transmitters could be counted starting at 1.  You know,
//  like god intended counting to work!!!  ;-))
// Add one since I start counting at 1.
#define TRANSMITTER_CT 3  // 2 Transmitters supported by coding 3.

#include <esp_now.h>
#include <WiFi.h>
#include "esp_sntp.h"      // Get UTC epoch here.
#include "TimeLib.h"       // Nice utilities for time manipulation/display.
#include <vector>          // Required for std::vector
#include <GFX_Setups.h>    // Setup for showing info on T5 ePaper

// Code to let Brownout detector be disabled. (Not currently used.)
//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

// ------------------------------------------------------------
// NOTE: This sketch uses a relative include:
//   #include "../CommonFile_v2.h"
// Arduino IDE 2.x supports this, so the file can live
// one level up in the sketchbook folder.
// If compilation ever fails, move CommonFile.h into
// libraries/CommonStuff/ and include it as <CommonFile.h>.
// ------------------------------------------------------------
#include "../CommonFile_v2.h"

#define OUTSIDE 1  // Points into the arrays to follow. 0 not used.
#define INSIDE  2  // Points into the arrays to follow. 0 not used.
float  temperature[TRANSMITTER_CT] = {0., 1000., 1000.};
// The lower (lowest) temp from the current cycle.
float  currentOutLowest = 1000.;  
float  RH[TRANSMITTER_CT] = {0., 0., 0.};  // Humidity from the transmitters.
// This is the time the temp and RH data arrived from each transmitter.
time_t epochData[TRANSMITTER_CT] = {0, 0, 0};
int    missedPackets[TRANSMITTER_CT] = {0, 0, 0};
int    prevPktSeq[TRANSMITTER_CT] = {0, -1, -1};
char * localesS[TRANSMITTER_CT] = {" ", "Out", "In"};
char * localesL[TRANSMITTER_CT] = {" ", "Outside", "Inside"};
char * sIniting("Initializing...");
int    intervalReceives[TRANSMITTER_CT] = {0, 0, 0};

// Set low rate update starting at midnight, ending at 10am.
#define LowRateStart 0  // Go into slower wakeup at this hour.
#define LowRateEnd   9  // Use computed rate (including backOff) starting at this hour.

// Transmitter ID of INSIDE sender.  Be sure to change if hardware changes.
char * insideMAC = "B4:E6:2D:8D:1B:85";  // INSIDE
char * tempType;
char   cCharWork[200];
char   cDST[10];
int    iYear, iMonth, iDay, iHomeOffset, iTempOffset;
int    iCurrMinute, iCurrHour, iPrevHour = 100, iCurrSecond, iCurrDOW;
int    iCurrMonth, iCurrDay, iCurrYear;
int    senderType; // (1 or 2 or maybe 3 some day...)
int    localCRC8;  // Impossible so it will not match.
int    x, y;
time_t workTime, UTC, lastReportTime;
struct tm * timeinfo;

unsigned long reportInterval = sendTime * 1000;  
int16_t tbx, tby; uint16_t tbw, tbh;
bool OnDataRecv_Running = false;

// After this many milliseconds, clear temp and RH
//  from that transmitter.
time_t staleDelay = 720000;  // If the data is this old (ms), clear it.

// And, now, the Vector setups.
const unsigned long DAY_SECONDS = 24UL * 60UL * 60UL;
struct TempReading {
  unsigned long timestamp;   // epoch time or millis()
  float temperature;         // the actual reading
};

//*************************************************
#define iIN_RANGE(v, low, high) (((low) <= (high)) ? ((v) >= (low) && (v) <= (high)) : ((v) >= (low) || (v) <= (high)))
//SleepTime = xIN_RANGE(12, 23, 10);  // Should return false
#define xIN_RANGE(v, low, high) (((low) <= (high)) ? ((v) > (low) && (v) < (high)) : ((v) > (low) || (v) < (high)))

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
  - reserve() + capacity() are the duo that make capacity() meaningful. Without reserve(), capacity is just whatever the implementation decided to allocate.
  - Iterators are invalidated when the vector reallocates, so knowing capacity can help avoid surprises.
  - Modifiers like insert() and erase() can be expensive if they move many elements, since vectors store data contiguously.

  Limitations & Gotchas
  - Growth strategy (how much capacity increases) is implementation-defined. Typically it’s about 1.5x or 2x, but you can’t rely on exact numbers.
  - shrink_to_fit() is non-binding: the implementation may ignore it.
  - max_size() is usually huge (close to size_t limits) and not practically useful.

  size()     returns the number of elements present in the vector
  clear()    removes all the elements of the vector
  front()    returns the first element of the vector
  back()     returns the last element of the vector
  empty()    returns 1 (true) if the vector is empty
  capacity() check the overall size of a vector
*/

// Global vector of readings
std::vector<TempReading> readings;
const int iExpectedSize = 600;
