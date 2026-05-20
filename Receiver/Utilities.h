void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *xferDataBytes, int len)
{
  // Cast incoming bytes to your struct
  const struct_message* transferData = (const struct_message*) xferDataBytes;

  // Get sender MAC directly from ESP-NOW
  const uint8_t* senderMAC = info->src_addr;

  // Verify sender MAC against allowedDevices table
  bool isMAC_Allowed = false;
  for (int i = 0; i < MAX_SENSORS; i++) {
    if (memcmp(senderMAC, allowedDevices[i].MAC, 6) == 0) {
      isMAC_Allowed = true;
      break;
    }
  }
  if (!isMAC_Allowed) {
    Serial.printf("Unknown Sender: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  senderMAC[0], senderMAC[1], senderMAC[2],
                  senderMAC[3], senderMAC[4], senderMAC[5]);
    return;
  } else {
    //    Serial.printf("Allowed packet from: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    //                  senderMAC[0], senderMAC[1], senderMAC[2],
    //                  senderMAC[3], senderMAC[4], senderMAC[5]);
    Serial.printf("Allowed packet from: %02X:%02X:%02X:%02X:%02X:%02X "
                  "| Temp: %.2f°C | Hum: %.2f%%\r\n",
                  senderMAC[0], senderMAC[1], senderMAC[2],
                  senderMAC[3], senderMAC[4], senderMAC[5],
                  transferData->myTempC / 100.0,
                  transferData->myRH / 100.0);
  }

  // Now you can safely use transferData->whatever for payload contents

  // Lookup slot using packet MAC instead of info->src_addr

  Serial.printf("Looking up slot for MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                senderMAC[0], senderMAC[1], senderMAC[2],
                senderMAC[3], senderMAC[4], senderMAC[5]);
  int idx = findSlot(senderMAC);

  if (idx < 0) {
    Serial.println("The slots table is full.");
    return;  // table full, skip
  } else {
    Serial.printf("Slot %i was found to be empty or a match.\r\n", idx);
  }

  // 1st sanity check: packet length and version
  if (len != sizeof(struct_message) ||
      transferData->version != packetVersion) {
    Serial.printf("Wrong data packet version, should be %i.\r\n", packetVersion);
    return;
  }

  // 2nd sanity check: CRC8
  uint8_t localCRC8 = crc8((const uint8_t*)transferData,
                           sizeof(struct_message) - 1);
  if (localCRC8 != transferData->crc8) {
    Serial.printf("CRC mismatch for sender %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  transferData->MAC[0], transferData->MAC[1], transferData->MAC[2],
                  transferData->MAC[3], transferData->MAC[4], transferData->MAC[5]);
    return;
  }

  // 3rd sanity check: duplicate packet
  if (prevPktSeq[idx] == transferData->pktSeq) {
    Serial.printf("Duplicate packet from sender %i: %i\r\n", idx, transferData->pktSeq);
    return;
  }

  // Store the packet
  Serial.printf("Storing data in slot %i\r\n", idx);
  memcpy(&slots[idx].xferData, xferDataBytes, len);
  slots[idx].lastUpdate = millis();

  // Mark arrival
  if (idx == INSIDE_SLOT) {
    intervalReceives[INSIDE] = true;
  } else {
    intervalReceives[OUTSIDE] = true;
  }
  memcpy(arrivals[idx].MAC, transferData->MAC, 6);
  time(&arrivals[idx].arrivalTime);
  arrivals[idx].desc = allowedDevices[idx].desc;
  arrivals[idx].newPacket = true;
}
/*****************************************************************************/
const char* getDeviceDesc(const uint8_t mac[6])
/*****************************************************************************/
{
  for (int i = 0; i < MAX_SENSORS; i++) {  // was: MAX_MACs
    if (memcmp(mac, allowedDevices[i].MAC, 6) == 0)
      return allowedDevices[i].desc;
  }
  return "Unknown Device";
}
/*****************************************************************************/
const char* getDeviceDescPadded(const uint8_t mac[6])
/*****************************************************************************/
{
  static char buf[26];  // 25 chars + null terminator
  const char* desc = "Unknown Device";

  for (int i = 0; i < MAX_SENSORS; i++) {
    if (memcmp(mac, allowedDevices[i].MAC, 6) == 0) {
      desc = allowedDevices[i].desc;
      break;
    }
  }

  // Copy description into buffer
  snprintf(buf, sizeof(buf), "%-25.25s", desc);
  return buf;
}

