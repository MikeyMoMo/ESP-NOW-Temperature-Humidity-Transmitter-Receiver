// T5 (caseless)        MAC: 7C:9E:BD:FA:EF:7C (2 cores, no PSRAM)
// T5 (w/case)          MAC: 7C:9E:BD:FA:F0:1C (2 cores, no PSRAM)
// Dev Module w/MSP2008 MAC: B4:E6:2D:8D:CC:7D (2 cores, w/ PSRAM)
// Tiny S3-Zero         MAC: E8:F6:0A:81:9F:EC (2 cores, no PSRAM)
// Tiny regular S3      MAC: 98:3D:AE:F1:F9:50 (2 cores, no PSRAM)
// CYD                  MAC: A4:F0:0F:5E:8A:E8 (2 cores, no PSRAM)
// T7 41.4              MAC: C4:4F:33:56:4C:71 (2 cores, w/ PSRAM 4MB)
// T7 (inside)          MAC: 2C:BC:BB:A8:C7:CC

const int packetVersion = 4;  // This is version 4 of the data packet.
#pragma pack(push, 1)  // Pack everything to 1 byte boundaries.
// Structure to send
typedef struct struct_message {
  uint8_t  version;
  int32_t  pktSeq;
  int32_t  myTempC;              // Degrees C * 100. Divide on receiver.
  uint16_t myRH;                 // RH% * 100.       Divide on receiver.
  int      goodSendsCumulative;  // Count of total send successes
  int      badSendsCumulative;   // Count of total send failures
  uint8_t  MAC[6];               // sensor MAC
  uint16_t VBAT;                 // Voltage of the battery at VBAT socket.
  int      len;                  // Entire length of this packet
  uint8_t  crc8;                 // Calculate on both sides and compare.
} struct_message;
struct_message xferData;
#pragma pack(pop)      // Restores the previous packing setting.

// How often to send readings.
int sendTime = 300;   // Every this many seconds

// This code currently supports 8 OUTSIDE transmitters
//  plus 1 INSIDE transmitter.
#define MAX_OUTSIDE_SENSORS 8  // 0-7  For more or less, just change this.
// For array allocation 8 + 1
#define MAX_SENSORS MAX_OUTSIDE_SENSORS + 1
// Slot #8 (the 9th slot) is the inside sensor.
#define INSIDE_SLOT INSIDE

// All of the time zone definition lengths must be the same length as the
//  largest one.  Your program might run out of memory if this is
//  not observed. It is a known bug in a library that they refuse to fix.
// By accident, none were used yet the timezone is correct!  Sweet!!
#ifdef CONFIG_FOR_JOE
const char * ssid     = "N_Port";           // Change this to your WiFi SSID
const char * wifipw   = "helita1943";       // Change this to your WiFi password
const char * cityName = "Benicia";
const char * LocalTZ  = "PST8PDT,M3.2.0,M11.1.0";  // Change to your time string.
#else
const char * ssid     = "Converge2G";       // Change this to your WiFi SSID
const char * wifipw   = "Lallave@Family7";  // Change this to your WiFi password
const char * cityName = "Bangui";
const char * LocalTZ  = "PHT-8                 ";  // Change to your time string.
#endif

// DO NOT CHANGE THIS ONE!!!
const char * cZulu    = "UTC0                  ";  // DO NOT CHANGE OR REMOVE!

#define RGB565(r,g,b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#define RGB888(r,g,b) ((r << 16) | (g << 8) | b)

/*
struct Config {
    const char* ssid;
    const char* pw;
    const char* city;
    const char* tz;
};

// Add as many deployments as you like
Config configs[] = {
    { "N_Port",     "helita1943",  "Benicia", "PST8PDT,M3.2.0,M11.1.0" },
    { "Converge2G", "Lallave@Family7", "Bangui", "PHT-8                 " },
    { "TestNet",    "password123", "Lab",     "UTC0                  " }
};

// Pick which one to use at compile time or runtime
#define DEPLOYMENT_INDEX 1   // 0 = Joe, 1 = Micheal, etc.

Config active = configs[DEPLOYMENT_INDEX];
*/

// Then

/*
WiFi.begin(active.ssid, active.pw);
Serial.println(active.city);
setenv("TZ", active.tz, 1);
tzset();
*/

