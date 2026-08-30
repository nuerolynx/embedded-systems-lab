#include <WiFiS3.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h"

/*
  UNO R4 WiFi - Access-style 8CH Relay Controller
  Pages:
    /           -> Control UI (dark)
    /configure  -> Configure UI (dark)

  APIs:
    /api/status
    /api/info
    /api/names
    /api/config
    /api/set?ch=1..8&on=0|1
    /api/all?on=0|1
    /api/pulse?ch=1..8&ms=50..10000   (ms optional uses default pulse)
    /api/pulse/set?ms=50..10000
    /api/name/set?ch=1..8&name=...
    /api/autooff/set?ch=1..8&sec=...
    /api/oneshot/set?ch=1..8&startLocal=YYYY-MM-DDTHH:MM&durSec=...
    /api/oneshot/clear?ch=1..8
    /api/daily/set?ch=1..8&en=0|1&s=0..1439&e=0..1439

  NOTE:
    - Uses NTP time. Schedules won't be accurate until timeValid=1.
    - Settings are RAM-only (reset on power loss).
*/

// Wi-Fi values are loaded from the ignored arduino_secrets.h file.

// Relay pins (CH1..CH8)
const int relayPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// LED ON = Relay ON
// activeLow=false => HIGH=ON
// activeLow=true  => LOW=ON
const bool activeLow = false;

WiFiServer server(80);
WiFiUDP udp;

// ------------------- TIME (NTP) -------------------
static const char* NTP_SERVER = "pool.ntp.org";
static const unsigned int NTP_PORT = 123;
static const int NTP_PACKET_SIZE = 48;
byte ntpPacket[NTP_PACKET_SIZE];

unsigned long lastNtpSyncMs = 0;
unsigned long ntpEpochAtSync = 0;   // seconds since 1970 at last sync
unsigned long millisAtSync = 0;
bool timeValid = false;

// America/Chicago basic offset (no DST handling here)
const long TZ_OFFSET_SECONDS = -6L * 3600L;

unsigned long nowEpochUtc() {
  if (!timeValid) return 0;
  unsigned long elapsed = (millis() - millisAtSync) / 1000UL;
  return ntpEpochAtSync + elapsed;
}

unsigned long nowEpochLocal() {
  unsigned long utc = nowEpochUtc();
  if (utc == 0) return 0;
  long shifted = (long)utc + TZ_OFFSET_SECONDS;
  if (shifted < 0) shifted = 0;
  return (unsigned long)shifted;
}

void sendNtpPacket(const char* address) {
  memset(ntpPacket, 0, NTP_PACKET_SIZE);
  ntpPacket[0] = 0b11100011; // LI, Version, Mode
  ntpPacket[1] = 0;         // Stratum
  ntpPacket[2] = 6;         // Polling Interval
  ntpPacket[3] = 0xEC;      // Precision
  udp.beginPacket(address, NTP_PORT);
  udp.write(ntpPacket, NTP_PACKET_SIZE);
  udp.endPacket();
}

bool syncTimeNtp() {
  udp.begin(2390);
  sendNtpPacket(NTP_SERVER);

  unsigned long start = millis();
  while (millis() - start < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      udp.read(ntpPacket, NTP_PACKET_SIZE);

      unsigned long highWord = (unsigned long)ntpPacket[40] << 24 |
                               (unsigned long)ntpPacket[41] << 16 |
                               (unsigned long)ntpPacket[42] << 8  |
                               (unsigned long)ntpPacket[43];

      const unsigned long SEVENTY_YEARS = 2208988800UL;
      unsigned long epoch = highWord - SEVENTY_YEARS;

      ntpEpochAtSync = epoch;
      millisAtSync = millis();
      lastNtpSyncMs = millis();
      timeValid = true;
      return true;
    }
    delay(10);
  }
  return false;
}

// ------------------- RELAYS / STATES -------------------
bool manualState[8]    = {false,false,false,false,false,false,false,false};
bool effectiveState[8] = {false,false,false,false,false,false,false,false};

unsigned long manualOnSince[8] = {0,0,0,0,0,0,0,0};

// Auto-OFF per channel (seconds). 0 disables.
unsigned long autoOffSec[8] = {0,0,0,0,0,0,0,0};

// Names
String chName[8] = {"Channel 1","Channel 2","Channel 3","Channel 4","Channel 5","Channel 6","Channel 7","Channel 8"};

// Pulse default (ms)
int cfgPulseMs = 800;

// ------------------- SCHEDULING -------------------
// One-time schedule per channel: start epoch (LOCAL) + duration seconds
bool oneShotEnabled[8] = {false,false,false,false,false,false,false,false};
unsigned long oneShotStartLocal[8] = {0,0,0,0,0,0,0,0};
unsigned long oneShotDurSec[8] = {0,0,0,0,0,0,0,0};

