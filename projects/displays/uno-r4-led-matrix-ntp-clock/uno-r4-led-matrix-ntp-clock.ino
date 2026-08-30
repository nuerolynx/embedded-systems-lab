#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "arduino_secrets.h"

#include "RTC.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// ====== WiFi credentials ======
// Wi-Fi values are loaded from the ignored arduino_secrets.h file.

// ====== NTP ======
WiFiUDP udp;
NTPClient ntp(udp, "pool.ntp.org", 0 /*UTC*/, 60UL * 60UL * 1000UL);

// Resync every 6 hours
const unsigned long RESYNC_MS = 6UL * 60UL * 60UL * 1000UL;

// Display refresh
const unsigned long DISPLAY_MS = 200;

// Try WiFi connect every 10 seconds (non-blocking)
const unsigned long WIFI_RETRY_MS = 10UL * 1000UL;

bool timeSynced = false;
unsigned long lastResync = 0;
unsigned long lastDisplay = 0;
unsigned long lastWiFiTry = 0;

// ====== 5x7 digit font (two digits fit) ======
const uint8_t DIGITS[10][7] = {
  {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}, //0
  {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}, //1
  {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}, //2
  {0b11110,0b00001,0b00001,0b01110,0b00001,0b00001,0b11110}, //3
  {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}, //4
  {0b11111,0b10000,0b10000,0b11110,0b00001,0b00001,0b11110}, //5
  {0b01110,0b10000,0b10000,0b11110,0b10001,0b10001,0b01110}, //6
  {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}, //7
  {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}, //8
  {0b01110,0b10001,0b10001,0b01111,0b00001,0b00001,0b01110}  //9
};

// ====== Matrix packing (UNO R4: MSB-first) ======
inline void setPixel(uint32_t frame[3], int row, int col, bool on) {
  if (row < 0 || row > 7 || col < 0 || col > 11) return;
  int idx  = row * 12 + col;      // 0..95
  int word = idx / 32;            // 0..2
  int bit  = 31 - (idx % 32);     // MSB-first
  uint32_t mask = (1UL << bit);
  if (on) frame[word] |= mask;
  else    frame[word] &= ~mask;
}

void drawTwoDigits(uint32_t frame[3], int value, bool modeDot, bool notSyncedDot) {
  frame[0] = frame[1] = frame[2] = 0;

  int tens = (value / 10) % 10;
  int ones = value % 10;

  for (int r = 0; r < 7; r++) {
    for (int c = 0; c < 5; c++) {
      bool onL = (DIGITS[tens][r] >> (4 - c)) & 0x01;
      bool onR = (DIGITS[ones][r] >> (4 - c)) & 0x01;
      setPixel(frame, r, c, onL);
      setPixel(frame, r, 6 + c, onR);
    }
  }

  // bottom-right dot ON when showing minutes
  setPixel(frame, 7, 11, modeDot);

  // bottom-left dot ON when NOT synced yet
  setPixel(frame, 7, 0, notSyncedDot);
}

// ====== DST helpers (America/Chicago) ======
bool isLeap(int y) { return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)); }
int daysInMonth(int y, int m) {
  static const int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (m == 2) return isLeap(y) ? 29 : 28;
  return dim[m - 1];
}

// epoch -> Y/M/D H:M:S (UTC)
void epochToYMDHMS(uint32_t epoch, int &y, int &mo, int &d, int &hh, int &mm, int &ss) {
  ss = epoch % 60; epoch /= 60;
  mm = epoch % 60; epoch /= 60;
  hh = epoch % 24; epoch /= 24;
  uint32_t days = epoch;

  y = 1970;
  while (true) {
    uint32_t diy = isLeap(y) ? 366 : 365;
    if (days >= diy) { days -= diy; y++; }
    else break;
  }

  mo = 1;
  while (true) {
    int dim = daysInMonth(y, mo);
    if (days >= (uint32_t)dim) { days -= dim; mo++; }
    else break;
  }
  d = (int)days + 1;
}

// 0=Sun..6=Sat
int dayOfWeek(int y, int m, int d) {
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

int nthSunday(int y, int m, int nth) {
  int count = 0;
  for (int day = 1; day <= daysInMonth(y, m); day++) {
    if (dayOfWeek(y, m, day) == 0) {
      count++;
      if (count == nth) return day;
    }
  }
  return 1;
}

bool isChicagoDST(int y, int mo, int d, int hh) {
  int startDay = nthSunday(y, 3, 2);
  int endDay   = nthSunday(y, 11, 1);

  if (mo < 3 || mo > 11) return false;
  if (mo > 3 && mo < 11) return true;

  if (mo == 3) {
    if (d < startDay) return false;
    if (d > startDay) return true;
    return hh >= 2;
  }

  // mo == 11
  if (d < endDay) return true;
  if (d > endDay) return false;
  return hh < 2;
}

uint32_t utcToChicago(uint32_t epochUTC) {
  // assume CST (UTC-6) then adjust if DST
  int y, mo, d, hh, mm, ss;
  uint32_t epochCST = epochUTC - 6UL * 3600UL;
  epochToYMDHMS(epochCST, y, mo, d, hh, mm, ss);

  if (isChicagoDST(y, mo, d, hh)) return epochUTC - 5UL * 3600UL; // CDT
  return epochCST; // CST
}

// ====== WiFi/NTP ======
void tryWiFiNonBlocking() {
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastWiFiTry < WIFI_RETRY_MS) return;
  lastWiFiTry = now;

  Serial.println("WiFi: trying to connect...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

bool syncRTCFromNTP() {
  if (WiFi.status() != WL_CONNECTED) return false;

  Serial.println("NTP: syncing...");
  ntp.begin();
  if (!ntp.update()) ntp.forceUpdate();

  uint32_t epochUTC = (uint32_t)ntp.getEpochTime();
  if (epochUTC < 1700000000UL) { // sanity check
    Serial.println("NTP: bad epoch received");
    return false;
  }

  uint32_t epochLocal = utcToChicago(epochUTC);
  RTCTime t((time_t)epochLocal);
  RTC.setTime(t);

  Serial.print("NTP: synced. Local epoch=");
  Serial.println(epochLocal);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  matrix.begin();
  RTC.begin();

  // prove matrix is alive immediately (brief flash)
  {
    uint32_t frame[3] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    matrix.loadFrame(frame);
    delay(250);
  }

  Serial.println("Booting clock...");
  tryWiFiNonBlocking();
}

void loop() {
  unsigned long nowMs = millis();

  // 1) Always keep trying WiFi occasionally (non-blocking)
  tryWiFiNonBlocking();

  // 2) If connected and time not synced yet, sync ASAP
  if (!timeSynced && WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
    timeSynced = syncRTCFromNTP();
    if (timeSynced) lastResync = nowMs;
  }

  // 3) Periodic resync to stay accurate
  if (timeSynced && (nowMs - lastResync >= RESYNC_MS) && WiFi.status() == WL_CONNECTED) {
    if (syncRTCFromNTP()) lastResync = nowMs;
  }

  // 4) Display update (ALWAYS runs)
  if (nowMs - lastDisplay >= DISPLAY_MS) {
    lastDisplay = nowMs;

    RTCTime now;
    RTC.getTime(now);

    int hh = now.getHour();
    int mm = now.getMinutes();
    int ss = now.getSeconds();

    bool showMinutes = (ss % 2) == 1;

    uint32_t frame[3];
    drawTwoDigits(frame, showMinutes ? mm : hh, showMinutes, !timeSynced);
    matrix.loadFrame(frame);
  }
}
