#include <SPI.h>
#include <MFRC522.h>
#include <WiFiS3.h>
#include <U8g2lib.h>
#include "arduino_secrets.h"
#include "access_config.h"

// ===================== VERSION =====================
#define VERSION_STR "v2.3-R4-GFX"

// ===================== PIN MAP =====================
#define SS_PIN   10
#define RST_PIN  9

#define RELAY_PIN      7
#define RED_LED_PIN    2
#define GREEN_LED_PIN  3
#define BUZZER_PIN     6

// OLED software SPI (your wiring)
#define OLED_CLK  A1
#define OLED_MOSI A2
#define OLED_CS   8
#define OLED_DC   4
#define OLED_RST  5

// Door contact
#define DOOR_PIN  A3
const bool DOOR_CONTACT_NC = true;

// PTE button
#define PTE_BTN_PIN A4

// PIR motion REX
#define PIR_PIN A5
const bool PIR_ACTIVE_HIGH = true;

// ===================== WIFI ========================
// Wi-Fi values are loaded from the ignored arduino_secrets.h file.
WiFiServer server(80);

// ===================== TIMING ======================
const uint32_t ACTION_MS              = 5000;
const uint32_t REMOTE_ACTION_MS       = 3000;
const uint32_t COOLDOWN_MS            = 800;

const uint32_t DOOR_HELD_OPEN_MS      = 15000;
const uint32_t FORCED_OPEN_WINDOW_MS  = 6000;
const uint32_t UNLOCK_OPEN_WINDOW_MS  = 8000;

const uint32_t PTE_MIN_UNLOCK_MS      = 5000;
const uint32_t BTN_DEBOUNCE_MS        = 40;

const uint32_t PIR_COOLDOWN_MS        = 2500;

const uint32_t OLED_BOOT_SHOW_MS      = 10000; // 10 sec boot/IP

// ===================== RFID ========================
MFRC522 rfid(SS_PIN, RST_PIN);

// ===================== OLED (U8g2 graphics) ========
// SH1106 128x64, SW SPI
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST);

// ===================== USERS =======================
// Authorized card UIDs are loaded from the ignored access_config.h file.

// ===================== ALARMS ======================
enum AlarmType : uint8_t { ALARM_NONE, ALARM_FORCED_OPEN, ALARM_HELD_OPEN };
AlarmType alarmType = ALARM_NONE;
bool alarmSilenced = false;

// ===================== LOG (ring buffer) ===========
struct LogEvent {
  uint32_t ms;
  char type[12];
  char detail[40];
};
static const uint8_t LOG_CAP = 20;
LogEvent logBuf[LOG_CAP];
uint8_t logHead = 0;
uint8_t logCount = 0;

void logEvent(const char* type, const char* detail) {
  LogEvent &e = logBuf[logHead];
  e.ms = millis();
  strncpy(e.type, type, sizeof(e.type) - 1); e.type[sizeof(e.type) - 1] = '\0';
  strncpy(e.detail, detail, sizeof(e.detail) - 1); e.detail[sizeof(e.detail) - 1] = '\0';
  logHead = (logHead + 1) % LOG_CAP;
  if (logCount < LOG_CAP) logCount++;
}

void stamp_mmss(uint32_t ms, char* out, size_t n) {
  uint32_t sec = ms / 1000;
  uint32_t mm = sec / 60;
  uint32_t ss = sec % 60;
  snprintf(out, n, "%lu:%02lu", (unsigned long)mm, (unsigned long)ss);
}

// ===================== STATE =======================
uint32_t offAt = 0;
uint32_t lastEventMs = 0;

char lastUID[20]     = "-";
char lastAction[28]  = "INIT";
char lastUser[10]    = "-";

bool doorClosed = true;
bool lastDoorClosed = true;
uint32_t doorOpenedAt = 0;
uint32_t lastDoorSampleMs = 0;

uint32_t lastUnlockMs = 0;
bool unlockPending = false;
uint32_t unlockPendingUntil = 0;

// PTE
bool btnRawLast = true;
bool btnStable = true;
uint32_t btnChangedMs = 0;
bool pteHolding = false;
uint32_t pteMinUnlockUntil = 0;

// PIR
uint32_t lastPirMs = 0;

// Siren
uint32_t sirenPhaseStartMs = 0;
bool sirenOnPhase = true;
uint32_t sirenLastStepMs = 0;
int fFreq = 900, fStep = 90;
int hFreq = 650, hStep = 35;