/*****************************************************************************/
void printSlotsDashboard()
/*****************************************************************************/
{
  int i, j = 0;

  Serial.print("As of "); Serial.println(myTimeStamp(time(nullptr)));

  for (i = 0; i < MAX_SENSORS; i++)  // Count used slots.
    if (slotIsUsed(i)) j++;

  if (j > 0)
  {
    // Serial.println(" Sensor (Slots) table listing:");
  } else {
    Serial.println(" Sensor (Slots) table is empty.");  // Nobody home.
    return;
  }

  Serial.println("------------------------------------------------------------"
                 "----------------+");
  Serial.println(" Slot |    MAC Address    |        Description        | Temp"
                 " |  RH  | Volts |");
  Serial.print("------------------------------------------------------------"
               "----------------+");

  for (i = 0; i < MAX_SENSORS; i++) {
    if (slotIsUsed(i)) {
      // Slot number
      Serial.print("\r\n  ");
      Serial.print(i);
      Serial.print("   | ");

      // MAC address
      for (j = 0; j < 6; j++) {
        if (slots[i].macAddr[j] < 0x10) Serial.print("0");
        Serial.print(slots[i].macAddr[j], HEX);
        if (j < 5) Serial.print(":");
      }
      Serial.print(" | ");

      // Lookup description from Allowed Devices
      Serial.print(getDeviceDescPadded(slots[i].macAddr));
      Serial.print(" | ");

      // Payload values (example fields from struct_message)
      Serial.print(slots[i].xferData.myTempC / 100., 1);
      Serial.print(" | ");
      Serial.print(slots[i].xferData.myRH / 100., 1);
      Serial.print(" | ");
      Serial.print(slots[i].xferData.VBAT / 100., 2);
      Serial.print("  |");
    }
  }
  Serial.println("\r\n------------------------------------------------------------"
                 "----------------+");
}
/*****************************************************************************/
void adjustBL()
/*****************************************************************************/
{
  millisNow = millis();
  if (millisNow - lastRun < 100) return;
  lastRun += 100;   // drift-free timing

  // Read sensor
  ldr = analogRead(34);

  // Update ring buffer
  sum -= samples[ringIndex];
  samples[ringIndex] = ldr;
  sum += samples[ringIndex];
  ringIndex = (ringIndex + 1) % NUM_SAMPLES;

  // Average
  avgLdr = sum / NUM_SAMPLES;
  newBrightness = constrain(map(avgLdr, 1900, 875, 0, 255), 0, 255);

  // Deadband + adaptive slew + tempered snap
  if (pinnedBrightness < 0) {
    pinnedBrightness = newBrightness; // initialize
    lastChangeTime = millis();
  } else if (abs(newBrightness - pinnedBrightness) > DEAD_BAND) {
    // Adaptive slew
    int diff = newBrightness - pinnedBrightness;
    int step = constrain(abs(diff) / 4, 1, SLEW_STEP * 4);

    if (diff > 0) {
      pinnedBrightness = min(pinnedBrightness + step, newBrightness);
    } else {
      pinnedBrightness = max(pinnedBrightness - step, newBrightness);
    }
    lastChangeTime = millis();
  } else {
    // Tempered snap: require BOTH time and deviation
    if ((millis() - lastChangeTime > SNAP_TIMEOUT) &&
        (abs(newBrightness - pinnedBrightness) > SNAP_DELTA)) {
      pinnedBrightness = newBrightness;
      lastChangeTime = millis();
    }
  }

  ledcWrite(TFT_BL, pinnedBrightness);
}
/*****************************************************************************/
bool slotIsEmpty(int i)
/*****************************************************************************/
{
  //if (42)      // true (non-zero)
  //if (-7)      // true (non-zero)
  //if (0)       // false
  //  if (i == 0 || i == 2) printMAC(slots[i].macAddr);
  return (MACs_EQUAL(slots[i].macAddr, zeroMac));
}
/*****************************************************************************/
bool slotIsUsed(int i)
/*****************************************************************************/
{
  return !slotIsEmpty(i);
}
/*****************************************************************************/
void printMAC(const uint8_t *mac)
/*****************************************************************************/
{
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                (unsigned int)mac[0], (unsigned int)mac[1],
                (unsigned int)mac[2], (unsigned int)mac[3],
                (unsigned int)mac[4], (unsigned int)mac[5]);
}
/*****************************************************************************/
// Helper: find slot index by MAC, or claim an empty one
int findSlot(const uint8_t *mac)
/*****************************************************************************/
{
  // A slot is considered used if the MAC address is 0.  To release a slot for
  //  reuse, set its MAC address to all 0s.
  // Now, both the allowed devices table and the slots will have slot 8, the
  //  9th actual slot be the INSIDE sensor.  Look at 8 in one and then claim
  //  it in the other.
  if (MACs_EQUAL(mac, allowedDevices[INSIDE_SLOT].MAC)) {
    // Serial.printf("findSlot found INSIDE MAC at %i\r\n", INSIDE_SLOT);
    // Claim slot (mostly redundant)
    memcpy(slots[INSIDE_SLOT].macAddr, mac, 6);
    Serial.println("INSIDE slot matched.");
    return (INSIDE_SLOT);  // Only 1 inside sensor and it has a special home.
  }

  // Not the INSIDE slot (8), find an outside slot 0-7)
  for (int i = OUTSIDE; i < MAX_SENSORS; i++) {
    if (MACs_EQUAL(slots[i].macAddr, mac)) {
      Serial.printf("Found mac match in slot %i findSlot\r\n", i);
      return i;  // existing match
    }
    if (slotIsEmpty(i)) {
      memcpy(slots[i].macAddr, mac, 6); // Claim slot
      Serial.printf("Found empty slot %i in findSlot.\r\n", i);
      return i;
    }
  }
  return SENSOR_TABLE_FULL;  // no room
}
/*****************************************************************************/
// Compute CRC8 using polynomial 0x07
uint8_t crc8(const uint8_t *data, size_t len)
/*****************************************************************************/
{
  uint8_t crc = 0x00;  // initial value
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}
/*****************************************************************************/
void updateTime()
/*****************************************************************************/
{
  // Load up the timeinfo struct for immediate use.
  time(&workTime);
  timeinfo    = localtime(&workTime);
  iCurrHour   = timeinfo->tm_hour;
  iCurrMinute = timeinfo->tm_min;
  iCurrSecond = timeinfo->tm_sec;
  iCurrDay    = timeinfo->tm_mday;
  iCurrDOW    = timeinfo->tm_wday;
  iCurrMonth  = timeinfo->tm_mon + 1;
  iCurrYear   = timeinfo->tm_year + 1900;
  if (iPrevHour != iCurrHour) {
    iPrevHour = iCurrHour;
    Serial.println(localtime(&workTime));
    Serial.printf("The 24 hour history has %i elements.\r\n",
                  outReadings.size());
    showHeapStatus();
    printSlotsDashboard();
  }
}
/*****************************************************************************/
// Add a new reading and purge anything older than 24 hours
void addVectorReading(unsigned long ts, int temp)
/*****************************************************************************/
{
  outReadings.push_back({ts, temp});

  // Purge entries older than 24 hours (86,400 seconds)
  while (!outReadings.empty() &&
         (ts - outReadings.front().timestamp > DAY_SECONDS)) {
    outReadings.erase(outReadings.begin());
  }
}
/*****************************************************************************/
// Print the vector contents
void print24Vector()
/*****************************************************************************/
{
  Serial.printf("Vector minimum capacity %i | Elements used %i\r\n",
                outReadings.capacity(), outReadings.size());
  for (auto &r : outReadings) {
    Serial.printf("[time=%lu, temp=%3.2f]\r\n", r.timestamp,
                  r.temperature / 100.);
  }
  // Serial.println();
}
/*****************************************************************************/
void showFields(int sender)
/*****************************************************************************/
{
  Serial.printf("Sender             %s\r\n", localesL[sender]);

  Serial.print( "Packet Version:    ");
  Serial.println(xferData.version);

  Serial.print("Packet Sequence:   ");
  Serial.println(xferData.pktSeq);

  Serial.print("Temperature:       ");
  Serial.println((float)xferData.myTempC / 100.);

  Serial.print("Relative Humidity: ");
  Serial.println(xferData.myRH / 100.);

  Serial.print("Good Sends:        ");
  Serial.println(xferData.goodSendsCumulative);


  Serial.print("Bad Sends:         ");
  Serial.println(xferData.badSendsCumulative);

  Serial.print("Packet CRC8:       ");
  Serial.println(xferData.crc8);
  // Serial.println("  -------");
}
/*****************************************************************************/
void timeSyncCallback(struct timeval * tv) // Must be before initTime
/*****************************************************************************/
{
  //  struct timeval {  // Instantiated as "*tv"
  // Number of whole seconds of elapsed time
  //   time_t      tv_sec;
  // Number of microseconds of rest of elapsed time minus tv_sec.
  //                             Always less than one million
  //   long int    tv_usec;
  //};
  Serial.println("\n----- Time Sync Received -----");
  Serial.printf("Time sync at %lu ms. UTC Epoch : ", millis());
  Serial.print(tv->tv_sec); Serial.print(" - ");
  Serial.println(ctime(&tv->tv_sec));
  delay(100);
}
/*****************************************************************************/
void initTime()
/*****************************************************************************/
{
  sntp_set_sync_interval(86400000);  // 1 day in ms.
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", LocalTZ, 1); tzset();
  Serial.println("\r\nWaiting for correct time...");

  strftime(cCharWork, sizeof(cCharWork), "%Y", localtime(&workTime));
  iYear = atoi(cCharWork);
  int iLooper = 0;
  while (iYear < 2024) {
    time(&workTime);
    strftime (cCharWork, 100, "%Y", localtime(&workTime));
    iYear = atoi(cCharWork);
    Serial.println(localtime(&workTime), "%A %m/%d/%Y %T %Z");
    if (iLooper++ > 60) {
      Serial.println("Cannot get time set. Rebooting.");
      ESP.restart();
    }
    delay(2000);
  }
  time(&UTC);
  workTime = UTC + iHomeOffset;
  updateTime();
}
/*****************************************************************************/
void printFontError(const char* fontName, int rc, const char* func, int line)
/*****************************************************************************/
{
  Serial.printf("Render loadFont error for %s\n", fontName);
  Serial.printf("In routine %s, near line %d\n", func, line - 2);
  Serial.printf("Error code %i\r\n", rc);
  // Optional: map known error codes to text
  switch (rc) {
    case 1: Serial.println("Error: invalid font data"); break;
    case 2: Serial.println("Error: allocation failed"); break;
    case 3: Serial.println("Error: font already loaded"); break;
    // Add more codes as you discover them
    default: Serial.println("Error: unknown code"); break;
  }
}
/*****************************************************************************/
// Routine to safely unload and load a font (templated)
template <size_t N>
bool safeLoadFont(const char* fontName, const uint8_t (&fontData)[N],
                  int line)
