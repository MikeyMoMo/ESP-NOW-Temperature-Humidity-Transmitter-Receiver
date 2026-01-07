#define SHOW_DETAILS  // Print out the inside info for testing.

#include <Definitions.h>

#include <esp_now.h>
#include <WiFi.h>

#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht;

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme280; // I2C

#include <Utilities.h>

/***************************************************************************/
void setup()
/***************************************************************************/
{
  // This is left in in case an error message print is necessary.
  // Error message prints are not bracketed with the ifdef's.
  Serial.begin(115200); delay(100);
#ifdef SHOW_DETAILS
  delay(500);
  Serial.println("This is ESP-NOW Temp & RH Transmitter program "
                 "running from:");
  Serial.println(__FILE__);
#endif
  // Init WiFi in STA mode
  //  WiFi.enableLongRange(true);
  WiFi.mode(WIFI_STA);
  // WiFi does not need to be started unless time is needed.
  // Just set to station mode and that's it.  ESP-NOW does the rest.

  // Touchy, Feely time!

  if (rtcMagic != 0xDEADBEEF) {
    // First boot or memory garbage
    seqCounter = 0;
    goodSendsCumulative = 0;
    badSendsCumulative = 0;
    rtcMagic = 0xDEADBEEF;
  }

  ANT20_Present =  aht.begin(); delay(30);
  if (ANT20_Present)
    Serial.println("AHT10/20 found, using real data.");
  else {
    Serial.println("Could not find AHT.  Checking for others");
    BME280_Present = bme280.begin();
    if (!BME280_Present) BME280_Present = bme280.begin(0x76);
    if (!BME280_Present) BME280_Present = bme280.begin(0x77);
  }

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(onDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo = {};  // Clear out accidental junk.
  memcpy(peerInfo.peer_addr, targetMAC, 6);  // Copy MAC address.
  peerInfo.channel = 0;       // 0 means “use current WiFi channel”
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // The real thing(s).
  // If there is a sensor out there, read real data from the sensor.

  if (BME280_Present) {
#ifdef SHOW_DETAILS
    Serial.println("Found BME280.");
    Serial.print("Sending valid BME280 data -- ");
#endif
    workFloatT = bme280.readTemperature();
    workFloatH = bme280.readHumidity();
    transferData.myTempC = int32_t(workFloatT * 100.);
    transferData.myRH    = int32_t(workFloatH * 100.);
    //#ifdef SHOW_DETAILS
    //    Serial.print("BME280 read temp: "); Serial.println(workFloatT);
    //    Serial.print("BME280 read RH:   "); Serial.println(workFloatH);
    //    Serial.print("BME280 read temp * 100: ");
    //    Serial.println(transferData.myTempC);
    //    Serial.print("BME280 read RH * 100:   ");
    //    Serial.println(transferData.myRH);
    //#endif
  }
  else if (ANT20_Present) {
#ifdef SHOW_DETAILS
    Serial.println("Found ANT/AHT20.");
#endif
    sensors_event_t humidity, temp;
    // populate temp and humidity objects with fresh data
    aht.getEvent(&humidity, &temp);
    transferData.myTempC = int32_t(temp.temperature * 100.);
    transferData.myRH    = int32_t(humidity.relative_humidity * 100.);
#ifdef SHOW_DETAILS
    Serial.print("Sending valid AxT20 data -- ");
#endif
  } else {
    // Fake temperature reading.
    transferData.myTempC = random(5000, 6000);
    transferData.myRH =    random(2500, 7500);
#ifdef SHOW_DETAILS
    Serial.print("Sending fake data -- ");
#endif
  }
#ifdef SHOW_DETAILS
  Serial.printf("(Raw)Temp %i, RH %i\r\n",
                transferData.myTempC, transferData.myRH);
  Serial.printf("Temp %.2f, RH %.2f\r\n",
                float(transferData.myTempC) / 100., 
                float(transferData.myRH) / 100.);
#endif
  transferData.version = packetVersion;
  transferData.pktSeq = ++seqCounter;
  if (seqCounter > 288) {
    seqCounter = 0; 
    ESP.restart();  // Daily restart to clean out detritus.
  }
  transferData.goodSendsCumulative = goodSendsCumulative;
  transferData.badSendsCumulative  = badSendsCumulative;
  // Now, add the CRC8
  transferData.crc8 = crc8((uint8_t*)&transferData,
                           sizeof(transferData) - 1);
  // Send message
  esp_err_t result = esp_now_send(targetMAC,
                                  (uint8_t *)&transferData,
                                  sizeof(transferData));
  if (result != ESP_OK) {
    Serial.print("Error queuing esp_now_send in setup.");
    showSendError(result);
  }
#ifdef SHOW_DETAILS
  Serial.print("My MAC: "); Serial.println(WiFi.macAddress());
#endif
}
/***************************************************************************/
void loop()
/***************************************************************************/
{
  // Not used; device sleeps after sending
}