// OLED state
uint32_t bootStartMs = 0;
bool oledBootMode = true;

// Event text (2 lines max, guaranteed to fit)
char oledMsg1[18] = "                ";
char oledMsg2[18] = "                ";

// Icon animation state
uint8_t doorFrameCur = 0; // 0=closed, 10=open
uint8_t lockFrameCur = 0; // 0=locked, 10=unlocked

// ===================== RELAY / IO ==================
inline void relayOn()  { digitalWrite(RELAY_PIN, HIGH); }
inline void relayOff() { digitalWrite(RELAY_PIN, LOW);  }
inline bool relayIsOn(){ return (digitalRead(RELAY_PIN) == HIGH); }

void accessOutputsOff() {
  relayOff();
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
}

// ===================== DOOR ========================
bool readDoorClosed() {
  int raw = digitalRead(DOOR_PIN);
  if (DOOR_CONTACT_NC) return (raw == LOW);
  else                return (raw == HIGH);
}

// ===================== UID =========================
bool uidEquals(const char* a, const char* b) { return strcmp(a, b) == 0; }

bool isValidUID(const char* uid) {
  if (uidEquals(uid, UID_USER1)) return true;
  if (UID_USER2[0] && uidEquals(uid, UID_USER2)) return true;
  return false;
}

const char* userFromUID(const char* uid) {
  if (uidEquals(uid, UID_USER1)) return "USER1";
  if (UID_USER2[0] && uidEquals(uid, UID_USER2)) return "USER2";
  return "-";
}

void uidToString(MFRC522::Uid *u, char* out, size_t n) {
  size_t pos = 0;
  for (byte i = 0; i < u->size; i++) {
    if (pos + 3 >= n) break;
    int wrote = snprintf(out + pos, n - pos, "%02X", u->uidByte[i]);
    if (wrote <= 0) break;
    pos += (size_t)wrote;
    if (i < u->size - 1 && pos + 2 < n) {
      out[pos++] = ':';
      out[pos] = '\0';
    }
  }
  out[n - 1] = '\0';
}

// ===================== OLED HELPERS ================
static inline void fit16(char* out, const char* in) {
  size_t i = 0;
  for (; i < 16 && in[i]; i++) out[i] = in[i];
  for (; i < 16; i++) out[i] = ' ';
  out[16] = '\0';
}

void oledSetEvent(const char* l1, const char* l2 = "") {
  fit16(oledMsg1, l1);
  fit16(oledMsg2, l2);
}

// ----- ICON DRAWING (fits on right side) -----
// Icon region: x=64..127, y=0..63 (64x64)
static const int ICON_X = 64;
static const int ICON_Y = 0;

// frame 0..10 (0 closed, 10 open)
void drawDoorFrame(uint8_t frame) {
  if (frame > 10) frame = 10;

  int x = ICON_X + 6;
  int y = ICON_Y + 10;
  int w = 26;
  int h = 44;

  // outer frame
  u8g2.drawFrame(x, y, w, h);

  // hinge
  u8g2.drawLine(x + 2, y + 2, x + 2, y + h - 3);

  int inset = map(frame, 0, 10, 0, 12);
  int skew  = map(frame, 0, 10, 0, 6);

  int xL = x + 3;
  int xR = x + w - 3 - inset;
  int yT = y + 3 + skew;
  int yB = y + h - 4 - skew;

  if (xR < xL + 2) xR = xL + 2;
  if (yB < yT + 10) yB = yT + 10;

  // door leaf
  u8g2.drawLine(xL, y + 3, xL, y + h - 4);
  u8g2.drawLine(xL, y + 3, xR, yT);
  u8g2.drawLine(xL, y + h - 4, xR, yB);
  u8g2.drawLine(xR, yT, xR, yB);

  // knob
  int knobX = xR - 2;
  int knobY = (yT + yB) / 2;
  u8g2.drawDisc(knobX, knobY, 2);

  // light rays if opening
  if (frame >= 6) {
    int gx = x + w + 2;
    u8g2.drawLine(gx, y + 8,  gx + 10, y + 4);
    u8g2.drawLine(gx, y + 22, gx + 12, y + 22);
    u8g2.drawLine(gx, y + 36, gx + 10, y + 40);
  }
}