/*****************************************************************************/
{
  int rc = 0;

  try {
    ofr.unloadFont();
  }
  catch (...) {
    Serial.println("unloadFont failed, continuing anyway...");
  }

  rc = ofr.loadFont(fontData, N);  // compiler knows N at compile time
  if (rc) {
    printFontError(fontName, rc, __func__, line);
    return false;  // failure
  }
  return true;  // success
}
/*****************************************************************************/
void printSensorTable()
/*****************************************************************************/
{
  int i, j = 0;

  Serial.print(myTimeStamp(time(nullptr)));

  for (i = 0; i < MAX_SENSORS; i++)  // Count used slots.
    if (slotIsUsed(i)) j++;

  if (j > 0)
  {
    Serial.println(" Sensor table listing:");
  } else {
    Serial.println(" Sensor table empty.");  // Nobody home.
    return;
  }

  for (i = 0; i < MAX_SENSORS; i++) {
    if (slotIsUsed(i)) {
      Serial.print(getDeviceDescPadded(slots[i].macAddr));
      Serial.printf("Used slot %i MAC %02X:%02X:%02X:%02X:%02X:%02X, "
                    "temp %.1f, RH %.1f, VBAT %.2f\r\n", i,
                    (unsigned int)slots[i].macAddr[0],
                    (unsigned int)slots[i].macAddr[1],
                    (unsigned int)slots[i].macAddr[2],
                    (unsigned int)slots[i].macAddr[3],
                    (unsigned int)slots[i].macAddr[4],
                    (unsigned int)slots[i].macAddr[5],
                    (float)slots[i].xferData.myTempC / 100.,
                    (float)slots[i].xferData.myRH / 100.,
                    (float)slots[i].xferData.VBAT / 100.);
    }
  }
}
/*****************************************************************************/
int getOutLowTempSlot()
/*****************************************************************************/
{
  int  lowestTemp = BAD_TEMP_HI_SENTINEL; // sentinel (high) value
  bool any = false;

  for (int i = 1; i < MAX_OUTSIDE_SENSORS; i++) {
    if (slotIsUsed(i)) {
      Serial.printf("LOT-Time check slot %i: millis now %lu, slot updated at %lu\r\n",
                    queued_slot, millis(), slots[queued_slot].lastUpdate);

      if ((millis() - slots[i].lastUpdate) > STALE_TIMEOUT) {
        Serial.printf("getOutLowTempSlot: Sensor %i stale, skipped.\n", i);
        continue;  // Skip this old data.
      } else {
        any = true;
        if (slots[i].xferData.myTempC < lowestTemp) {
          lowestTemp = slots[i].xferData.myTempC;
          lowestTempSlot = i;
        }  // if lower than lowest
      }  // end if stale
    }  // end if slot used
  }  // end for
  return any ? lowestTempSlot : -1;
}
/*****************************************************************************/
void printWrapped(const char *str, int width)
/*****************************************************************************/
{
  int col = 0;
  const char *p = str;

  while (*p) {
    // If we hit a space and we're past the wrap width, break line here
    if (*p == ' ' && col >= width) {
      Serial.print("\r\n");
      col = 0;
      p++; // skip the space
      continue;
    }

    // Print the current character
    Serial.print(*p);
    col++;

    // If we hit newline in the source, reset column count
    if (*p == '\n') {
      col = 0;
    }

    p++;
  }
  Serial.print("\r\n"); // final line break
}
/*****************************************************************************/
void showHeapStatus()
/*****************************************************************************/
{
  // Current free heap
  uint32_t freeHeap = ESP.getFreeHeap();
  // Lowest free heap ever recorded since boot
  uint32_t minHeap  = ESP.getMinFreeHeap();

  Serial.printf("Free heap: %lu bytes, Min ever: %lu bytes\r\n",
                freeHeap, minHeap);
}
/*****************************************************************************/
void get24HourHiLo()
/*****************************************************************************/
{
  if (outReadings.empty()) {
    min24Temp = 0;
    max24Temp = 0;
    return;
  }
  min24Temp = outReadings[0].temperature;
  max24Temp = outReadings[0].temperature;
  for (auto &r : outReadings) {
    if (r.temperature < min24Temp) min24Temp = r.temperature;
    if (r.temperature > max24Temp) max24Temp = r.temperature;
  }
}
/*****************************************************************************/
// Helper: format timestamp
String myTimeStamp(time_t t)
/*****************************************************************************/
{
  char buffer[32];
  struct tm *timeinfo = localtime(&t);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  String result = "[";
  result += buffer;
  result += "]";
  return result;
}
/*****************************************************************************/
void showHelp()
/*****************************************************************************/
{
  // Optional: handle invalid input
  Serial.print("Unknown command "); Serial.println(input);
  Serial.println("Enter D to show Expanded slots table.");
  Serial.println("Enter H to show Heap status.");
  Serial.println("Enter P to show used slots in Sensor table.");
  Serial.println("Enter T to show local Time.");
  Serial.println("Enter V to see the 24 hour Vector table entries.");
  Serial.println("Any other character shows this help.");
}
/*****************************************************************************/
void printTouchToSerial(TS_Point p)
/*****************************************************************************/
{
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}
/*****************************************************************************/
void printPeerInfo()
/*****************************************************************************/
{
  esp_now_peer_num_t peerCount;
  esp_err_t result = esp_now_get_peer_num(&peerCount);

  if (result == ESP_OK)
    Serial.printf("Total peers: %d\r\n", peerCount.total_num);
  else
    Serial.printf("Failed to get peer count, error: %d\r\n", result);
}
/*****************************************************************************/
void addPeer(const uint8_t *mac)
/*****************************************************************************/
{
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;       // use current WiFi channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  } else {
    Serial.printf("Peer added: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  (unsigned int)mac[0], (unsigned int)mac[1],
                  (unsigned int)mac[2], (unsigned int)mac[3],
                  (unsigned int)mac[4], (unsigned int)mac[5]);
  }
}
/*****************************************************************************/
bool inBox(int x, int y, int x1, int y1, int w, int h)
/*****************************************************************************/
{
  return (x >= x1 && x <= (x1 + w) &&
          y >= y1 && y <= (y1 + h));
}
//#include <Preferences.h>   // ESP32 persistence library
//
//Preferences prefs;
//
//// Calibration data
//int xRawMin, xRawMax, yRawMin, yRawMax;
//
//// Known screen dimensions
//const int SCREEN_WIDTH  = 240;
//const int SCREEN_HEIGHT = 320;
//
//// Example calibration points (corners + middles)
//struct Point {
//  int screenX;
//  int screenY;
//  int rawX;
//  int rawY;
//};
//
//Point calibrationPoints[8] = {
//  {0, 0, 0, 0},                 // top-left
//  {SCREEN_WIDTH / 2, 0, 0, 0},  // top-middle
//  {SCREEN_WIDTH - 1, 0, 0, 0},  // top-right
//  {0, SCREEN_HEIGHT / 2, 0, 0}, // middle-left
//  {SCREEN_WIDTH - 1, SCREEN_HEIGHT / 2, 0, 0}, // middle-right
//  {0, SCREEN_HEIGHT - 1, 0, 0}, // bottom-left
//  {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 1, 0, 0}, // bottom-middle
//  {SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 0, 0} // bottom-right
//};
//
///*****************************************************************************/
//void setup() {
///*****************************************************************************/
//  Serial.begin(115200);
//  prefs.begin("touchCal", false);
//
//  // Try to load saved calibration
//  if (prefs.isKey("xMin")) {
//    xRawMin = prefs.getInt("xMin");
//    xRawMax = prefs.getInt("xMax");
//    yRawMin = prefs.getInt("yMin");
//    yRawMax = prefs.getInt("yMax");
//    Serial.println("Loaded calibration from NVS.");
//  } else {
//    Serial.println("No calibration found. Run calibration routine.");
//  }
//}
//
///*****************************************************************************/
//void touch_calibration() {
///*****************************************************************************/
//  // Example: read raw touch values
//  int rawX = readTouchX();  // <-- replace with your touch driver
//  int rawY = readTouchY();
//
//  // Map raw values to screen coordinates
//  int screenX = map(rawX, xRawMin, xRawMax, 0, SCREEN_WIDTH - 1);
//  int screenY = map(rawY, yRawMin, yRawMax, 0, SCREEN_HEIGHT - 1);
//
//  // Use screenX, screenY for UI logic...
//}
//
//// Calibration routine (call once, e.g. from a menu)
///*****************************************************************************/
//void runCalibration() {
///*****************************************************************************/
//  Serial.println("Touch each circle as it appears...");
//
//  for (int i = 0; i < 8; i++) {
//    drawCircle(calibrationPoints[i].screenX, calibrationPoints[i].screenY);
//    calibrationPoints[i].rawX = waitForTouchX();  // blocking read
//    calibrationPoints[i].rawY = waitForTouchY();
//  }
//
//  // Compute min/max from collected points
//  xRawMin = min(calibrationPoints[0].rawX, calibrationPoints[2].rawX);
//  xRawMax = max(calibrationPoints[0].rawX, calibrationPoints[2].rawX);
//  yRawMin = min(calibrationPoints[0].rawY, calibrationPoints[5].rawY);
//  yRawMax = max(calibrationPoints[0].rawY, calibrationPoints[5].rawY);
//
//  // Optionally average across middle points for refinement
//  // Example: average top-middle, bottom-middle for X scaling
//  // Example: average middle-left, middle-right for Y scaling
//
//  // Save to NVS
//  prefs.putInt("xMin", xRawMin);
//  prefs.putInt("xMax", xRawMax);
//  prefs.putInt("yMin", yRawMin);
//  prefs.putInt("yMax", yRawMax);
//
//  Serial.println("Calibration saved.");
//}

