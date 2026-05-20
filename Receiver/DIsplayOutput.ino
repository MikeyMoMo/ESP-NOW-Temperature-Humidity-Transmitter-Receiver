#include "Definitions.h"  // Should not be needed but this solves a problem.
/*****************************************************************************/
void Update_MSP2008_Display()
/*****************************************************************************/
{
  //  lowOutTempSlot = getOutLowTempSlot();

  //  Setup Headers

  if (useSprite)
    drawRects(spr);
  else
    drawRects(tft);

  if (!safeLoadFont("EldoradoRomanTTF", EldoradoRomanTTF, __LINE__))
    while (1);
  ofr.setFontSize(34);

  // The following loop really is overkill.  The preceeding 6 statements take
  //  care of it quite nicely and more plainly but, CoPilot suggested this
  //  way for larger enums and I wanted to enshrine it here so I could
  //  look back on it at some future time and see how it all fit together.
  //  Pretty cool code, really.  If the enum is expanded, it automatically
  //  expands the loop.
  for (int loc = 0; loc < ENUM_COUNT; loc++) {
    int x = (loc == INSIDE) ? tft.width() * .75 : tft.width() * .25;
    int color = (loc == INSIDE) ? InsideRed : OutsideBlue;

    ofr.setCursor(x, 10);
    ofr.setFontColor(TFT_BLACK, color);
    ofr.cprintf(localesL[loc]);
  }

  // Draw lines on the first and last pixel columns.
  //  tft.drawFastVLine(0, 0, tft.height(), TFT_BLACK);
  //  tft.drawFastVLine / (tft.width() - 1, 0,
  //                           tft.height(), TFT_BLACK);

  // Put in the readings

  // Sender -- OUTSIDE;

  //  printSensorTable();
  printSlotsDashboard();
  ofr.setFontColor(TFT_BLACK, OutsideBlue);
  lowOutTempSlot = getOutLowTempSlot();
  addVectorReading(workTime, slots[lowOutTempSlot].xferData.myTempC);
  //  Serial.printf("In DisplayOutput, lowOutTempSlot %i\r\n", lowOutTempSlot);
  if (lowOutTempSlot < 0) {
    if (!safeLoadFont("EldoradoBoldTTF", EldoradoBoldTTF, __LINE__))
      while (1);
    ofr.setFontSize(42);
    //    tbw = ofr.getTextWidth(na);
    //    tbh = ofr.getTextHeight(na);
    ofr.setCursor(tft.width() * .25, tempY);
    ofr.cprintf(na);

    if (!safeLoadFont("EldoradoRomanTTF", EldoradoRomanTTF, __LINE__))
      while (1);
    ofr.setFontSize(34);
    ofr.setCursor(tft.width() * .25, RHY);
    ofr.cprintf(na);

  } else {  // Got a good looking temp, print it.

    if (!safeLoadFont("EldoradoBoldTTF", EldoradoBoldTTF, __LINE__))
      while (1);
    ofr.setFontSize(45);
    snprintf(cCharWork, sizeof(cCharWork), "%.1f°%s",
             convertTemp(slots[lowOutTempSlot].xferData.myTempC / 100.),
             tempType);
    //    tbw = ofr.getTextWidth(cCharWork);
    //    tbh = ofr.getTextHeight(cCharWork);
    ofr.setCursor(tft.width() / 4, tempY);
    ofr.cprintf(cCharWork);

    if (!safeLoadFont("EldoradoRomanTTF", EldoradoRomanTTF, __LINE__))
      while (1);
    ofr.setFontSize(34);
    ofr.setCursor(tft.width() *.25, RHY);
    if (slots[lowOutTempSlot].xferData.myRH / 100. >= 99.5)
      ofr.cprintf("RH 100%%");
    else
      ofr.cprintf("RH %2.1f%%",
                  slots[lowOutTempSlot].xferData.myRH / 100.);

  }
  //----------------------------------------------------

  // Sender = INSIDE;

  ofr.setFontColor(TFT_BLACK, InsideRed);
  if (slots[INSIDE_SLOT].xferData.myTempC > BAD_TEMP_HI_CHECK) {
    if (!safeLoadFont("EldoradoBoldTTF", EldoradoBoldTTF, __LINE__))
      while (1);
    ofr.setFontSize(42);
    //    tbw = ofr.getTextWidth(na);
    //    tbh = ofr.getTextHeight(na);
    ofr.setCursor(tft.width() * .75, tempY);
    ofr.cprintf(na);

    if (!safeLoadFont("EldoradoRomanTTF", EldoradoRomanTTF, __LINE__))
      while (1);
    ofr.setFontSize(34);
    //    tbw = ofr.getTextWidth(na);
    //    tbh = ofr.getTextHeight(na);
    ofr.setCursor(tft.width() * .75, RHY);
    ofr.cprintf(na);

  } else {  // Got a good looking temp, print it.

    if (!safeLoadFont("EldoradoBoldTTF", EldoradoBoldTTF, __LINE__))
      while (1);
    ofr.setFontSize(45);
    snprintf(cCharWork, sizeof(cCharWork), "%.1f°%s",
             convertTemp(slots[INSIDE_SLOT].xferData.myTempC / 100.),
             tempType);
    //    tbw = ofr.getTextWidth(cCharWork);
    //    tbh = ofr.getTextHeight(cCharWork);
    ofr.setCursor(tft.width() *.75, tempY);
    ofr.cprintf(cCharWork);

    if (!safeLoadFont("EldoradoRomanTTF", EldoradoRomanTTF, __LINE__))
      while (1);
    ofr.setFontSize(34);
    ofr.setCursor(tft.width() *.75, RHY);
    if (slots[INSIDE_SLOT].xferData.myRH / 100. >= 99.5)
      ofr.cprintf("RH 100%%");
    else
      ofr.cprintf("RH %3.1f%%",
                  slots[INSIDE_SLOT].xferData.myRH / 100.);
  }

  ofr.setFontColor(TFT_BLACK, HiLoGreen);
  // Add high and low for the last 24 hours to the display.
  //  snprintf(cCharWork, sizeof(cCharWork), "24 Hr Low/High/DP");
  ofr.setCursor(tft.width() / 2, infoLine[4] + 20);
  ofr.cprintf("24 Hr Low/High/DP");

  lowOutTempSlot = getOutLowTempSlot();
  if (lowOutTempSlot < 0)
    Serial.println("No valid low temp yet.");
  else {
    DPtempC = slots[lowOutTempSlot].xferData.myTempC / 100.0;
    DPRH    = slots[lowOutTempSlot].xferData.myRH / 100.0;
    // RH must be divided by 100 again to get fraction (0–1)
    rhFraction = DPRH / 100.0;
    lamda = log(rhFraction) +
            ((17.625 * DPtempC) / (243.04 + DPtempC));
    denom = 17.625 - lamda;
    if (fabs(denom) < DPTol) {
      Serial.println("No division by 0, please.");
    } else {
      dewPoint = (243.04 * lamda) / denom;
    }
  }
  // If there are no readings or there is a hitch in the getalong getting
  //  high and low temps, display n/a for all readings.
  if (outReadings.empty()) {
    snprintf(cCharWork, sizeof(cCharWork), "%s / %s / %s", na, na, na);
  } else {
    get24HourHiLo();  // min24Temp, max24Temp
    snprintf(cCharWork, sizeof(cCharWork), "%.1f° / %.1f° / %.1f°",
             convertTemp(float(min24Temp) / 100.),
             convertTemp(float(max24Temp) / 100.),
             convertTemp(dewPoint));
  }
  ofr.setCursor((tft.width() / 2), infoLine[5] + 20);
  ofr.cprintf(cCharWork);

  if (useSprite)
    spr.pushSprite(0, 0);
  intervalReceives[INSIDE]  = false;
  intervalReceives[OUTSIDE] = false;
}
/*****************************************************************************/
float convertTemp(float temp)  // Convert... or not!
/*****************************************************************************/
{
#ifdef USE_F_TEMPS
  return temp * 1.8 + 32.;
#else
  return temp;
#endif
}
// Define a helper function
/*****************************************************************************/
void drawRects(TFT_eSPI & where)
/*****************************************************************************/
{
  // Too wide to fix a library bug.
  where.fillRect(0, 0, where.width() / 2 + 3, 160, OutsideBlue);  // Light Blue
  where.fillRect(where.width() / 2 + 1, 0, where.width() - 1, 160,
                 InsideRed);  // Light Red
  where.fillRect(0, 161, where.width(), where.height(),
                 HiLoGreen);  // Light Green

  where.drawFastVLine(where.width() / 2, 0, 160, TFT_BLACK);
  where.drawFastHLine(0, 160, where.width(), TFT_BLACK);
}