// frame 0..10 (0 locked, 10 unlocked)
void drawLockFrame(uint8_t frame) {
  if (frame > 10) frame = 10;

  int bx = ICON_X + 34;
  int by = ICON_Y + 30;
  int bw = 24;
  int bh = 24;

  int sx = bx + 4;
  int sy = by - 16;
  int sw = bw - 8;
  int sh = 16;

  int lift  = map(frame, 0, 10, 0, 6);
  int shift = map(frame, 0, 10, 0, 4);

  // body
  u8g2.drawRFrame(bx, by, bw, bh, 3);
  // keyhole
  u8g2.drawDisc(bx + bw/2, by + 9, 3);
  u8g2.drawBox(bx + bw/2 - 1, by + 12, 2, 7);

  // shackle
  u8g2.drawLine(sx + shift, sy - lift + sh, sx + shift, sy - lift + 5);
  u8g2.drawLine(sx + shift, sy - lift + 5, sx + shift + sw, sy - lift + 5);

  if (frame < 7) {
    u8g2.drawLine(sx + shift + sw, sy - lift + 5, sx + shift + sw, sy - lift + sh);
  } else {
    u8g2.drawLine(sx + shift + sw, sy - lift + 5, sx + shift + sw, sy - lift + sh - 7);
    // open rays
    u8g2.drawLine(bx + bw - 2, by - 6, bx + bw + 6, by - 10);
    u8g2.drawLine(bx + bw - 2, by - 1, bx + bw + 8, by - 1);
    u8g2.drawLine(bx + bw - 2, by + 4, bx + bw + 6, by + 8);
  }
}

void oledRender() {
  u8g2.clearBuffer();

  // Left side text (0..63)
  u8g2.setFont(u8g2_font_6x10_tf);

  // Status lines (always visible)
  u8g2.setCursor(0, 12);
  u8g2.print(doorClosed ? "DOOR: CLOSED" : "DOOR: OPEN");

  u8g2.setCursor(0, 24);
  u8g2.print(relayIsOn() ? "LOCK: UNLOCKED" : "LOCK: LOCKED");

  // Event lines (always fit)
  u8g2.setCursor(0, 42);
  u8g2.print(oledMsg1);

  u8g2.setCursor(0, 54);
  u8g2.print(oledMsg2);

  // Alarm footer (short)
  u8g2.setCursor(0, 64);
  // Note: 64 is bottom baseline; 6x10 font baseline at 64 works.
  if (alarmType == ALARM_FORCED_OPEN) u8g2.print(alarmSilenced ? "DFO: SILENCED" : "DFO: ALARM");
  else if (alarmType == ALARM_HELD_OPEN) u8g2.print(alarmSilenced ? "DHO: SILENCED" : "DHO: ALARM");
  else u8g2.print(" ");

  // Right side icons
  drawDoorFrame(doorFrameCur);
  drawLockFrame(lockFrameCur);

  u8g2.sendBuffer();
}

void oledShowBootWithIp(const char* ip) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 12); u8g2.print("ACCESS CONTROL");
  u8g2.setCursor(0, 24); u8g2.print(VERSION_STR);
  u8g2.setCursor(0, 42); u8g2.print("IP:");
  u8g2.setCursor(0, 54); u8g2.print(ip);
  u8g2.sendBuffer();
}

// short, fast animations (keeps system responsive)
void animateDoorTo(uint8_t targetFrame) {
  targetFrame = constrain(targetFrame, 0, 10);
  if (doorFrameCur == targetFrame) return;

  int step = (targetFrame > doorFrameCur) ? 1 : -1;

  // quick animation: max 11 frames, 18ms each => ~200ms worst case
  for (int f = doorFrameCur; f != (int)targetFrame; f += step) {
    doorFrameCur = (uint8_t)f;
    oledRender();
    delay(18);
  }
  doorFrameCur = targetFrame;
  oledRender();
}

void animateLockTo(uint8_t targetFrame) {
  targetFrame = constrain(targetFrame, 0, 10);
  if (lockFrameCur == targetFrame) return;

  int step = (targetFrame > lockFrameCur) ? 1 : -1;
  for (int f = lockFrameCur; f != (int)targetFrame; f += step) {
    lockFrameCur = (uint8_t)f;
    oledRender();
    delay(18);
  }
  lockFrameCur = targetFrame;
  oledRender();
}