// Daily schedule window per channel: start/end minutes from midnight (LOCAL)
bool dailyEnabled[8] = {false,false,false,false,false,false,false,false};
int dailyStartMin[8] = {0,0,0,0,0,0,0,0};
int dailyEndMin[8]   = {0,0,0,0,0,0,0,0};

void writeRelayHardware(int idx, bool on) {
  if (activeLow) digitalWrite(relayPins[idx], on ? LOW : HIGH);
  else           digitalWrite(relayPins[idx], on ? HIGH : LOW);
}

void applyEffectiveToHardware() {
  for (int i=0;i<8;i++) writeRelayHardware(i, effectiveState[i]);
}

void setManual(int ch, bool on) {
  if (ch < 1 || ch > 8) return;
  int i = ch - 1;
  manualState[i] = on;
  manualOnSince[i] = on ? millis() : 0;
}

void pulseRelay(int ch, int ms) {
  if (ch < 1 || ch > 8) return;
  if (ms < 50) ms = 50;
  if (ms > 10000) ms = 10000;

  int i = ch - 1;
  // pulse directly on hardware
  writeRelayHardware(i, true);
  delay(ms);
  writeRelayHardware(i, false);

  // restore effective state after pulse
  applyEffectiveToHardware();
}

int localMinutesOfDay(unsigned long epochLocal) {
  if (epochLocal == 0) return 0;
  unsigned long secOfDay = epochLocal % 86400UL;
  return (int)(secOfDay / 60UL);
}

bool scheduleActiveFor(int i, unsigned long epochLocal) {
  // One-shot
  if (oneShotEnabled[i] && oneShotStartLocal[i] > 0 && oneShotDurSec[i] > 0) {
    unsigned long start = oneShotStartLocal[i];
    unsigned long end = start + oneShotDurSec[i];
    if (epochLocal >= start && epochLocal < end) return true;
    if (epochLocal >= end) oneShotEnabled[i] = false; // auto-expire
  }

  // Daily
  if (dailyEnabled[i] && epochLocal > 0) {
    int m = localMinutesOfDay(epochLocal);
    int s = dailyStartMin[i];
    int e = dailyEndMin[i];
    if (s == e) return false;

    if (s < e) {
      if (m >= s && m < e) return true;
    } else {
      // wraps midnight
      if (m >= s || m < e) return true;
    }
  }
  return false;
}

void updateEffectiveStates() {
  unsigned long epochLocal = nowEpochLocal();

  for (int i=0;i<8;i++) {
    bool schedOn = scheduleActiveFor(i, epochLocal);

    // auto-off manual if enabled and schedule not forcing ON
    if (!schedOn && manualState[i] && autoOffSec[i] > 0 && manualOnSince[i] > 0) {
      unsigned long elapsed = (millis() - manualOnSince[i]) / 1000UL;
      if (elapsed >= autoOffSec[i]) {
        manualState[i] = false;
        manualOnSince[i] = 0;
      }
    }

    effectiveState[i] = schedOn ? true : manualState[i];
  }

  applyEffectiveToHardware();
}

// ------------------- URL decode (UNO R4 compatible; NO String.insert) -------------------
String urlDecode(String s) {
  s.replace("+", " ");

  String out = "";
  out.reserve(s.length());

  auto hexVal = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  };

  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '%' && i + 2 < (int)s.length()) {
      int v1 = hexVal(s[i + 1]);
      int v2 = hexVal(s[i + 2]);
      if (v1 >= 0 && v2 >= 0) {
        out += char((v1 << 4) | v2);
        i += 2;
        continue;
      }
    }
    out += c;
  }
  return out;
}

// ------------------- HTTP helpers -------------------
void send200(WiFiClient &client, const char* contentType, const String &body) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");
  client.println();
  client.print(body);
}

void send200Text(WiFiClient &client, const String &body) {
  send200(client, "text/plain; charset=utf-8", body);
}

void send200Json(WiFiClient &client, const String &body) {
  send200(client, "application/json; charset=utf-8", body);
}

void send404(WiFiClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
  client.println("Not Found");
}

bool getQueryStr(const String &url, const char* key, String &outVal) {
  int q = url.indexOf('?');
  if (q < 0) return false;
  String query = url.substring(q + 1);

  String k = String(key) + "=";
  int p = query.indexOf(k);
  if (p < 0) return false;

  int start = p + k.length();
  int end = query.indexOf('&', start);
  if (end < 0) end = query.length();

  outVal = query.substring(start, end);
  outVal = urlDecode(outVal);
  return true;
}

bool getQueryInt(const String &url, const char* key, long &outVal) {
  String s;
  if (!getQueryStr(url, key, s)) return false;
  outVal = s.toInt();
  return true;
}

bool readRequestLine(WiFiClient &client, String &method, String &url) {
  String line = client.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return false;

  int sp1 = line.indexOf(' ');
  int sp2 = line.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) return false;

  method = line.substring(0, sp1);
  url = line.substring(sp1 + 1, sp2);
  return true;
}

