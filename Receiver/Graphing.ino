#define MAX_POINTS 288   // 24h at 5-min intervals
float tempBuf[MAX_POINTS];
float humBuf[MAX_POINTS];
int head = 0;
/*****************************************************************************/
void drawGraph() 
/*****************************************************************************/
{
  tft.fillScreen(TFT_BLACK);

  // Axes
  tft.drawLine(20, 220, 300, 220, TFT_WHITE); // X axis
  tft.drawLine(20, 220, 20, 20, TFT_WHITE);   // Y axis

  // Scale factors (example: temp 0–40°C, humidity 0–100%)
  float tempScale = 200.0 / 40.0;
  float humScale  = 200.0 / 100.0;

  // Plot temperature (red)
  for (int i = 1; i < MAX_POINTS; i++) {
    int idx0 = (head + i - 1) % MAX_POINTS;
    int idx1 = (head + i) % MAX_POINTS;
    int x0 = 20 + (i - 1);
    int x1 = 20 + i;
    int y0 = 220 - (int)(tempBuf[idx0] * tempScale);
    int y1 = 220 - (int)(tempBuf[idx1] * tempScale);
    tft.drawLine(x0, y0, x1, y1, TFT_RED);
  }

  // Plot humidity (blue)
  for (int i = 1; i < MAX_POINTS; i++) {
    int idx0 = (head + i - 1) % MAX_POINTS;
    int idx1 = (head + i) % MAX_POINTS;
    int x0 = 20 + (i - 1);
    int x1 = 20 + i;
    int y0 = 220 - (int)(humBuf[idx0] * humScale);
    int y1 = 220 - (int)(humBuf[idx1] * humScale);
    tft.drawLine(x0, y0, x1, y1, TFT_BLUE);
  }
}
