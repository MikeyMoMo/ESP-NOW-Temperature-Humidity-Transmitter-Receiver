/***************************************************************************/
// Compute CRC8 using polynomial 0x07
uint8_t crc8(const uint8_t *data, size_t len)
/***************************************************************************/
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
/***************************************************************************/
void deduceOffsets()
/***************************************************************************/
{
  // Only update the TZ hourly to get the current offset and DST setting.
  // That's really more than necessary but not such a big deal.
  // I was doing it every second.  However, it only takes 32 ms., so...
  // No biggy.
  // It totally saves me from having to figure out DST for myself.
  // I did it once and it was NOT pretty and only applied to the U.S.!
  // I put in the time env string and activate it then see what conditions
  //  it creates. It gives me complete time, the time type (standard or DST)
  //  and the seconds offset. SO much easier than trying to do all of this
  //  myself.  It takes care of changing the time from DST to standard time
  //  and I just use that number.  Sweet!
  //  I just have to be sure that the time is valid and I do that by being
  //  sure that the computed year is less than or equal to my
  //  birth year (2024).  (The program, not me!)

  //  Serial.printf("Today is Julian day %i\r\n",
  //                calcJulian(iYear, iMonth, iDay));

  //  iStartMillis = millis();

#ifdef SHOW_DETAILS
  Serial.print(cityName);
#endif
  // Anybody but me see how silly that 1 is?
  setenv("TZ", LocalTZ, 1); tzset();
  time(&workTime);
#ifdef SHOW_DETAILS
  Serial.print(" "); Serial.print(ctime(&workTime));
#endif
  strftime (cCharWork, sizeof(cCharWork), "%Y", localtime(&workTime));
  iYear = atoi(cCharWork);
  strftime (cCharWork, sizeof(cCharWork), "%m", localtime(&workTime));
  iMonth = atoi(cCharWork);
  strftime (cCharWork, sizeof(cCharWork), "%d", localtime(&workTime));
  iDay = atoi(cCharWork);
#ifdef SHOW_DETAILS
  //  Serial.println(localtime(&workTime),
  //                 " initial set %a, %d-%m-%Y %T %Z %z");
#endif
  while (iYear < 2024) {
    time(&workTime);
    strftime (cCharWork, sizeof(cCharWork), " %Y", localtime(&workTime));
    iYear = atoi(cCharWork);
#ifdef SHOW_DETAILS
    Serial.print(cityName);
    Serial.println(localtime(&workTime),
                   " waiting %a, %d-%m-%Y %T %Z %z");
    delay(1000);
#endif
  }
#ifdef SHOW_DETAILS
  //  Serial.println(localtime(&workTime),
  //                 " after waiting %a, %d-%m-%Y %T %Z %z");
  //  Serial.print(localtime(&workTime), " %a, %d-%m-%Y %T %Z %z");
#endif

  strftime (cDST, 10, " %Z", localtime(&workTime));
  strftime (cCharWork, sizeof(cCharWork), " %z", localtime(&workTime));
  iTempOffset = atoi(cCharWork);
  iHomeOffset = (iTempOffset / 100) * 3600 + iTempOffset % 100 * 60;
#ifdef SHOW_DETAILS
  Serial.printf(" offset = %+i\r\n", iHomeOffset);
#endif
  // This must be done and must be last.  The local time is
  //  based off of this. They are not kept separately, only created
  //  when needed by adding the offset to UTC.
#ifdef SHOW_DETAILS
  Serial.print("UTC");
#endif
  setenv("TZ", cZulu, 1); tzset();
  strftime (cCharWork, sizeof(cCharWork), " %Y", localtime(&workTime));
  time(&UTC);
  iYear = atoi(cCharWork);
#ifdef SHOW_DETAILS
  //  Serial.println(localtime(&UTC),
  //                 "Zulu initial set %a, %d-%m-%Y %T %Z %z");
#endif
  while (iYear < 2024) {
    time(&UTC);
    strftime (cCharWork, sizeof(cCharWork), "%Y", localtime(&UTC));
    iYear = atoi(cCharWork);
#ifdef SHOW_DETAILS
    Serial.println(localtime(&UTC), "cZulu waiting %a, %d-%m-%Y %T %Z %z");
    delay(1000);
#endif
  }
#ifdef SHOW_DETAILS
  //  Serial.println(localtime(&UTC),
  //                 " after waiting %a, %d-%m-%Y %T %Z %z");
  Serial.println(localtime(&UTC), " %a, %d-%m-%Y %T %Z %z");
#endif

  setenv("TZ", LocalTZ, 1); tzset();
}
/****************************************************************************/
void updateTime()  // This has to be before OnDataRecv.  Don't know why!
/****************************************************************************/
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
    deduceOffsets();
    iPrevHour = iCurrHour;
  }
}
/****************************************************************************/
// Add a new reading and purge anything older than 24 hours
void addReading(unsigned long ts, float temp)
/****************************************************************************/
{
  readings.push_back({ts, temp});

  // Purge entries older than 24 hours (86,400 seconds)
  while (!readings.empty() &&
         (ts - readings.front().timestamp > DAY_SECONDS)) {
    readings.erase(readings.begin());
#ifdef SHOW_DETAILS
    Serial.print("One reading erased. ");
#endif
  }  // Don't let this get in the #ifdef!  All heck will break loose. ;-))
#ifdef SHOW_DETAILS
  Serial.printf("Size of readings vector: %i\r\n", readings.size());
#endif
}
/****************************************************************************/
// Print the vector contents
void printVector()
/****************************************************************************/
{
  Serial.printf("Capacity %i | Elements %i\r\n",
                readings.capacity(), readings.size());
  for (auto &r : readings) {
    Serial.printf("[time=%lu, temp=%3.2f]\r\n", r.timestamp, r.temperature);
  }
  Serial.println();
}
/***************************************************************************/
void showFields(int sender)
/***************************************************************************/
{
  Serial.printf("Sender             %s\r\n", localesL[sender]);

  Serial.print( "Packet Version:    ");
  Serial.println(transferData.version);

  Serial.print("Packet Sequence:   ");
  Serial.println(transferData.pktSeq);

  Serial.print("Temperature:       ");
  Serial.println(transferData.myTempC);

  Serial.print("Relative Humidity: ");
  Serial.println(transferData.myRH);

  Serial.print("Good Sends:        ");
  Serial.println(transferData.goodSendsCumulative);

  Serial.print("Bad Sends:         ");
  Serial.println(transferData.badSendsCumulative);

  Serial.print("Packet CRC8:       ");
  Serial.print(transferData.crc8); Serial.println("  -------");
}
/***************************************************************************/
void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *transferDataBytes, int len)
/***************************************************************************/
{
  OnDataRecv_Running = true;

  updateTime();

  if (millis() > epochData[OUTSIDE] + staleDelay) {
    if (temperature[OUTSIDE] < 1000.)
    {
      temperature[OUTSIDE] = 1000.; RH[OUTSIDE] = 0.;
      Serial.print("Cleared stale data for ");
      Serial.println(localesL[OUTSIDE]);
    }
  }
  if (millis() > epochData[INSIDE] + staleDelay) {
    if (temperature[INSIDE] < 1000.)
    {
      temperature[INSIDE] = 1000.; RH[INSIDE] = 0.;
      Serial.print("Cleared stale data for ");
      Serial.println(localesL[INSIDE]);
    }
  }
  memcpy(&transferData, transferDataBytes, sizeof(transferData));

  // If the packet layout changes, the version number will change and
  //  additional code decoding will have to be done.  Leave the old code
  //  in there, if possible, to handle older transmitter versions.
  if (transferData.version == packetVersion) {
    // The sender's MAC in printable and comparable format:
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);

#ifdef SHOW_DETAILS
    Serial.printf("Temp %3.2f, RH %3.2f ", transferData.myTempC,
                  transferData.myRH);
#endif

    // Now we do all the right stuff to trap the input.

    if (!strcmp(macStr, insideMAC))   // Is this from INSIDE sender?
      senderType = INSIDE;  // Start of INSIDE code.
    else
      senderType = OUTSIDE;  // There can be multiple of these.

    intervalReceives[senderType]++;
    temperature[senderType] = float(transferData.myTempC) / 100.;
#ifdef CONFIG_FOR_JOE
    temperature[senderType] = temperature[senderType] * 1.8 + 32.;
#endif
    RH[senderType] = float(transferData.myRH) / 100.;
    epochData[senderType] = millis();
    localCRC8 = crc8((uint8_t*)&transferData,
                     sizeof(transferData) - 1);
    if (localCRC8 != transferData.crc8) {
      Serial.printf("Locally calculated CRC8 for sender %i "
                    "does not match the received data.\r\n", senderType);
      OnDataRecv_Running = false;
      return;
    }
    if (transferData.pktSeq == prevPktSeq[senderType]) {
      Serial.printf("Duplicate packet (%i) for sender %i. Packet ignored.\r\n",
                    transferData.pktSeq, senderType);
      OnDataRecv_Running = false;
      return;
    }
    // See how many are missing.
    if (transferData.pktSeq > prevPktSeq[senderType] + 1) {
      missedPackets[senderType] +=
        (transferData.pktSeq - prevPktSeq[senderType] - 1);
    }

    prevPktSeq[senderType] = transferData.pktSeq;

    if (senderType == OUTSIDE) {
      // Update the Vector and remove old entries.
      time(&workTime);
      addReading(workTime, transferData.myTempC);
      // printVector();
      if (transferData.myTempC < currentOutLowest)
        currentOutLowest = transferData.myTempC;
    }

#ifdef SHOW_DETAILS
    Serial.printf(" (%s) -- ", localesS[senderType]);
    showFields(senderType);  // For debugging.  //
    // End of OUTSIDE code.

    Serial.printf("Received on %02i/%02i/%02i at %02i:%02i:%02i",
                  iCurrMonth, iCurrDay, iCurrYear,
                  iCurrHour, iCurrMinute, iCurrSecond);
    Serial.print(" from MAC : ");
    Serial.println(macStr);
    Serial.println("----------------------");
#endif
  } else {
    Serial.printf("Unknown data packet version: %i\r\n",
                  transferData.version);
  }
  OnDataRecv_Running = false;
}
/***************************************************************************/
void timeSyncCallback(struct timeval * tv) // Must be before initTime
/***************************************************************************/
{
  //  struct timeval {  // Instantiated as "*tv"
  // Number of whole seconds of elapsed time
  //   time_t      tv_sec;
  // Number of microseconds of rest of elapsed time minus tv_sec.
  //                             Always less than one million
  //   long int    tv_usec;
  //};
  Serial.println("\n----- Time Sync Received -----");
  Serial.printf("Time sync at %u ms. UTC Epoch : ", millis());
  Serial.print(tv->tv_sec); Serial.print(" - ");
  Serial.println(ctime(&tv->tv_sec));
  delay(100);
}
/****************************************************************************/
void initTime()
/****************************************************************************/
{
  sntp_set_sync_interval(86400000);  // 1 day in ms.
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", LocalTZ, 1); tzset();
  Serial.println("\r\nWaiting for correct time...");

  //  Serial.printf("Incoming epoch : %lu\r\n", workTime);
  strftime(cCharWork, sizeof(cCharWork), "%Y", localtime(&workTime));
  iYear = atoi(cCharWork);
  //  Serial.printf("0 iYear %i\r\n", iYear);
  int iLooper = 0;
  while (iYear < 2024) {
    time(&workTime);
    strftime (cCharWork, 100, "%Y", localtime(&workTime));
    iYear = atoi(cCharWork);
    //    Serial.printf("1 iYear %i\r\n", iYear);
    Serial.println(localtime(&workTime), "UTC %A %m/%d/%Y %T");
    if (iLooper++ > 60) {
      Serial.println("Cannot get time set. Rebooting.");
      ESP.restart();
    }
    delay(2000);
  }
  Serial.println("Get zone offsets");
  //  Serial.println("\r\nDetermining zone offsets.");
  deduceOffsets();
  time(&UTC);
  workTime = UTC + iHomeOffset;
}
/****************************************************************************/
// Find minimum temperature in the current 24‑hour window
float getMinTemp()
/****************************************************************************/
{
  // empty() status is checked before the call.
  //  if (readings.empty()) return NAN; // handle empty case
  float minVal = readings[0].temperature;
  for (auto &r : readings) {
    if (r.temperature < minVal) minVal = r.temperature;
  }
#ifdef CONFIG_FOR_JOE
  return minVal * 1.8 + 32.;
#else
  return minVal;
#endif
}
/****************************************************************************/
// Find maximum temperature in the current 24‑hour window
float getMaxTemp()
/****************************************************************************/
{
  // empty() status is checked before the call.
  //  if (readings.empty()) return NAN; // handle empty case
  float maxVal = readings[0].temperature;
  for (auto &r : readings) {
    if (r.temperature > maxVal) maxVal = r.temperature;
  }
#ifdef CONFIG_FOR_JOE
  return maxVal * 1.8 + 32.;
#else
  return maxVal;
#endif
}
/****************************************************************************/
void Update_T5_Display()
/****************************************************************************/
{
  // Screen pixels 250x122 but it still looks good!
  
  //  Setup Headers

  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  //  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  //  display.getTextBounds(sTL, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  //  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  //  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  int pageCt = 1;
  display.firstPage();
  display.fillScreen(GxEPD_WHITE);

  display.getTextBounds(localesL[OUTSIDE], 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(display.width() / 4 - tbw / 2, 10);
  display.print(localesL[OUTSIDE]);
  display.getTextBounds(localesL[INSIDE], 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(display.width() / 4 * 3 - tbw / 2, 10);
  display.print(localesL[INSIDE]);
  display.drawFastVLine(display.width() / 2, 0, display.height()*.75,
                        GxEPD_BLACK);
  // Draw lines on the first and last pixel columns.
  //  display.drawFastVLine(0, 0, display.height(), GxEPD_BLACK);
  //  display.drawFastVLine / (display.width() - 1, 0,
  //                           display.height(), GxEPD_BLACK);

  // Put in the readings

  senderType = OUTSIDE;
  x = 0; y = 52;
  /* Testing */
  //  temperature[senderType] = 300.;

  //  display.setFont(&FreeSerifBold18pt7b);
  if (temperature[senderType] > 200.) {
    display.setFont(&FreeSerifBold18pt7b);
    display.getTextBounds("n/a", 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(display.width() / 4 - (tbw / 2), y);
    display.printf("n/a");
    display.setFont(&FreeSerifBold12pt7b);
    x = 10; y = 85;
    display.getTextBounds("RH n/a", 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(display.width() / 4 - (tbw / 2), y);
    display.print("RH n/a");
  } else {
    display.setFont(&FreeSerifBold18pt7b);

    if (temperature[senderType] >= 100.)
      display.setCursor(x, y);
    if (temperature[senderType] < 100. && temperature[senderType] >= 10.)
      display.setCursor(x + 20, y);
    else if (temperature[senderType] < 10. && temperature[senderType] >= 0.)
      display.setCursor(x + 36, y);
    else if (temperature[senderType] <= 0. && temperature[senderType] > 10.)
      display.setCursor(x + 20, y);
    else if (temperature[senderType] < 10.)
      display.setCursor(x, y);

    display.printf("%.1f", temperature[senderType]);
    display.drawCircle(x + 87, y - 20, 4, GxEPD_BLACK);
    display.drawCircle(x + 87, y - 20, 3, GxEPD_BLACK);
    display.setCursor(x + 93, y);
#ifdef CONFIG_FOR_JOE
    display.print("F");
#else
    display.print("C");
#endif
    display.setFont(&FreeSerifBold12pt7b);
    x = 10; y = 85;
    display.setCursor(x, y);
    if (RH[senderType] >= 99.5)
      display.printf("RH %3.0f%%", RH[senderType]);
    else
      display.printf("RH %3.1f%%", RH[senderType]);
  }

  senderType = INSIDE;
  /* Testing */
  //  temperature[senderType] = 300.;

  x = 125; y = 52;
  //  display.setFont(&FreeSerifBold18pt7b);
  if (temperature[senderType] > 200.) {
    display.setFont(&FreeSerifBold18pt7b);
    display.getTextBounds(localesL[INSIDE], 0, 0, &tbx, &tby, &tbw, &tbh);

    display.getTextBounds("n/a", 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(display.width() / 4 * 3 - (tbw / 2), y);
    display.setCursor(display.width() / 4 * 3 - (tbw / 2), y);
    display.printf("n/a");
    display.setFont(&FreeSerifBold12pt7b);
    x = 135; y = 85;
    display.getTextBounds("RH n/a", 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(display.width() / 4 * 3 - (tbw / 2), y);
    display.print("RH n/a");
  } else {
    display.setFont(&FreeSerifBold18pt7b);

    if (temperature[senderType] >= 100.)
      display.setCursor(x, y);
    if (temperature[senderType] < 100. && temperature[senderType] >= 10.)
      display.setCursor(x + 20, y);
    else if (temperature[senderType] < 10. && temperature[senderType] >= 0.)
      display.setCursor(x + 36, y);
    else if (temperature[senderType] <= 0. && temperature[senderType] > 10.)
      display.setCursor(x + 20, y);
    else if (temperature[senderType] < 10.)
      display.setCursor(x, y);

    display.printf("%3.1f", temperature[senderType]);
    display.drawCircle(x + 87, y - 20, 4, GxEPD_BLACK);
    display.drawCircle(x + 87, y - 20, 3, GxEPD_BLACK);
    display.setCursor(x + 93, y);
#ifdef CONFIG_FOR_JOE
    display.print("F");
#else
    display.print("C");
#endif

    display.setFont(&FreeSerifBold12pt7b);
    x = 135; y = 85;
    display.setCursor(x, y);
    if (RH[senderType] >= 99.5)
      display.printf("RH %3.0f%%", RH[senderType]);
    else
      display.printf("RH %3.1f%%", RH[senderType]);
  }

  // Add high and low for the last 24 hours to the display.
  display.setCursor(x, y);
  snprintf(cCharWork, sizeof(cCharWork), "24Hr L/H %.1f/%.1f",
           getMinTemp() / 100., getMaxTemp() / 100.);
  display.getTextBounds(cCharWork, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor((display.width() / 2) - (tbw / 2), 115);
  display.print(cCharWork);

  //  }
  //  while (display.nextPage());
  display.display();
  currentOutLowest = 1000.; // Start the lowest outside temp cycle again.
}