void drainHeaders(WiFiClient &client) {
  while (client.connected()) {
    String h = client.readStringUntil('\n');
    if (h == "\r" || h.length() == 0) break;
  }
}

// ------------------- JSON builders -------------------
String jsonStatus() {
  String json = "{\"relays\":[";
  for (int i=0;i<8;i++) {
    json += (effectiveState[i] ? "1" : "0");
    if (i<7) json += ",";
  }
  json += "]}";
  return json;
}

String jsonInfo() {
  IPAddress ip = WiFi.localIP();
  String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
  String json = "{";
  json += "\"ssid\":\"" + String(ssid) + "\",";
  json += "\"ip\":\"" + ipStr + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"timeValid\":" + String(timeValid ? 1 : 0) + ",";
  json += "\"epochLocal\":" + String((unsigned long)nowEpochLocal());
  json += "}";
  return json;
}

String jsonNames() {
  String json = "{\"names\":[";
  for (int i=0;i<8;i++) {
    String n = chName[i];
    n.replace("\"", "'");
    json += "\"" + n + "\"";
    if (i<7) json += ",";
  }
  json += "]}";
  return json;
}

String jsonConfig() {
  String json = "{";
  json += "\"pulseMs\":" + String(cfgPulseMs) + ",";
  json += "\"autoOffSec\":[";
  for (int i=0;i<8;i++) { json += String(autoOffSec[i]); if (i<7) json += ","; }
  json += "],";
  json += "\"oneShot\":[";
  for (int i=0;i<8;i++) {
    json += "{";
    json += "\"en\":" + String(oneShotEnabled[i]?1:0) + ",";
    json += "\"start\":" + String(oneShotStartLocal[i]) + ",";
    json += "\"dur\":" + String(oneShotDurSec[i]);
    json += "}";
    if (i<7) json += ",";
  }
  json += "],";
  json += "\"daily\":[";
  for (int i=0;i<8;i++) {
    json += "{";
    json += "\"en\":" + String(dailyEnabled[i]?1:0) + ",";
    json += "\"s\":" + String(dailyStartMin[i]) + ",";
    json += "\"e\":" + String(dailyEndMin[i]);
    json += "}";
    if (i<7) json += ",";
  }
  json += "]";
  json += "}";
  return json;
}

// ------------------- Pages -------------------
const char* PAGE_MAIN = R"HTML(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta name="color-scheme" content="dark">
  <title>Relay Control</title>
  <style>
    :root{
      --bg:#0b0f14; --card:#121924; --border:#1f2a3a; --text:#e7edf6; --muted:#9aa7b8;
      --btn:#0f1622; --btnBorder:#223149; --on:#22c55e; --off:#3b475a; --thumb:#e7edf6;
    }
    body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial; margin:16px; background:var(--bg); color:var(--text); }
    .card { max-width: 590px; margin: 0 auto; padding: 16px; background:var(--card); border:1px solid var(--border); border-radius:14px; box-shadow:0 10px 30px rgba(0,0,0,.35);}
    .title { font-size:18px; font-weight:900; margin-bottom:8px; display:flex; justify-content:space-between; align-items:center; gap:10px;}
    .meta { display:flex; gap:10px; flex-wrap:wrap; margin-bottom:12px; color:var(--muted); font-size:13px; }
    .pill { padding:6px 10px; border:1px solid var(--border); border-radius:999px; background:rgba(255,255,255,0.02); }
    .divider { height:1px; background:var(--border); margin:14px 0; }
    .row { display:flex; align-items:center; justify-content:space-between; padding:12px 0; border-bottom:1px solid var(--border); gap:12px; }
    .row:last-child { border-bottom:none; }
    .left { display:flex; flex-direction:column; gap:2px; }
    .ch { font-weight:900; }
    .hint { font-size:12px; color:var(--muted); }
    .right { display:flex; align-items:center; gap:10px; }

    .switch { position:relative; display:inline-block; width:54px; height:30px; }
    .switch input { display:none; }
    .slider { position:absolute; cursor:pointer; inset:0; background:var(--off); transition:.2s; border-radius:30px; border:1px solid var(--btnBorder);}
    .slider:before { position:absolute; content:""; height:24px; width:24px; left:3px; bottom:3px; background:var(--thumb); transition:.2s; border-radius:50%; box-shadow:0 6px 18px rgba(0,0,0,.35); }
    input:checked + .slider { background:var(--on); border-color:rgba(34,197,94,.6); }
    input:checked + .slider:before { transform:translateX(24px); }

    button, a.btn {
      padding:10px 12px; border-radius:10px; border:1px solid var(--btnBorder);
      background:var(--btn); color:var(--text); cursor:pointer; text-decoration:none; display:inline-block;
    }
    button:active, a.btn:active { transform: scale(.99); }
    .pulseBtn { padding:9px 10px; border-radius:10px; border:1px solid rgba(34,197,94,.35); background:rgba(34,197,94,.10); color:var(--text); font-weight:900; letter-spacing:.3px; }
    .btnrow { display:flex; gap:10px; margin-top:14px; flex-wrap:wrap; }
    .small { color:var(--muted); font-size:13px; margin-top:10px; }
  </style>
