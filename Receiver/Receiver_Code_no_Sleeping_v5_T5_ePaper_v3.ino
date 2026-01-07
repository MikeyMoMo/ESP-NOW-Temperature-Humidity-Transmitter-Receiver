//#define SHOW_DETAILS
//#define CONFIG_FOR_JOE

// T5 (caseless) MAC is 7C:9E:BD:FA:EF:7C
// T5 (w/case)   MAC is 7C:9E:BD:FA:F0:1C

//#define Config_for_Joe
#include <Definitions.h>
#include <Utilities.h>
/****************************************************************************/
void setup()
/****************************************************************************/
{
  Serial.begin(115200); delay(4000);
  Serial.println("This is ESP-NOW Basic Receiver program running from:");
  Serial.println(__FILE__);
  // The first parameter must match the Serial.begin speed. 0 to supperss all.
  // The second parameter is whether to include additional diagnostic output
  // The third parameter is the reset pulse length.  Varies by display type.
  // The fourth parameter, if false, disables use of default SPI settings.
  //  display.init(115200, true, 2, false);
  display.init(0, false, 2, false);  // No showing of display statuses.
  //  display.setRotation(1);  // Power socket on top
  display.setRotation(3);  // Power socket on bottom
  display.firstPage();
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSerifBold12pt7b);
  display.getTextBounds(sIniting, 0, 0, &tbx, &tby, &tbw, &tbh);
  Serial.printf("Display width %i, height %i\r\n",
                display.width(), display.height());
  display.setCursor((display.width() / 2) - (tbw / 2),
                    (display.height() / 4));
  display.print(sIniting);
  display.display();

  WiFi.mode(WIFI_STA);
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
  Serial.print("My (the Receiver) MAC: ");
  Serial.println(WiFi.macAddress());

  initTime();
  // WiFi.disconnect()  // Not desired or required.

  Serial.println("Trying to initialize ESP-NOW.");
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW initialized properly. Listening for data.");
  Serial.println("----------------------");

  // Really only need 288 (12 * 24) per transmitter.
  // Being generous plus enough to handle two outside transmitters.
  readings.reserve(iExpectedSize);  // This number is for 2 senders.

  display.fillScreen(GxEPD_WHITE);

  display.getTextBounds("WiFi Connected", 0, 0, &tbx, &tby, &tbw, &tbh);
  Serial.printf("Width %i, Height %i\r\n", display.width(), display.height());
  display.setCursor((display.width() / 2) - (tbw / 2),
                    (display.height() / 4));
  display.print("WiFi Connected");

  snprintf(cCharWork, sizeof(cCharWork), "Waiting up to");
  display.getTextBounds(cCharWork, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor((display.width() / 2) - (tbw / 2),
                    (display.height() / 4 * 2) + 10);
  display.print(cCharWork);

  snprintf(cCharWork, sizeof(cCharWork), "%i seconds.", sendTime);
  display.getTextBounds(cCharWork, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor((display.width() / 2) - (tbw / 2),
                    (display.height() / 4 * 3) + 20);
  display.print(cCharWork);
  display.display();
}
/****************************************************************************/
void loop()
/****************************************************************************/
{
  // Let the update happen if both remotes have reported or the timer is up
  //  and the receive callback code is not running.

  if (OnDataRecv_Running) return;

#ifdef CONFIG_FOR_JOE
  tempType = "F";
#else
  tempType = "C";
#endif

  if ((millis() > lastReportTime + reportInterval) ||
      (intervalReceives[INSIDE] && intervalReceives[OUTSIDE])) {
    lastReportTime = millis();  // Reset timer.

    if (iIN_RANGE(iCurrHour, LowRateStart, LowRateEnd))
      sendTime = 3600;  // Update the display hourly during sleep time.
    else
      sendTime = 300;   // Update the display every 5 minutes during the day.

    updateTime();

    Serial.printf("%02i/%02i/%02i %02i:%02i:%02i ",
                  iCurrMonth, iCurrDay, iCurrYear,
                  iCurrHour, iCurrMinute, iCurrSecond);

    // These are not in a loop because of minor formatting difference in
    //  the Serial.prints.  If more are added, they can be looped all
    //  except the last one that is slightly different.
    senderType = OUTSIDE;
    if (temperature[senderType] > 999.)
      Serial.printf("%s:------------------ / ", localesS[senderType]);
    else
      Serial.printf("%s:%3.2f°%s/%3.2f%% (%i) / ",
                    localesS[senderType], temperature[senderType],
                    tempType,
                    RH[senderType], intervalReceives[senderType]);
    intervalReceives[senderType] = 0;

    senderType = INSIDE;
    if (temperature[senderType] > 999.)
      Serial.printf("%s:------------------ ", localesS[senderType]);
    else
      Serial.printf("%s:%3.2f°%s/%3.2f%% (%i) ",
                    localesS[senderType], temperature[senderType],
                    tempType,
                    RH[senderType], intervalReceives[senderType]);
    intervalReceives[senderType] = 0;

    if (!readings.empty()) {
      Serial.printf("Low:%3.2fº%s ", getMinTemp() / 100., tempType);
      Serial.printf("High:%3.2fº%s ", getMaxTemp() / 100., tempType);
    }
    // Compute Dew Point.
    double lamda = log(RH[OUTSIDE] / 100.) +
                   ((17.625 * temperature[OUTSIDE]) /
                    (243.04 + temperature[OUTSIDE]));
    // Serial.printf("The lamda value is %.2f.", lamda);
    Serial.printf("DP:%3.2fº%s\r\n",
                  (243.04 * lamda) / (17.625 - lamda), tempType);

    Update_T5_Display();
  }
}
