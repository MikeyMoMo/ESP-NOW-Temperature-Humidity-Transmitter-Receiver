const int packetVersion = 2;  // This is version 2 of the data packet.
#pragma pack(push, 1)  // Pack everything to 1 byte boundaries.
// Structure to send
typedef struct struct_message {
  uint8_t  version;
  int32_t  pktSeq;
  int32_t  myTempC;             // Degrees C * 100.  Divide on receiver.
  uint16_t myRH;                // RH% * 100.        Divide on receiver.
  int      goodSendsCumulative;  // Count of total send successes
  int      badSendsCumulative;   // Count of total send failures
  uint8_t  crc8;                 // Calculate on both sides and compare.
} struct_message;
struct_message transferData;
#pragma pack(pop)      // Restores the previous packing setting.

// How often to send
int sendTime = 300;   // Every this many seconds

// To save display updates and power, we will go into a lower update rate in these hours.

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

// All of the time zone definitions must be the same size as the
//  largest one.  Your program might run out of memory if this is
//  not observed. It is a known bug in a library that they refuse to fix.
// DO NOT CHANGE THIS ONE!!!
const char * cZulu    = "UTC0                  ";  // DO NOT CHANGE OR REMOVE!
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

