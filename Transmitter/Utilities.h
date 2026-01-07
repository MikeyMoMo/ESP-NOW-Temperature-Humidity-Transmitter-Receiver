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
// Show detailed error when there is a problem.
void showSendError(esp_err_t result)
/***************************************************************************/
{
  switch (result) {
    // No retry needed; wait for callback result
    case ESP_OK: Serial.println("Send succeeded"); break;
    // Fatal → re‑init ESP‑NOW, then retry once
    case ESP_ERR_ESPNOW_NOT_INIT: Serial.println("ESPNOW is not initialized."); break;
    // Fatal → fix code/config; retry won’t help
    case ESP_ERR_ESPNOW_ARG: Serial.println("Invalid argument."); break;
    // Retry once; if persistent, reboot
    case ESP_ERR_ESPNOW_INTERNAL: Serial.println("Internal error."); break;
    // Retry after short delay; if repeated, backOff
    case ESP_ERR_ESPNOW_NO_MEM: Serial.println("Out of memory, resend after delay."); break;
    // Fatal → add peer before retry
    case ESP_ERR_ESPNOW_NOT_FOUND: Serial.println("Peer is not found."); break;
    // Fatal → check Wi‑Fi mode/channel
    case ESP_ERR_ESPNOW_IF: Serial.println("Wi-Fi interface mismatch."); break;
    // Fatal → align channels; retry won’t help
    case ESP_ERR_ESPNOW_CHAN: Serial.println("Wi-Fi channel mismatch."); break;
    default: Serial.printf("Unknown problem, code: 0x%X\n", result); break;
  }
}
/***************************************************************************/
// Callback when data is sent
void onDataSent(const esp_now_send_info_t *info,
                esp_now_send_status_t status)
/***************************************************************************/
{
#ifdef SHOW_DETAILS
  Serial.print("Send Status: ");
  //  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
#endif
  if (status == ESP_NOW_SEND_SUCCESS) {  // Good send.  Wait full time
    badSendsForReboot = 0; goodSendsCumulative++;
    baseSleep = sendTime;  // normal cycle
    Serial.println("Success");
    // Try to send every 5 minutes with a 10 second wiggle to try to avoid
    //  transmission collisions.
  }
  else
  {
    badSendsForReboot++; badSendsCumulative++;
    Serial.printf("Send has failed %i times in a row.\r\n", badSendsForReboot);
    if (badSendsForReboot <= 2) {
      backOff = (sendTime / 4) * (1 << (badSendsForReboot - 1));
      if (backOff > maxbackOff) backOff = maxbackOff;
      baseSleep = backOff;
      if (badSendsForReboot > badSendMax) ESP.restart();
    }
  }
  // After send attempt, go back to sleep
  // Try to send sooner with a 10 second wiggle to try to avoid
  //  transmission collisions.
  jitter = random(-10, 11);
  sleepFor = (baseSleep + jitter) * 1000000ULL;
#ifdef SHOW_DETAILS
  Serial.printf("Sleeping for %i seconds.\r\n", sleepFor / 1000000ULL);
#endif

// The transmitter does not know the time.  Can't backoff send frequency.
//  if (iIN_RANGE(iCurrHour, sleepStart, sleepEnd))
//    sleepFor = 3600;  // Update the display hourly during sleep time.

  esp_sleep_enable_timer_wakeup(sleepFor);
  esp_deep_sleep_start();
}
