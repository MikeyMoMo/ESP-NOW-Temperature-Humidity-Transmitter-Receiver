// T5 (caseless) MAC is 7C:9E:BD:FA:EF:7C
// T5 (w/case)   MAC is 7C:9E:BD:FA:F0:1C

// ------------------------------------------------------------
// NOTE: This sketch uses a relative include:
//   #include "../CommonFile.h"
// Arduino IDE 2.x supports this, so the file can live
// one level up in the sketchbook folder.
// If compilation ever fails, move CommonFile.h into
// libraries/CommonStuff/ and include it as <CommonFile.h>.
// ------------------------------------------------------------
#include "../CommonFile_v2.h"

bool ANT20_Present;
bool BME280_Present;

unsigned long long sleepFor;

// Maximum bad sends before reboot.
#define badSendMax 4  // Before reboot.
// This retains value across deep sleep but is reset on ESP.restart().
// Count of failures in a row. When it exceeds badSendMax, we reboot.
RTC_DATA_ATTR uint32_t badSendsForReboot = 0;
// This is incremented for every send.  Receiver ignores duplicates.
RTC_NOINIT_ATTR int32_t seqCounter;  // Keep going till power cycle.
// Total count of good sends since powerup.
RTC_NOINIT_ATTR uint32_t goodSendsCumulative;
// Total count of bad sends since powerup.
RTC_NOINIT_ATTR uint32_t badSendsCumulative;
// This is a watermark to say that variables have/have not been init'd.
RTC_NOINIT_ATTR uint32_t rtcMagic = 0xDEADBEEF;

uint32_t backOff, baseSleep;
int jitter;
float workFloatT, workFloatH;

#define maxbackOff 1800  // Cap at 1/2 hour (seconds)

// Your receiver's MAC address (for hitting a specific receiver): (w/ACKs)
//uint8_t targetMAC[] = {0x7C, 0x9E, 0xBD, 0xFA, 0xEF, 0x7C};  // Caseless T5
uint8_t targetMAC[] = {0x7C, 0x9E, 0xBD, 0xFA, 0xF0, 0x1C};  // T5 w/case

// To broadcast to all receivers on the node, use this: (w/o ACKs)
//uint8_t targetMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,};                                               {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*
   For your failed send counter that survives reboots,
    declare it with RTC_NOINIT_ATTR:
   RTC_NOINIT_ATTR int failedAttempts = 0;
   RTC_NOINIT_ATTR int testVar = 0;

  void setup() {
  Serial.begin(115200);
  testVar++;
  Serial.printf("Test var after reset: %d\n", testVar);
  delay(5000);
  ESP.restart();  // Will see 2, 3, 4... on each boot
  }

  Key Differences

  RTC_DATA_ATTR:   Survives deep sleep cycles only; wiped by any reset/reboot.
  RTC_NOINIT_ATTR: Survives resets/reboots too; cleared only by full power-off.
                   Perfect for crash counters or error tracking across restarts.
*/
