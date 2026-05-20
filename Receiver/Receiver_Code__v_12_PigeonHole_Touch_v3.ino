//#define CONFIG_FOR_JOE
//#define USE_F_TEMPS  // If commented, uses °C.
#if TEMP_CELSIUS
const char degree_symbol[] = "\u00B0F";
#else
const char degree_symbol[] = "\u00B0C";
#endif
//#define Config_for_Joe
#include <Definitions.h>
#include <Prototypes.h>
#include <Utilities.h>
// 1,807 lines of code & comments as of 2/22/25.
/*****************************************************************************/
void setup()
/*****************************************************************************/
{
  Serial.begin(115200); delay(4000);
  Serial.println("This is ESP-NOW Temp/RH Receiver program running from:");
  Serial.println(__FILE__);

  /* Testing */
  Serial.printf("ENUM_COUNT=%d, INSIDE=%d, OUTSIDE=%d\n",
                ENUM_COUNT, INSIDE, OUTSIDE);
  /* End Testing */

  // Needed for render library to print messages.
  ofr.setSerial(Serial); delay(1000);

  Serial.println("\r\n--------------------------------\r\nStarting");
  //  ofr.showFreeTypeVersion();      // print FreeType version
  //  ofr.showCredit();               // print FTL credit
  ofr.getFreeTypeVersion(vers);
  // A different way where you get the text in hand
  Serial.print("OFR version: "); printWrapped(vers, 54);

  // Tell the world (library adds \n)
  ofr.getCredit(cred);            // This way, you get the text in hand
  Serial.print("OFR credit: "); printWrapped(cred, 54);

  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(3);

  tft.init();
  tft.setRotation(3);  // Power socket on bottom
  tft.fillScreen(TFT_WHITE);
  if (psramInit() && psramFound()) {  // Supposedly present
    useSprite = true;  // Assumed for now...
    if (spr.createSprite(tft.width(), tft.height()) == nullptr) {
      useSprite = false;
      Serial.println("PSRAM not available (OK if CYD).\r\n"
                     "Sprite not created. Using direct tft writes.");
    }
  } else {
    useSprite = false;
    Serial.println("PSRAM not available - check menu item (OK on CYD).\r\n"
                   "Sprite not created. Using direct tft writes.");
  }
  if (useSprite) {  // Announce Sprite success.  Hooray, better display.
    Serial.printf("Sprite created: %i x %i\r\n", spr.width(), spr.height());
    spr.fillSprite(TFT_WHITE);
    ofr.setDrawer(spr);             // Link ofr output to sprite.
  } else {  // Sadly, no PSRAM available.  Slightly worse looking display.
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextDatum(MC_DATUM);

    // Serial.println("Sprite not created. Will use direct tft writes.");
    tft.drawString("Sprite Not Created.\r\n",
                   tft.width() / 2, tft.height() / 2, 4);
    tft.drawString("(OK for CYD.)\r\n",
                   tft.width() / 2, tft.height() / 2 + 30, 4);
    delay(4000);  // Let the user see it, then move on without using sprite.
    tft.fillScreen(TFT_WHITE);
    // The single setDrawer call is the same as using the four calls under it.
    // Take your pick!
    //  ofr.setDrawer(spr);             // Link ofr output to sprite.
    //  ofr.setDrawPixel(tft.drawPixel);
    //  ofr.setDrawFastHLine(tft.drawFastHLine); // optional
    //  ofr.setStartWrite(tft.st.macartWrite);       // optional
    //  ofr.setEndWrite(tft.endWrite);           // optional
    ofr.setDrawer(tft);             // Link ofr output to base tft screen.
  }

#ifdef TFT_BL
  ledcAttach(TFT_BL, 5000, 8);  // PWM timer automatically assigned.
  ledcWrite(TFT_BL, 200);  // Turn the display on bigly for init messages.
#endif

  if (!safeLoadFont("EldoradoBoldTTF", EldoradoBoldTTF, __LINE__))
    while (1);
  ofr.setFontSize(42);

  //  tbw = ofr.getTextWidth(sIniting);
  //  tbh = ofr.getTextHeight(sIniting);
  Serial.printf("Display width %i, height %i\r\n",
                tft.width(), tft.height());
  if (useSprite)
    spr.fillSprite(TFT_WHITE);
  else
    tft.fillScreen(TFT_WHITE);

  ofr.setCursor(tft.width() / 2, infoLine[2]);
  ofr.setFontColor(TFT_BLACK, TFT_WHITE);
  // Foreground, Background (optional)
  ofr.cprintf(sIniting);
  ofr.setCursor(tft.width() / 2, infoLine[4]);
  ofr.cprintf("WiFi");
  if (useSprite)
    spr.pushSprite(0, 0);

  WiFi.mode(WIFI_STA);
  // pick a channel, same on both
  //  esp_wifi_set_channel(7, WIFI_SECOND_CHAN_NONE);
  Serial.print("Starting WiFi .");
  WiFi.begin(ssid, wifipw);
  int waitCt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if (waitCt++ > 20) {
      Serial.print("Cannot connect to WAP, rebooting.");
      delay(5000);
      ESP.restart();
    }
  }
  Serial.println(" WiFi Connected.");
  Serial.print("My (the Receiver's) MAC: ");
  Serial.println(WiFi.macAddress());

  if (useSprite)
    spr.fillSprite(TFT_WHITE);
  else
    tft.fillScreen(TFT_WHITE);

  ofr.setCursor(tft.width() / 2, infoLine[2]);
  ofr.setFontColor(TFT_BLACK, TFT_WHITE);
  // Foreground, Background (optional)
  ofr.cprintf(sIniting);
  ofr.setCursor(tft.width() / 2, infoLine[4]);
  ofr.cprintf("Time");
  if (useSprite)
    spr.pushSprite(0, 0);

  initTime();

  Serial.println("Trying to initialize ESP-NOW.");
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW initialized properly. Listening for data.");
  Serial.println("-----------------------");

  // Really only need 288 (12 * 24) per transmitter.
  // Being generous plus enough to handle two outside transmitters.
  // This is the size for 2 outside senders plus a little.
  outReadings.reserve(iExpectedSize);

  if (useSprite)
    spr.fillSprite(TFT_WHITE);
  else
    tft.fillScreen(TFT_WHITE);

  //  getTextBounds(WiFiConnected, 0, 0, &tbx, &tby, &tbw, &tbh);
  ofr.setCursor(tft.width() / 2, infoLine[1]);
  ofr.cprintf(WiFiConnected);  // Now says "Initialized."

  snprintf(cCharWork, sizeof(cCharWork), "Waiting up to");
  ofr.setCursor(tft.width() / 2, infoLine[3]);
  ofr.cprintf(cCharWork);

  snprintf(cCharWork, sizeof(cCharWork), "%i seconds.", sendTime);
  ofr.setCursor(tft.width() / 2, infoLine[5]);
  ofr.cprintf(cCharWork);
  if (useSprite)
    spr.pushSprite(0, 0);

  for (int i = 0; i < MAX_SENSORS; i++) {
    memset(slots[i].macAddr, 0, 6);
    slots[i].xferData = {};   // zero-initialize the whole payload struct
    slots[i].xferData.myTempC = BAD_TEMP_HI_SENTINEL;
    slots[i].lastUpdate = 0;
  }
  // Set bad sentinel.
  slots[INSIDE_SLOT].xferData.myTempC = BAD_TEMP_HI_SENTINEL;

  lastReportTime = millis();  // Start display update timer.

  pinMode(CYD_LED_RED,   OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE,  OUTPUT);

  Serial.printf("Current WiFi channel: %ld\r\n", WiFi.channel());
  Serial.println("-----------------------");

  // Take one real sensor reading
  startLDR = analogRead(34);

  sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = startLDR;
    sum += startLDR;
  }
  ringIndex = 0;

  esp_now_peer_info_t peerInfo = {};
  // Register all peers in the "friends" table: allowedDevices
  for (int i = 0; i < MAX_SENSORS; i++) {  // was: MAX_MACs
    if (memcmp(allowedDevices[i].MAC, zeroMac, 6) == 0) {
      continue;  // Only register used slots.
    } else {
      peerInfo = {};  // Clear out leftovers.
      // Register peer
      Serial.printf("Trying to register peer in slot %i... "
                    "%02X:%02X:%02X:%02X:%02X:%02X %s\r\n", i,
                    allowedDevices[i].MAC[0], allowedDevices[i].MAC[1],
                    allowedDevices[i].MAC[2], allowedDevices[i].MAC[3],
                    allowedDevices[i].MAC[4], allowedDevices[i].MAC[5],
                    allowedDevices[i].desc);

      // Copy receiver MAC address.
      memcpy(peerInfo.peer_addr, allowedDevices[i].MAC, 6);
      peerInfo.channel = 0;      // 0 means “use current WiFi channel”
      peerInfo.encrypt = false;  // Don't need no stinkin' 'cryption!
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
      } else {
        Serial.println("Peer registered.");
      }
    }
  }

  avgLdr = startLDR;
  pinnedBrightness = constrain(map(avgLdr, 1900, 875, 0, 255), 0, 255);
  prevPinnedBrightness = pinnedBrightness;
  prevBrightness = pinnedBrightness;

  // The LEDs are "active low", meaning HIGH == off, LOW == on
  // Turn off the RGB LED on the CYD board back.
  digitalWrite(CYD_LED_RED,   HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE,  HIGH);

  lastChangeTime = millis();

}
/*****************************************************************************/
void loop()
/*****************************************************************************/
{
  // Let the update happen if both remotes have reported or the timer is up

  if (ts.tirqTouched() && ts.touched()) {
    while (ts.touched())
      ts.readData(&tsX, &tsY, &tsZ);
    Serial.printf("Touch released at %i, %i with pressure %i\r\n",
                  tsX, tsY, tsZ);
    if (inBox(tsX, tsY, 0, 0, 2199, 2636)) {
      while (ts.touched());
      Serial.println("Blue box");
    }
    if (inBox(tsX, tsY, 2200, 0, 1700, 2636)) {
      while (ts.touched());
      Serial.println("Pink box");
    }
    if (inBox(tsX, tsY, 0, 2637, 3900, 1200)) {
      while (ts.touched());
      Serial.println("Green box");
    }
    //    TS_Point p = ts.getPoint();
    //    printTouchToSerial(p);
    delay(10);
  }

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');  // Read full line
    input.trim();  // Remove \r, spaces
    if (input.length() == 1)
    {
      char cmd = toupper(input[0]);
      switch (cmd)
      {
        case 'D': printSlotsDashboard(); break;
        case 'H': showHeapStatus(); break;
        case 'P': printSensorTable(); break;
        case 'T': Serial.println(localtime(&workTime)); break;
        case 'V': print24Vector(); break;
        default:
          showHelp(); break;
      }
    } else {
      Serial.println("Single characters only, please.");
      showHelp();
    }
  }

  updateTime();
  adjustBL();

  for (queued_slot = 0; queued_slot < MAX_SENSORS; queued_slot++) {
    if (slotIsUsed(queued_slot)) {
      // If the reading is too old, skip it.
//      Serial.printf("loop-Time check slot %i: millis now %lu, slot updated at %lu\r\n",
//                    queued_slot, millis(), slots[queued_slot].lastUpdate);
      if ((millis() - slots[queued_slot].lastUpdate) > STALE_TIMEOUT) {
        // Serial.print(localtime(&workTime));
        Serial.print(myTimeStamp(time(nullptr)));
        Serial.printf(" Sensor %i data is stale, ignoring.\r\n", queued_slot);
        // A sender slot is considered used if there is a MAC address in
        //  it.  If the MAC address is all 0s, the slot is considered to
        //  be free for use.  Here, a slot is being released because it
        //  has not send in a value in the STALE_TIMEOUT period.
        esp_now_send(slots[queued_slot].macAddr,
                     (uint8_t *)REBOOT_MSG, strlen(REBOOT_MSG));
        slots[queued_slot].lastUpdate = (uint32_t) - 1;
        memset(slots[queued_slot].macAddr, 0, 6);  // Release it for reuse.

      } else {  // process it.

        // Process this valid slot.

        if (arrivals[queued_slot].newPacket) {
          arrivals[queued_slot].newPacket = false;
          Serial.print(myTimeStamp(arrivals[queued_slot].arrivalTime));
          if (queued_slot == INSIDE_SLOT) {
            Serial.printf(" Packet from INSIDE sensor %i: ", queued_slot);
          } else {
            Serial.printf(" Packet from OUTSIDE sensor %i: ", queued_slot);
          }  // End: if (queued_slot == INSIDE_SLOT)
          // human-friendly name
          Serial.println(allowedDevices[queued_slot].desc);
        }  // End: if (newPacketReceived[queued_slot])
      }  // End: if ((millis() - slots[queued_slot].lastUpdate)
    }  // End: if (slotIsUsed(queued_slot))
  }  // End: for (queued_slot = 0;

  if (iIN_RANGE(iCurrHour, LowRateStart, LowRateEnd))
    sendTime = 3600;  // Update the display hourly during sleep time.
  else
    sendTime = 315;   // Update the display every 5 minutes during the day.
  reportInterval = sendTime * 1000;

  //  if (iPrevSecond != iCurrSecond) {
  //    iPrevSecond = iCurrSecond;
  //    Serial.print(millis()); Serial.print(" ");
  //    Serial.print(lastReportTime); Serial.print(" ");
  //    Serial.println(reportInterval);
  //  }

  if ((millis() > lastReportTime + reportInterval) ||
      (intervalReceives[INSIDE] && intervalReceives[OUTSIDE])) {
    lastReportTime = millis();  // Reset timer.
    Update_MSP2008_Display();
  }
}
