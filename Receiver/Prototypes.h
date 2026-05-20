void    setup();
void    loop();
void    Update_MSP2008_Display();
float   convertTemp(float temp);
void    drawRects(TFT_eSPI & where);
void    OnDataRecv(const esp_now_recv_info_t *info,
                   const uint8_t *xferDataBytes, int len);
const   char* getDeviceDesc(const uint8_t mac[6]);
const   char* getDeviceDescPadded(const uint8_t mac[6]);
void    printSlotsDashboard();
void    adjustBL();
bool    slotIsEmpty(int i);
bool    slotIsUsed(int i);
void    printMAC(const uint8_t *mac);
int     findSlot(const uint8_t *mac);
uint8_t crc8(const uint8_t *data, size_t len);
void    updateTime();
void    addVectorReading(unsigned long ts, int temp);
void    print24Vector();
void    showFields(int sender);
void    timeSyncCallback(struct timeval * tv);
void    initTime();
void    printFontError(const char* fontName, int rc, const char* func, int line);
template <size_t N>
bool    safeLoadFont(const char* fontName, const uint8_t (&fontData)[N],
                     int line);
void    printSensorTable();
int     getOutLowTempSlot();
void    printWrapped(const char *str, int width);
void    showHeapStatus();
void    get24HourHiLo();
String  myTimeStamp(time_t t);
void    showHelp();
void    printTouchToSerial(TS_Point p);
void    printPeerInfo();
void    addPeer(const uint8_t *mac);
bool    inBox(int x, int y, int x1, int y1, int w, int h);

//void  getTextBounds(const String& text, int16_t x, int16_t y,
//                    int16_t *x1, int16_t *y1,
//                    uint16_t *w, uint16_t *h);