</head>
<body>
  <div class="card">
    <div class="title">
      <span>UNO R4 WiFi - Control</span>
      <a class="btn" href="/configure">Configure</a>
    </div>

    <div class="meta">
      <div class="pill" id="ipPill">IP: …</div>
      <div class="pill" id="timePill">Time: …</div>
      <div class="pill" id="rssiPill">RSSI: …</div>
    </div>

    <div class="divider"></div>
    <div id="relays"></div>

    <div class="btnrow">
      <button onclick="setAll(1)">All ON</button>
      <button onclick="setAll(0)">All OFF</button>
      <button onclick="refresh()">Refresh</button>
    </div>

    <div class="small" id="status">Loading…</div>
  </div>

<script>
  const relaysDiv = document.getElementById('relays');
  const statusDiv = document.getElementById('status');
  const ipPill = document.getElementById('ipPill');
  const timePill = document.getElementById('timePill');
  const rssiPill = document.getElementById('rssiPill');

  let names = Array.from({length:8}, (_,i)=>`Channel ${i+1}`);

  function makeRow(i) {
    const ch = i + 1;
    const row = document.createElement('div');
    row.className = 'row';
    row.innerHTML = `
      <div class="left">
        <div class="ch">${names[i]}</div>
        <div class="hint">Toggle = latch • Pulse = momentary • Schedules may force ON</div>
      </div>
      <div class="right">
        <button class="pulseBtn" onclick="pulse(${ch})">PULSE</button>
        <label class="switch">
          <input type="checkbox" id="ch${ch}" onchange="toggleRelay(${ch}, this.checked)">
          <span class="slider"></span>
        </label>
      </div>
    `;
    return row;
  }

  function rebuild() {
    relaysDiv.innerHTML = '';
    for (let i=0;i<8;i++) relaysDiv.appendChild(makeRow(i));
  }

  async function toggleRelay(ch, on) {
    statusDiv.textContent = `Setting ${names[ch-1]} ${on ? 'ON' : 'OFF'}…`;
    try {
      const r = await fetch(`/api/set?ch=${ch}&on=${on ? 1 : 0}`);
      statusDiv.textContent = await r.text();
    } catch {
      statusDiv.textContent = 'Error talking to controller.';
    }
  }

  async function pulse(ch) {
    statusDiv.textContent = `Pulsing ${names[ch-1]}…`;
    try {
      const r = await fetch(`/api/pulse?ch=${ch}`);
      statusDiv.textContent = await r.text();
      await refresh();
    } catch {
      statusDiv.textContent = 'Pulse failed.';
    }
  }

  async function setAll(on) {
    statusDiv.textContent = `Setting ALL ${on ? 'ON' : 'OFF'}…`;
    try {
      await fetch(`/api/all?on=${on ? 1 : 0}`);
      await refresh();
      statusDiv.textContent = `OK: all ${on ? 'ON' : 'OFF'}`;
    } catch {
      statusDiv.textContent = 'Error setting all relays.';
    }
  }

  async function refresh() {
    try {
      const r = await fetch('/api/status');
      const j = await r.json();
      j.relays.forEach((v,i)=>{
        const ch = i + 1;
        const el = document.getElementById(`ch${ch}`);
        if (el) el.checked = (v === 1);
      });
      statusDiv.textContent = 'Synced.';
    } catch {
      statusDiv.textContent = 'Refresh failed.';
    }
  }

  async function loadNames() {
    try {
      const r = await fetch('/api/names');
      const j = await r.json();
      names = j.names;
    } catch {}
    rebuild();
  }

  async function loadInfo() {
    try {
      const r = await fetch('/api/info');
      const j = await r.json();
      ipPill.textContent = `IP: ${j.ip}`;
      rssiPill.textContent = `RSSI: ${j.rssi}`;
      timePill.textContent = j.timeValid ? `Time: OK` : `Time: NTP…`;
    } catch {
      ipPill.textContent = 'IP: ?';
      timePill.textContent = 'Time: ?';
      rssiPill.textContent = 'RSSI: ?';
    }
  }

  (async ()=>{
    await loadNames();
    await loadInfo();
    await refresh();
    setInterval(loadInfo, 5000);
    setInterval(refresh, 3000);
  })();
</script>
</body>
</html>
)HTML";