// Here begins the MAC updating code:

//// Assume you already have:
//// AllowedDevice allowedDevices[];
//// #define MAX_MACs (sizeof(allowedDevices) / sizeof(allowedDevices[0]))
//
//// Helper: check if slot is empty
///*****************************************************************************/
//bool slotEmpty(int idx) {
///*****************************************************************************/
//  for (int j = 0; j < 6; j++) {
//    if (allowedDevices[idx].mac[j] != 0x00) return false;
//  }
//  return true;
//}
//
//// Add MAC by command
///*****************************************************************************/
//void addMAC(uint8_t mac[6], const char *desc) {
///*****************************************************************************/
//  int target = -1;
//
//  // Rule: slot 8 is reserved for Inside sensor
//  if (slotEmpty(8)) {
//    target = 8;
//  } else {
//    // Otherwise fill from slot 0 upward
//    for (int i = 0; i < 8; i++) {
//      if (slotEmpty(i)) {
//        target = i;
//        break;
//      }
//    }
//  }
//
//  if (target >= 0) {
//    memcpy(allowedDevices[target].mac, mac, 6);
//    strncpy(allowedDevices[target].desc, desc, sizeof(allowedDevices[target].desc)-1);
//    Serial.printf("Added MAC to slot %d (%s)\r\n", target, allowedDevices[target].desc);
//  } else {
//    Serial.println("No empty slots available!");
//  }
//}
//
//// Delete MAC by slot number
///*****************************************************************************/
//void deleteMAC(int slot) {
///*****************************************************************************/
//  if (slot < 0 || slot >= MAX_MACs) {
//    Serial.println("Invalid slot number.");
//    return;
//  }
//  memset(allowedDevices[slot].mac, 0x00, 6);
//  strncpy(allowedDevices[slot].desc, "None Registered", sizeof(allowedDevices[slot].desc)-1);
//  Serial.printf("Deleted slot %d\r\n", slot);
//}
//
//// Parse serial input
///*****************************************************************************/
//void handleSerialCommand() {
///*****************************************************************************/
//  if (Serial.available()) {
//    String cmd = Serial.readStringUntil('\n');
//    cmd.trim();
//
//    if (cmd.startsWith("add")) {
//      // Example: add 4C:11:AE:A6:9F:34 T-Display
//      int spaceIdx = cmd.indexOf(' ');
//      if (spaceIdx > 0) {
//        String macStr = cmd.substring(spaceIdx+1, spaceIdx+18);
//        String desc   = cmd.substring(spaceIdx+19);
//
//        uint8_t mac[6];
//        sscanf(macStr.c_str(), "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
//               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
//
//        addMAC(mac, desc.c_str());
//      }
//    } else if (cmd.startsWith("del")) {
//      // Example: del 3
//      int slot = cmd.substring(4).toInt();
//      deleteMAC(slot);
//    } else {
//      Serial.println("Unknown command. Use 'add <MAC> <desc>' or 'del <slot>'.");
//    }
//  }
//}
// Print the current allowedDevices table
///*****************************************************************************/
//void listMACs() {
///*****************************************************************************/
//  Serial.println("------------------------------------------------------------");
//  Serial.println(" Slot |        MAC Address        | Description");
//  Serial.println("------------------------------------------------------------");
//  for (int i = 0; i < MAX_MACs; i++) {
//    Serial.printf("  %d   | ", i);
//    for (int j = 0; j < 6; j++) {
//      Serial.printf("%02X", allowedDevices[i].mac[j]);
//      if (j < 5) Serial.print(":");
//    }
//    Serial.printf(" | %s\r\n", allowedDevices[i].desc);
//  }
//  Serial.println("------------------------------------------------------------");
//}
//
//// Clear all slots
///*****************************************************************************/
//void clearAllMACs() {
///*****************************************************************************/
//  for (int i = 0; i < MAX_MACs; i++) {
//    memset(allowedDevices[i].mac, 0x00, 6);
//    strncpy(allowedDevices[i].desc, "None Registered", sizeof(allowedDevices[i].desc)-1);
//  }
//  Serial.println("All slots cleared.");
//}
//
//// Extend serial command handler
///*****************************************************************************/
//void handleSerialCommand() {
///*****************************************************************************/
//  if (Serial.available()) {
//    String cmd = Serial.readStringUntil('\n');
//    cmd.trim();
//
//    if (cmd.startsWith("add")) {
//      // Example: add 4C:11:AE:A6:9F:34 T-Display
//      int spaceIdx = cmd.indexOf(' ');
//      if (spaceIdx > 0) {
//        String macStr = cmd.substring(spaceIdx+1, spaceIdx+18);
//        String desc   = cmd.substring(spaceIdx+19);
//
//        uint8_t mac[6];
//        sscanf(macStr.c_str(), "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
//               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
//
//        addMAC(mac, desc.c_str());
//      }
//
//    } else if (cmd.startsWith("del")) {
//      // Example: del 3
//      int slot = cmd.substring(4).toInt();
//      deleteMAC(slot);
//
//    } else if (cmd.equalsIgnoreCase("list")) {
//      listMACs();
//
//    } else if (cmd.equalsIgnoreCase("clear")) {
//      clearAllMACs();
//
//    } else {
//      Serial.println("Commands: add <MAC> <desc>, del <slot>, list, clear");
//    }
//  }
//}