void buildIpString(char* ipbuf, size_t n) {
  IPAddress ip = WiFi.localIP();
  snprintf(ipbuf, n, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

// ===================== SOUNDS ======================
void chimeGranted() {
  if (alarmType != ALARM_NONE) return;
  tone(BUZZER_PIN, 1800, 120); delay(140);
  tone(BUZZER_PIN, 2200, 140); delay(160);
  noTone(BUZZER_PIN);
}
void chimeDenied() {
  if (alarmType != ALARM_NONE) return;
  tone(BUZZER_PIN, 400, 180); delay(220);
  tone(BUZZER_PIN, 400, 180); delay(220);
  tone(BUZZER_PIN, 300, 220); delay(250);
  noTone(BUZZER_PIN);
}

// ===================== ALARM CONTROL ===============
void resetSiren() {
  sirenPhaseStartMs = millis();
  sirenOnPhase = true;
  sirenLastStepMs = 0;
  fFreq = 900;  fStep = 90;
  hFreq = 650;  hStep = 35;
}

void startAlarm(AlarmType t) {
  alarmType = t;
  alarmSilenced = false;
  noTone(BUZZER_PIN);
  resetSiren();

  if (t == ALARM_FORCED_OPEN) {
    strncpy(lastAction, "DOOR FORCED", sizeof(lastAction)-1);
    logEvent("ALARM", "DOOR FORCED OPEN");
    oledSetEvent("DOOR FORCED", "OPEN ALARM");
  } else {
    strncpy(lastAction, "DOOR HELD", sizeof(lastAction)-1);
    logEvent("ALARM", "DOOR HELD OPEN");
    oledSetEvent("DOOR HELD", "OPEN ALARM");
  }
  oledRender();
}

void silenceAlarm() {
  if (alarmType == ALARM_NONE) return;
  if (alarmSilenced) return;

  alarmSilenced = true;
  noTone(BUZZER_PIN);

  strncpy(lastAction, "ALARM SILENCED", sizeof(lastAction)-1);
  logEvent("SILENCE", "Alarm silenced (latched)");
  oledSetEvent("ALARM", "SILENCED");
  oledRender();
}

void clearAlarmOnDoorClose() {
  if (alarmType == ALARM_NONE) return;

  alarmType = ALARM_NONE;
  alarmSilenced = false;
  noTone(BUZZER_PIN);
  resetSiren();

  strncpy(lastAction, "ALARM RESET", sizeof(lastAction)-1);
  logEvent("AL_CLR", "Door closed - reset");
  oledSetEvent("ALARM RESET", "DOOR CLOSED");
  oledRender();
}

void updateSiren(uint32_t now) {
  if (alarmType == ALARM_NONE || alarmSilenced) return;

  uint32_t onDur  = (alarmType == ALARM_FORCED_OPEN) ? 2000 : 1000;
  uint32_t offDur = (alarmType == ALARM_FORCED_OPEN) ? 200  : 500;

  uint32_t phaseDur = sirenOnPhase ? onDur : offDur;
  if (now - sirenPhaseStartMs >= phaseDur) {
    sirenOnPhase = !sirenOnPhase;
    sirenPhaseStartMs = now;
    sirenLastStepMs = 0;
    if (!sirenOnPhase) { noTone(BUZZER_PIN); return; }
  }
  if (!sirenOnPhase) return;

  uint32_t stepEvery = (alarmType == ALARM_FORCED_OPEN) ? 25 : 55;
  if (now - sirenLastStepMs < stepEvery) return;
  sirenLastStepMs = now;

  if (alarmType == ALARM_FORCED_OPEN) {
    fFreq += fStep;
    if (fFreq >= 2600) { fFreq = 2600; fStep = -fStep; }
    if (fFreq <= 700 ) { fFreq = 700;  fStep = -fStep; }
    tone(BUZZER_PIN, fFreq);
  } else {
    hFreq += hStep;
    if (hFreq >= 1600) { hFreq = 1600; hStep = -hStep; }
    if (hFreq <= 650 ) { hFreq = 650;  hStep = -hStep; }
    tone(BUZZER_PIN, hFreq);
  }
}

// ===================== UNLOCK ACTIONS ===============
void beginUnlock(const char* actionLabel, const char* userLabel, uint32_t pulseMs, bool setPending) {
  accessOutputsOff();
  digitalWrite(GREEN_LED_PIN, HIGH);
  relayOn();

  // Animate lock -> unlocked
  animateLockTo(10);

  lastUnlockMs = millis();

  strncpy(lastAction, actionLabel, sizeof(lastAction)-1);
  strncpy(lastUser, userLabel, sizeof(lastUser)-1);

  if (setPending) {
    unlockPending = true;
    unlockPendingUntil = lastUnlockMs + UNLOCK_OPEN_WINDOW_MS;
  } else {
    unlockPending = false;
    unlockPendingUntil = 0;
  }

  offAt = millis() + pulseMs;

  char d[40];
  snprintf(d, sizeof(d), "%s %s", actionLabel, userLabel);
  logEvent("UNLOCK", d);

  // OLED event text (short)
  if (strcmp(actionLabel, "ACCESS GRANTED") == 0) oledSetEvent("ACCESS", "GRANTED");
  else if (strcmp(actionLabel, "REMOTE UNLOCK") == 0) oledSetEvent("REMOTE", "UNLOCK");
  else oledSetEvent(actionLabel, "");

  oledRender();

  if (alarmType == ALARM_NONE) chimeGranted();
}

void ptePressed(uint32_t now) {
  pteHolding = true;
  pteMinUnlockUntil = now + PTE_MIN_UNLOCK_MS;

  accessOutputsOff();
  digitalWrite(GREEN_LED_PIN, HIGH);
  relayOn();

  animateLockTo(10);

  lastUnlockMs = now;
  unlockPending = false;
  unlockPendingUntil = 0;

  strncpy(lastAction, "PTE", sizeof(lastAction)-1);
  strncpy(lastUser, "-", sizeof(lastUser)-1);
  logEvent("REX", "PTE BUTTON");

  oledSetEvent("PTE", "UNLOCK");
  oledRender();
}

void pteReleased(uint32_t now) {
  pteHolding = false;

  if (now >= pteMinUnlockUntil) {
    relayOff();
    digitalWrite(GREEN_LED_PIN, LOW);

    animateLockTo(0);

    logEvent("REX", "PTE RELEASE -> LOCK");
    oledSetEvent("LOCK", "ENGAGED");
    oledRender();
  } else {
    logEvent("REX", "PTE TAP -> 5s");
  }
}

// ===================== FULL GUI HTML ================
static const char GUI_HTML[] =
"<!doctype html><html><head><meta charset=utf-8>"
"<meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>Access Control</title>"
"<style>"
":root{--bg:#0b0f14;--card:#111826;--line:#223041;--mut:#9fb0c0;--txt:#e6edf3;--blue:#0f6cff;--red:#c62828}"
"body{margin:0;font-family:system-ui,Segoe UI,Arial;background:var(--bg);color:var(--txt)}"
".wrap{max-width:900px;margin:0 auto;padding:18px}"
".card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:16px;box-shadow:0 8px 24px rgba(0,0,0,.35)}"
"h1{font-size:18px;margin:0 0 8px;display:flex;gap:10px;align-items:center}"
".pill{padding:6px 10px;border-radius:999px;background:#0d1522;border:1px solid var(--line);color:#cfe3ff;font-size:12px}"
".muted{color:var(--mut);font-size:13px;margin:0 0 14px}"
".row{display:flex;gap:10px;flex-wrap:wrap}"
"button{flex:1;min-width:220px;padding:14px 12px;border-radius:14px;border:1px solid #2a3a52;background:var(--blue);color:#fff;font-weight:800;font-size:15px;cursor:pointer}"
"button.alt{background:var(--red);border-color:#6b1b1b}"
"button:disabled{opacity:.55;cursor:not-allowed}"
".grid{margin-top:12px;display:grid;gap:10px;grid-template-columns:1fr 1fr}"
".kv{display:flex;justify-content:space-between;gap:10px;background:#0d1522;border:1px solid var(--line);border-radius:12px;padding:10px}"
".k{color:var(--mut);font-size:12px}"
".v{font-weight:650}"
".log{margin-top:12px;background:#0d1522;border:1px solid var(--line);border-radius:12px;padding:10px}"
"pre{margin:0;white-space:pre-wrap;word-break:break-word;font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:12px;color:#d3deea}"
".big{font-size:22px}"
".badge{display:inline-block;margin-left:8px;padding:3px 8px;border-radius:999px;border:1px solid var(--line);font-size:12px}"
"</style></head><body><div class=wrap><div class=card>"
"<h1><span id=lockIcon class=big>🔒</span> Access Control <span class=pill>" VERSION_STR
"</span><span id=alarmBadge class=badge>OK</span></h1>"
"<p class=muted>IP: <span id=ip>...</span> - RFID + Web + Door + Alarms + REX</p>"
"<div class=row>"
"<button id=unlock>REMOTE UNLOCK</button>"
"<button id=silence class=alt>SILENCE ALARM</button>"
"</div>"
"<div class=grid>"
"<div class=kv><div class=k>Door</div><div class=v id=door>...</div></div>"
"<div class=kv><div class=k>Lock</div><div class=v id=lock>...</div></div>"
"<div class=kv><div class=k>Alarm</div><div class=v id=alarm>...</div></div>"
"<div class=kv><div class=k>User</div><div class=v id=user>...</div></div>"
"<div class=kv><div class=k>Last action</div><div class=v id=la>...</div></div>"
"<div class=kv><div class=k>Last UID</div><div class=v id=uid>...</div></div>"
"</div>"
"<div class=log><pre id=logbox>Loading log...</pre></div>"
"</div></div>"
"<script>"
"const $=id=>document.getElementById(id);"
"async function jget(p){return (await fetch(p,{cache:'no-store'})).json();}"
"async function tget(p){return (await fetch(p,{cache:'no-store'})).text();}"
"async function refresh(){try{"
"const s=await jget('/status');"
"$('ip').textContent=s.ip;"
"$('door').textContent=s.d?'CLOSED':'OPEN';"
"$('lock').textContent=s.l?'UNLOCKED':'LOCKED';"
"$('lockIcon').textContent=s.l?'🔓':'🔒';"
"$('alarm').textContent=s.a;"
"$('alarmBadge').textContent=s.a;"
"$('user').textContent=s.u||'-';"
"$('la').textContent=s.la;"
"$('uid').textContent=s.uid||'-';"
"$('silence').disabled=!s.al;"
"}catch(e){}}"
"async function refreshLog(){try{$('logbox').textContent=await tget('/log');}catch(e){}}"
"$('unlock').onclick=async()=>{"
"$('unlock').disabled=true;$('unlock').textContent='UNLOCKING...';"
"try{await fetch('/unlock',{cache:'no-store'});}catch(e){}"
"setTimeout(()=>{$('unlock').disabled=false;$('unlock').textContent='REMOTE UNLOCK';},900);"
"setTimeout(()=>{refresh();refreshLog();},180);"
"};"
"$('silence').onclick=async()=>{"
"$('silence').disabled=true;"
"try{await fetch('/silence',{cache:'no-store'});}catch(e){}"
"setTimeout(()=>{refresh();refreshLog();},180);"
"};"
"setInterval(refresh,600);setInterval(refreshLog,1500);"
"refresh();refreshLog();"
"</script></body></html>";

// ===================== HTTP HELPERS ================
void sendText(WiFiClient& c, const char* ct, const char* body) {
  c.println("HTTP/1.1 200 OK");
  c.print("Content-Type: "); c.println(ct);
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.print("Content-Length: "); c.println(strlen(body));
  c.println();
  c.print(body);
}

void sendHtml(WiFiClient& c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: text/html; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print(GUI_HTML);
}

void send404(WiFiClient& c) {
  c.println("HTTP/1.1 404 Not Found");
  c.println("Content-Type: text/plain");
  c.println("Connection: close");
  c.println();
  c.println("Not found");
}

// ===================== WEB HANDLER ==================
void handleWeb() {
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.read(); // \n

  while (client.connected() && client.available()) {
    String h = client.readStringUntil('\n');
    if (h == "\r" || h.length() == 0) break;
  }

  if (req.indexOf("GET / ") >= 0) {
    sendHtml(client);
  }
  else if (req.indexOf("GET /unlock") >= 0) {
    beginUnlock("REMOTE UNLOCK", "WEB", REMOTE_ACTION_MS, true);
    sendText(client, "text/plain", "OK");
  }
  else if (req.indexOf("GET /silence") >= 0) {
    silenceAlarm();
    sendText(client, "text/plain", "OK");
  }
  else if (req.indexOf("GET /status") >= 0) {
    char ipbuf[24];
    buildIpString(ipbuf, sizeof(ipbuf));

    const char* a = "OK";
    bool al = (alarmType != ALARM_NONE);
    if (alarmType == ALARM_FORCED_OPEN) a = alarmSilenced ? "FORCED OPEN (SIL)" : "FORCED OPEN (SND)";
    else if (alarmType == ALARM_HELD_OPEN) a = alarmSilenced ? "HELD OPEN (SIL)" : "HELD OPEN (SND)";

    char json[260];
    snprintf(json, sizeof(json),
      "{\"ip\":\"%s\",\"d\":%s,\"l\":%s,\"a\":\"%s\",\"al\":%s,\"u\":\"%s\",\"la\":\"%s\",\"uid\":\"%s\"}",
      ipbuf,
      doorClosed ? "true":"false",
      relayIsOn() ? "true":"false",
      a,
      al ? "true":"false",
      lastUser,
      lastAction,
      lastUID
    );

    sendText(client, "application/json; charset=utf-8", json);
  }
  else if (req.indexOf("GET /log") >= 0) {
    static char out[1400];
    size_t pos = 0;
    pos += snprintf(out + pos, sizeof(out) - pos,
                    "Uptime | Event | Detail\n--------------------------------\n");

    uint8_t start = (logHead + LOG_CAP - logCount) % LOG_CAP;
    for (uint8_t i = 0; i < logCount; i++) {
      uint8_t idx = (start + i) % LOG_CAP;
      char t[10]; stamp_mmss(logBuf[idx].ms, t, sizeof(t));
      pos += snprintf(out + pos, sizeof(out) - pos, "%s | %s | %s\n",
                      t, logBuf[idx].type, logBuf[idx].detail);
      if (pos >= sizeof(out) - 80) break;
    }
    out[sizeof(out) - 1] = '\0';
    sendText(client, "text/plain; charset=utf-8", out);
  }
  else {
    send404(client);
  }

  delay(2);
  client.stop();
}

// ===================== SETUP ========================
void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(PTE_BTN_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);

  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  accessOutputsOff();
  noTone(BUZZER_PIN);

  // OLED init
  u8g2.begin();
  u8g2.setContrast(255);

  bootStartMs = millis();
  oledBootMode = true;

  // RFID init
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  doorClosed = readDoorClosed();
  lastDoorClosed = doorClosed;

  // initial icon states
  doorFrameCur = doorClosed ? 0 : 10;
  lockFrameCur = relayIsOn() ? 10 : 0;

  logEvent("BOOT", VERSION_STR);

  Serial.println("=================================");
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: "); Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("=================================");

  if (WiFi.status() == WL_CONNECTED) {
    char ipbuf[24];
    buildIpString(ipbuf, sizeof(ipbuf));

    Serial.println("WIFI CONNECTED");
    Serial.print("IP: "); Serial.println(ipbuf);
    Serial.print("OPEN: http://"); Serial.println(ipbuf);

    server.begin();
    logEvent("WIFI", ipbuf);

    oledShowBootWithIp(ipbuf);
  } else {
    Serial.println("WIFI FAILED (RFID still works)");
    logEvent("WIFI", "FAILED");
    oledShowBootWithIp("WIFI FAILED");
  }

  oledSetEvent("SYSTEM", "STARTING");
}

// ===================== LOOP =========================
void loop() {
  uint32_t now = millis();

  handleWeb();

  // End boot OLED after 10 seconds
  if (oledBootMode && (now - bootStartMs >= OLED_BOOT_SHOW_MS)) {
    oledBootMode = false;
    oledRender();
  }

  // Timed off for unlock pulses
  if (offAt && now > offAt) {
    accessOutputsOff();
    offAt = 0;

    // Animate lock back to locked when relay drops
    animateLockTo(0);

    if (!oledBootMode) oledRender();
  }

  // ===== PTE button =====
  bool raw = digitalRead(PTE_BTN_PIN);
  if (raw != btnRawLast) { btnRawLast = raw; btnChangedMs = now; }

  if ((now - btnChangedMs) >= BTN_DEBOUNCE_MS && raw != btnStable) {
    btnStable = raw;
    if (btnStable == LOW) ptePressed(now);
    else pteReleased(now);
  }

  // If tapped, relock when min expires
  if (!pteHolding && pteMinUnlockUntil && now >= pteMinUnlockUntil) {
    pteMinUnlockUntil = 0;
    if (relayIsOn()) {
      relayOff();
      digitalWrite(GREEN_LED_PIN, LOW);

      animateLockTo(0);

      logEvent("REX", "PTE MIN DONE -> LOCK");
      oledSetEvent("LOCK", "ENGAGED");
      if (!oledBootMode) oledRender();
    }
  }

  // ===== PIR motion REX =====
  int pirRaw = digitalRead(PIR_PIN);
  bool motion = PIR_ACTIVE_HIGH ? (pirRaw == HIGH) : (pirRaw == LOW);

  if (motion && (now - lastPirMs) > PIR_COOLDOWN_MS) {
    lastPirMs = now;

    accessOutputsOff();
    digitalWrite(GREEN_LED_PIN, HIGH);
    relayOn();

    animateLockTo(10);

    lastUnlockMs = now;
    unlockPending = false;
    unlockPendingUntil = 0;

    offAt = now + PTE_MIN_UNLOCK_MS;

    strncpy(lastAction, "REX MOTION", sizeof(lastAction)-1);
    strncpy(lastUser, "-", sizeof(lastUser)-1);
    logEvent("REX", "MOTION");

    oledSetEvent("REX", "MOTION");
    if (!oledBootMode) oledRender();
  }

  // ===== Door sampling =====
  if (now - lastDoorSampleMs >= 50) {
    lastDoorSampleMs = now;
    doorClosed = readDoorClosed();

    if (doorClosed != lastDoorClosed) {
      lastDoorClosed = doorClosed;

      // Animate door icon on transition
      animateDoorTo(doorClosed ? 0 : 10);

      if (!doorClosed) {
        doorOpenedAt = now;

        if (unlockPending) { unlockPending = false; unlockPendingUntil = 0; }

        bool authorized = (lastUnlockMs != 0) && (now - lastUnlockMs <= FORCED_OPEN_WINDOW_MS);

        if (!authorized) {
          startAlarm(ALARM_FORCED_OPEN);
        } else {
          strncpy(lastAction, "DOOR OPEN", sizeof(lastAction)-1);
          logEvent("DOOR", "OPEN (authorized)");
          oledSetEvent("DOOR", "OPEN");
          if (!oledBootMode) oledRender();
        }
      } else {
        doorOpenedAt = 0;

        if (alarmType != ALARM_NONE) {
          clearAlarmOnDoorClose();
        } else {
          strncpy(lastAction, "DOOR CLOSED", sizeof(lastAction)-1);
          logEvent("DOOR", "CLOSED");
          oledSetEvent("DOOR", "CLOSED");
          if (!oledBootMode) oledRender();
        }

        unlockPending = false;
        unlockPendingUntil = 0;
      }
    }
  }

  // ===== Unlock window timeout (RFID/Web only) =====
  if (unlockPending && unlockPendingUntil && now > unlockPendingUntil) {
    unlockPending = false;
    unlockPendingUntil = 0;

    if (doorClosed && alarmType == ALARM_NONE) {
      strncpy(lastAction, "UNLOCK TIMEOUT", sizeof(lastAction)-1);
      logEvent("TIMEOUT", "No door open in 8s");
      oledSetEvent("UNLOCK", "TIMEOUT");
      if (!oledBootMode) oledRender();
      tone(BUZZER_PIN, 1200, 120); delay(140); noTone(BUZZER_PIN);
    }
  }

  // ===== Door Held Open =====
  if (!doorClosed && alarmType == ALARM_NONE && doorOpenedAt) {
    bool authorizedOpen = (lastUnlockMs != 0) && (doorOpenedAt - lastUnlockMs <= FORCED_OPEN_WINDOW_MS);
    if (authorizedOpen && (now - doorOpenedAt >= DOOR_HELD_OPEN_MS)) {
      startAlarm(ALARM_HELD_OPEN);
      if (!oledBootMode) oledRender();
    }
  }

  // ===== Siren update =====
  updateSiren(now);

  // ===== RFID read =====
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  if ((now - lastEventMs) < COOLDOWN_MS) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastEventMs = now;

  char uid[20];
  uidToString(&rfid.uid, uid, sizeof(uid));
  strncpy(lastUID, uid, sizeof(lastUID)-1);

  if (isValidUID(uid)) {
    const char* user = userFromUID(uid);
    strncpy(lastUser, user, sizeof(lastUser)-1);

    beginUnlock("ACCESS GRANTED", user, ACTION_MS, true);
    logEvent("GRANTED", uid);

  } else {
    strncpy(lastUser, "-", sizeof(lastUser)-1);
    strncpy(lastAction, "ACCESS DENIED", sizeof(lastAction)-1);

    accessOutputsOff();
    digitalWrite(RED_LED_PIN, HIGH);
    offAt = now + ACTION_MS;

    logEvent("DENIED", uid);

    oledSetEvent("ACCESS", "DENIED");
    if (!oledBootMode) oledRender();

    chimeDenied();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(30);
}