const char* PAGE_CONFIG = R"HTML(
<!doctype html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <meta name="color-scheme" content="dark">
  <title>Configure</title>
  <style>
    :root{
      --bg:#0b0f14; --card:#121924; --border:#1f2a3a; --text:#e7edf6; --muted:#9aa7b8;
      --btn:#0f1622; --btnBorder:#223149;
    }
    body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial; margin:16px; background:var(--bg); color:var(--text); }
    .card { max-width: 720px; margin: 0 auto; padding: 16px; background:var(--card); border:1px solid var(--border); border-radius:14px; box-shadow:0 10px 30px rgba(0,0,0,.35);}
    .title { font-size:18px; font-weight:900; margin-bottom:8px; display:flex; justify-content:space-between; align-items:center; gap:10px;}
    a.btn, button {
      padding:10px 12px; border-radius:10px; border:1px solid var(--btnBorder);
      background:var(--btn); color:var(--text); cursor:pointer; text-decoration:none; display:inline-block;
    }
    button.primary { border-color: rgba(34,197,94,.45); background: rgba(34,197,94,.12); font-weight: 900; }
    .muted { color: var(--muted); font-size: 13px; margin-top: 4px; }
    .grid { display:grid; grid-template-columns: 1fr; gap: 14px; margin-top: 14px; }
    .panel { border:1px solid var(--border); border-radius:12px; padding:12px; background: rgba(255,255,255,0.02); }
    .row { display:flex; gap:10px; flex-wrap:wrap; align-items:center; }
    label { font-weight:800; }
    select, input[type="text"], input[type="number"], input[type="datetime-local"], input[type="time"]{
      padding:10px 10px; border-radius:10px; border:1px solid var(--btnBorder);
      background:var(--btn); color:var(--text); outline:none;
    }
    input[type="text"]{ min-width: 260px; }
    .status { margin-top: 10px; color: var(--muted); font-size: 13px; }
    .small { color: var(--muted); font-size: 12px; }
    .divider { height:1px; background: var(--border); margin: 12px 0; }
  </style>
</head>
<body>
  <div class="card">
    <div class="title">
      <span>Configure</span>
      <a class="btn" href="/">Back</a>
    </div>
    <div class="muted">Rename channels, set auto-off timers, and create schedules (one-time or daily).</div>

    <div class="grid">

      <div class="panel">
        <div class="row">
          <label>Channel</label>
          <select id="chSel"></select>
        </div>

        <div class="divider"></div>

        <div class="row">
          <label>Name</label>
          <input id="name" type="text" placeholder="Front Door Strike">
          <button class="primary" onclick="saveName()">Save Name</button>
        </div>
        <div class="small">Example names: “Front Door”, “Lobby LED”, “Maglock”, “Strike”.</div>

        <div class="divider"></div>

        <div class="row">
          <label>Auto-OFF</label>
          <input id="autoVal" type="number" min="0" step="1" value="0">
          <select id="autoUnit">
            <option value="sec">seconds</option>
            <option value="min">minutes</option>
            <option value="hr">hours</option>
          </select>
          <button class="primary" onclick="saveAutoOff()">Save Auto-OFF</button>
        </div>
        <div class="small">0 disables. Auto-OFF applies to manual toggles (schedule can still force ON).</div>
      </div>

      <div class="panel">
        <div style="font-weight:900; margin-bottom:8px;">One-time schedule (date/time + duration)</div>
        <div class="row">
          <label>Start</label>
          <input id="oneStart" type="datetime-local">
          <label>Duration</label>
          <input id="oneDurVal" type="number" min="1" step="1" value="10">
          <select id="oneDurUnit">
            <option value="min">minutes</option>
            <option value="sec">seconds</option>
            <option value="hr">hours</option>
          </select>
        </div>
        <div class="row" style="margin-top:10px;">
          <button class="primary" onclick="saveOneShot()">Enable One-time</button>
          <button onclick="clearOneShot()">Clear One-time</button>
        </div>
        <div class="small">Temporary unlock/energize window for a specific date/time.</div>
      </div>

      <div class="panel">
        <div style="font-weight:900; margin-bottom:8px;">Daily schedule (recurring window)</div>
        <div class="row">
          <label>Start</label>
          <input id="dayStart" type="time" value="08:00">
          <label>End</label>
          <input id="dayEnd" type="time" value="17:00">
        </div>
        <div class="row" style="margin-top:10px;">
          <button class="primary" onclick="enableDaily()">Enable Daily</button>
          <button onclick="disableDaily()">Disable Daily</button>
        </div>
        <div class="small">If Start > End, it wraps midnight (e.g., 22:00 to 06:00).</div>
      </div>

      <div class="panel">
        <div style="font-weight:900; margin-bottom:8px;">Pulse default (ms)</div>
        <div class="row">
          <label>Pulse</label>
          <input id="pulseMs" type="number" min="50" max="10000" step="50" value="800">
          <button class="primary" onclick="savePulse()">Save Pulse</button>
        </div>
        <div class="small">Pulse remains in milliseconds (strike-style momentary).</div>
      </div>

    </div>

    <div class="status" id="status">Loading…</div>
  </div>

<script>
  const statusDiv = document.getElementById('status');
  const chSel = document.getElementById('chSel');

  let names = [];
  let cfg = null;

  function secFrom(val, unit){
    val = parseInt(val||'0',10);
    if (isNaN(val) || val < 0) val = 0;
    if (unit === 'hr') return val * 3600;
    if (unit === 'min') return val * 60;
    return val;
  }

  function parseTimeToMin(t){
    const parts = (t||'00:00').split(':');
    const hh = parseInt(parts[0]||'0',10);
    const mm = parseInt(parts[1]||'0',10);
    return (hh*60) + mm;
  }

  async function api(path){
    const r = await fetch(path);
    if(!r.ok) throw new Error('HTTP');
    return r;
  }

  function fillChannels(){
    chSel.innerHTML = '';
    for(let i=1;i<=8;i++){
      const o = document.createElement('option');
      o.value = i;
      o.textContent = `${i} - ${names[i-1] || ('Channel '+i)}`;
      chSel.appendChild(o);
    }
    chSel.onchange = loadChannelFields;
  }

  function toHHMM(m){
    const hh = String(Math.floor(m/60)).padStart(2,'0');
    const mm = String(m%60).padStart(2,'0');
    return `${hh}:${mm}`;
  }

  function loadChannelFields(){
    const ch = parseInt(chSel.value,10);
    document.getElementById('name').value = names[ch-1] || ('Channel '+ch);

    const sec = (cfg && cfg.autoOffSec) ? cfg.autoOffSec[ch-1] : 0;
    let unit = 'sec', val = sec;
    if (sec % 3600 === 0 && sec >= 3600) { unit='hr'; val = sec/3600; }
    else if (sec % 60 === 0 && sec >= 60) { unit='min'; val = sec/60; }
    document.getElementById('autoUnit').value = unit;
    document.getElementById('autoVal').value = val;

    const d = cfg.daily[ch-1];
    document.getElementById('dayStart').value = toHHMM(d.s);
    document.getElementById('dayEnd').value   = toHHMM(d.e);

    document.getElementById('pulseMs').value = cfg.pulseMs;
  }

  async function loadAll(){
    statusDiv.textContent = 'Loading…';
    const n = await (await api('/api/names')).json();
    names = n.names;

    cfg = await (await api('/api/config')).json();

    fillChannels();
    loadChannelFields();
    statusDiv.textContent = 'Ready.';
  }

  async function saveName(){
    const ch = parseInt(chSel.value,10);
    const name = encodeURIComponent((document.getElementById('name').value || '').trim() || ('Channel '+ch));
    const r = await api(`/api/name/set?ch=${ch}&name=${name}`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  async function saveAutoOff(){
    const ch = parseInt(chSel.value,10);
    const sec = secFrom(document.getElementById('autoVal').value, document.getElementById('autoUnit').value);
    const r = await api(`/api/autooff/set?ch=${ch}&sec=${sec}`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  async function savePulse(){
    const ms = parseInt(document.getElementById('pulseMs').value||'800',10);
    const r = await api(`/api/pulse/set?ms=${ms}`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  async function saveOneShot(){
    const ch = parseInt(chSel.value,10);
    const startStr = document.getElementById('oneStart').value;
    if(!startStr){ statusDiv.textContent = 'Pick a start date/time.'; return; }

    const durSec = secFrom(document.getElementById('oneDurVal').value, document.getElementById('oneDurUnit').value);
    const encStart = encodeURIComponent(startStr);

    const r = await api(`/api/oneshot/set?ch=${ch}&startLocal=${encStart}&durSec=${durSec}`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  async function clearOneShot(){
    const ch = parseInt(chSel.value,10);
    const r = await api(`/api/oneshot/clear?ch=${ch}`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  async function enableDaily(){
    const ch = parseInt(chSel.value,10);
    const s = parseTimeToMin(document.getElementById('dayStart').value);
    const e = parseTimeToMin(document.getElementById('dayEnd').value);
    const r = await api(`/api/daily/set?ch=${ch}&en=1&s=${s}&e=${e}`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  async function disableDaily(){
    const ch = parseInt(chSel.value,10);
    const r = await api(`/api/daily/set?ch=${ch}&en=0&s=0&e=0`);
    statusDiv.textContent = await r.text();
    await loadAll();
  }

  loadAll().catch(()=>statusDiv.textContent='Failed to load config.');
</script>
</body>
</html>
)HTML";

// ------------------- WiFi connect with loud IP -------------------
void connectWiFiWithLoudIP() {
  Serial.println("Connecting to WiFi…");

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERROR: WiFi module not detected. Tools > Board must be UNO R4 WiFi.");
    while (true) {}
  }

  int status = WL_IDLE_STATUS;
  unsigned long start = millis();

  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(2000);
    Serial.print("WiFi status code: ");
    Serial.println(status);

    if (millis() - start > 45000) {
      Serial.println("FAILED to connect in ~45s. Check SSID/PASS and 2.4GHz WiFi.");
      Serial.println("Auto-resetting in 5 seconds...");
      delay(5000);
      NVIC_SystemReset();
    }
  }

  Serial.println("WiFi connected.");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());

  for (int i = 0; i < 20; i++) {
    Serial.print("CONNECT TO: http://");
    Serial.println(WiFi.localIP());
    delay(1000);
  }
}

// ------------------- datetime-local parse to local epoch -------------------
bool isLeap(int y) { return ((y%4==0 && y%100!=0) || (y%400==0)); }
int daysInMonth(int y,int m){
  static int d[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if(m==2) return d[m-1]+(isLeap(y)?1:0);
  return d[m-1];
}
unsigned long localComponentsToEpoch(int Y,int Mo,int D,int H,int Mi){
  unsigned long days=0;
  for(int y=1970;y<Y;y++) days += isLeap(y)?366:365;
  for(int m=1;m<Mo;m++) days += daysInMonth(Y,m);
  days += (unsigned long)(D-1);
  return days*86400UL + (unsigned long)H*3600UL + (unsigned long)Mi*60UL;
}
bool parseDateTimeLocal(const String &s, int &Y,int &Mo,int &D,int &H,int &Mi){
  if (s.length() < 16) return false;
  Y  = s.substring(0,4).toInt();
  Mo = s.substring(5,7).toInt();
  D  = s.substring(8,10).toInt();
  H  = s.substring(11,13).toInt();
  Mi = s.substring(14,16).toInt();
  if (Y < 1970 || Mo < 1 || Mo > 12 || D < 1 || D > 31 || H < 0 || H > 23 || Mi < 0 || Mi > 59) return false;
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // Init relays OFF
  for (int i=0;i<8;i++) {
    pinMode(relayPins[i], OUTPUT);
    manualState[i] = false;
    effectiveState[i] = false;
    manualOnSince[i] = 0;
    writeRelayHardware(i, false);
  }

  Serial.println("\nUNO R4 WiFi - Access-style Relay Controller");
  connectWiFiWithLoudIP();

  Serial.println("Syncing time (NTP)...");
  for (int i=0;i<5;i++) {
    if (syncTimeNtp()) break;
    delay(500);
  }
  Serial.println(timeValid ? "Time: OK" : "Time: NOT READY (will retry)");

  server.begin();
  Serial.println("HTTP server started.");
  Serial.println("Main:      /");
  Serial.println("Configure: /configure");
}

void loop() {
  // NTP refresh every ~10 minutes
  if (WiFi.status() == WL_CONNECTED) {
    if (!timeValid || (millis() - lastNtpSyncMs) > 10UL*60UL*1000UL) {
      syncTimeNtp();
    }
  }

  updateEffectiveStates();

  WiFiClient client = server.available();
  if (!client) return;

  String method, url;
  if (!readRequestLine(client, method, url)) { client.stop(); return; }
  drainHeaders(client);
  if (method != "GET") { send404(client); client.stop(); return; }

  // Pages
  if (url == "/" || url.startsWith("/?")) {
    send200(client, "text/html; charset=utf-8", String(PAGE_MAIN));
    client.stop(); return;
  }
  if (url == "/configure" || url.startsWith("/configure?")) {
    send200(client, "text/html; charset=utf-8", String(PAGE_CONFIG));
    client.stop(); return;
  }

  // APIs
  if (url.startsWith("/api/status")) { send200Json(client, jsonStatus()); client.stop(); return; }
  if (url.startsWith("/api/info"))   { send200Json(client, jsonInfo());   client.stop(); return; }
  if (url.startsWith("/api/names"))  { send200Json(client, jsonNames());  client.stop(); return; }
  if (url.startsWith("/api/config")) { send200Json(client, jsonConfig()); client.stop(); return; }

  if (url.startsWith("/api/set")) {
    long ch=-1,on=-1;
    bool ok1 = getQueryInt(url, "ch", ch);
    bool ok2 = getQueryInt(url, "on", on);
    if (ok1 && ok2 && ch>=1 && ch<=8 && (on==0||on==1)) {
      setManual((int)ch, on==1);
      send200Text(client, "OK: CH" + String(ch) + (on==1 ? " ON" : " OFF"));
    } else send200Text(client, "ERR: /api/set?ch=1..8&on=0|1");
    client.stop(); return;
  }

  if (url.startsWith("/api/all")) {
    long on=-1;
    if (getQueryInt(url, "on", on) && (on==0||on==1)) {
      for (int ch=1; ch<=8; ch++) setManual(ch, on==1);
      send200Text(client, String("OK: all ") + (on==1?"ON":"OFF"));
    } else send200Text(client, "ERR: /api/all?on=0|1");
    client.stop(); return;
  }

  if (url.startsWith("/api/pulse?")) {
    long ch=-1, ms=cfgPulseMs;
    bool ok1 = getQueryInt(url, "ch", ch);
    getQueryInt(url, "ms", ms);
    if (ok1 && ch>=1 && ch<=8) {
      if (ms < 50) ms = 50;
      if (ms > 10000) ms = 10000;
      pulseRelay((int)ch, (int)ms);
      send200Text(client, "OK: PULSED CH" + String(ch) + " for " + String(ms) + "ms");
    } else send200Text(client, "ERR: /api/pulse?ch=1..8&ms=50..10000");
    client.stop(); return;
  }

  if (url.startsWith("/api/pulse/set")) {
    long ms=cfgPulseMs;
    if (getQueryInt(url, "ms", ms)) {
      if (ms < 50) ms = 50;
      if (ms > 10000) ms = 10000;
      cfgPulseMs = (int)ms;
      send200Text(client, "OK: pulseMs=" + String(cfgPulseMs));
    } else send200Text(client, "ERR: /api/pulse/set?ms=50..10000");
    client.stop(); return;
  }

  if (url.startsWith("/api/name/set")) {
    long ch=-1;
    String name;
    bool ok1 = getQueryInt(url, "ch", ch);
    bool ok2 = getQueryStr(url, "name", name);
    if (ok1 && ok2 && ch>=1 && ch<=8) {
      name.trim();
      if (name.length() == 0) name = "Channel " + String((int)ch);
      if (name.length() > 32) name = name.substring(0, 32);
      chName[ch-1] = name;
      send200Text(client, "OK: name set");
    } else send200Text(client, "ERR: /api/name/set?ch=1..8&name=Front%20Door");
    client.stop(); return;
  }

  if (url.startsWith("/api/autooff/set")) {
    long ch=-1, sec=0;
    bool ok1 = getQueryInt(url, "ch", ch);
    bool ok2 = getQueryInt(url, "sec", sec);
    if (ok1 && ok2 && ch>=1 && ch<=8 && sec >= 0) {
      if (sec > 3600000L) sec = 3600000L;
      autoOffSec[ch-1] = (unsigned long)sec;
      send200Text(client, "OK: autoOffSec=" + String(autoOffSec[ch-1]));
    } else send200Text(client, "ERR: /api/autooff/set?ch=1..8&sec=0..");
    client.stop(); return;
  }

  if (url.startsWith("/api/oneshot/set")) {
    long ch=-1, dur=0;
    String startStr;
    bool ok1 = getQueryInt(url, "ch", ch);
    bool ok2 = getQueryStr(url, "startLocal", startStr);
    bool ok3 = getQueryInt(url, "durSec", dur);

    if (ok1 && ok2 && ok3 && ch>=1 && ch<=8 && dur>0) {
      int Y,Mo,D,H,Mi;
      if (!parseDateTimeLocal(startStr, Y,Mo,D,H,Mi)) {
        send200Text(client, "ERR: bad startLocal format");
        client.stop(); return;
      }
      unsigned long startLocalEpoch = localComponentsToEpoch(Y,Mo,D,H,Mi);
      oneShotEnabled[ch-1] = true;
      oneShotStartLocal[ch-1] = startLocalEpoch;
      oneShotDurSec[ch-1] = (unsigned long)dur;
      send200Text(client, "OK: one-shot enabled");
    } else send200Text(client, "ERR: /api/oneshot/set?ch=1..8&startLocal=YYYY-MM-DDTHH:MM&durSec=...");
    client.stop(); return;
  }

  if (url.startsWith("/api/oneshot/clear")) {
    long ch=-1;
    if (getQueryInt(url, "ch", ch) && ch>=1 && ch<=8) {
      oneShotEnabled[ch-1] = false;
      oneShotStartLocal[ch-1] = 0;
      oneShotDurSec[ch-1] = 0;
      send200Text(client, "OK: one-shot cleared");
    } else send200Text(client, "ERR: /api/oneshot/clear?ch=1..8");
    client.stop(); return;
  }

  if (url.startsWith("/api/daily/set")) {
    long ch=-1,en=0,s=0,e=0;
    bool ok1 = getQueryInt(url, "ch", ch);
    bool ok2 = getQueryInt(url, "en", en);
    bool ok3 = getQueryInt(url, "s", s);
    bool ok4 = getQueryInt(url, "e", e);
    if (ok1 && ok2 && ok3 && ok4 && ch>=1 && ch<=8) {
      if (s < 0) s = 0; if (s > 1439) s = 1439;
      if (e < 0) e = 0; if (e > 1439) e = 1439;
      dailyEnabled[ch-1] = (en==1);
      dailyStartMin[ch-1] = (int)s;
      dailyEndMin[ch-1] = (int)e;
      send200Text(client, "OK: daily schedule updated");
    } else send200Text(client, "ERR: /api/daily/set?ch=1..8&en=0|1&s=0..1439&e=0..1439");
    client.stop(); return;
  }

  send404(client);
  client.stop();
}
