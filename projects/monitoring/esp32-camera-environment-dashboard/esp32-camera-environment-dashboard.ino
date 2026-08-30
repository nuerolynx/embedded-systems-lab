#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include "arduino_secrets.h"
#include "esp_http_server.h"

// ===== WiFi =====
// Wi-Fi values are loaded from the ignored arduino_secrets.h file.

// ===== DHT =====
#define DHTTYPE DHT11
#define DHTPIN  14   // Recommended. Change to 15 if you insist.
DHT dht(DHTPIN, DHTTYPE);

// ===== Stream boundary (camera server :81) =====
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY     = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ===== SunFounder Camera Extension (your working pin map) =====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===== Servers =====
WebServer uiServer(80);
httpd_handle_t stream_httpd = NULL;   // camera stream on :81

bool readDHT(float &tC, float &tF, float &h) {
  h  = dht.readHumidity();
  tC = dht.readTemperature();
  tF = dht.readTemperature(true);
  if (isnan(h) || isnan(tC) || isnan(tF)) return false;
  return true;
}

// ===== iPhone no-scroll UI =====
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>ESP32 Env Panel</title>
<style>
:root{
  --bg:#070b18;
  --panel:rgba(17,26,51,.78);
  --stroke:rgba(255,255,255,.08);
  --text:#e9eeff;
  --muted:rgba(233,238,255,.70);
  --good:#3ee6b2;
  --bad:#ff5d7a;
  --shadow:0 14px 40px rgba(0,0,0,.55);
  --r:18px;
}

*{box-sizing:border-box;}
html,body{height:100%; overflow:hidden;}  /* important: no scroll */
body{
  margin:0;
  padding: max(10px, env(safe-area-inset-top)) 12px max(10px, env(safe-area-inset-bottom));
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif;
  color:var(--text);
  background:
    radial-gradient(900px 600px at 20% 10%, rgba(74,144,226,.18), transparent 60%),
    radial-gradient(800px 500px at 80% 90%, rgba(255,77,109,.12), transparent 55%),
    radial-gradient(600px 400px at 60% 30%, rgba(62,230,178,.10), transparent 60%),
    var(--bg);
}

.wrap{
  height:100%;
  max-width:720px;
  margin:0 auto;
  display:flex;
  flex-direction:column;
  gap:10px;
}

.top{
  display:flex;justify-content:space-between;align-items:center;gap:10px;
}
.title{display:flex;flex-direction:column;gap:2px;min-width:0;}
h1{margin:0;font-size:16px;letter-spacing:.2px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}
.sub{font-size:11px;color:var(--muted);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}
.pill{
  display:inline-flex;align-items:center;gap:8px;
  padding:7px 10px;border-radius:999px;
  background:rgba(17,26,51,.55);
  border:1px solid var(--stroke);
  box-shadow:0 8px 22px rgba(0,0,0,.25);
  font-size:11px;color:var(--muted);white-space:nowrap;
}
.dot{width:10px;height:10px;border-radius:999px;background:rgba(255,255,255,.35);}
.dot.good{background:var(--good);box-shadow:0 0 16px rgba(62,230,178,.35);}
.dot.bad{background:var(--bad);box-shadow:0 0 16px rgba(255,93,122,.35);}

.card{
  border-radius:var(--r);
  background:linear-gradient(180deg, rgba(17,26,51,.80), rgba(15,23,48,.60));
  border:1px solid var(--stroke);
  box-shadow:var(--shadow);
  overflow:hidden;
}
.tempGlow{ transition: box-shadow .45s ease, border-color .45s ease; }

/* CAM: constrained height so it fits on phone */
.camWrap{display:flex;flex-direction:column;}
.camHeader{
  display:flex;justify-content:space-between;align-items:center;gap:10px;
  padding:8px 10px;
  border-bottom:1px solid var(--stroke);
  font-size:11px;color:var(--muted);
}
button{
  appearance:none;border:none;cursor:pointer;
  padding:7px 10px;border-radius:12px;
  color:rgba(233,238,255,.92);
  background:rgba(122,168,255,.14);
  border:1px solid rgba(122,168,255,.22);
  font-weight:700;
  font-size:12px;
}
button:active{transform:translateY(1px);}

/* Use a fixed-ish viewport-friendly height for the camera area */
.camStage{
  width:100%;
  height: min(42vh, 340px);  /* key: prevents scrolling */
  background:#000;
  position:relative;
}
.cam{
  position:absolute; inset:0;
  width:100%; height:100%;
  object-fit:cover;
  display:block;
}

/* ENV card */
.envPad{padding:10px;}
.envTop{
  display:flex;align-items:flex-end;justify-content:space-between;gap:10px;flex-wrap:wrap;
}
.envLabel{font-size:11px;color:var(--muted);}
.readings{display:flex;gap:10px;flex-wrap:wrap;align-items:baseline;}
.reading{
  display:flex;flex-direction:column;gap:2px;
  padding:6px 10px;border-radius:14px;
  border:1px solid var(--stroke);
  background:rgba(0,0,0,.14);
}
.valueRow{display:flex;align-items:baseline;gap:6px;}
.big{font-size:28px;font-weight:900;line-height:1;}
.unit{font-size:12px;color:var(--muted);font-weight:800;}
.small{font-size:11px;color:var(--muted);}

.badges{margin-top:8px;display:flex;gap:8px;flex-wrap:wrap;}
.badge{
  padding:6px 10px;border-radius:999px;
  border:1px solid var(--stroke);
  background:rgba(0,0,0,.14);
  font-size:11px;color:var(--muted);
}

/* Spectrum bar (shorter) */
.tbarWrap{
  margin-top:10px;
  position:relative;
  height:12px;
  border-radius:999px;
  border:1px solid var(--stroke);
  background:rgba(0,0,0,.18);
  overflow:hidden;
}
.tbar{
  position:absolute; inset:0;
  background: linear-gradient(90deg,#1b4bff 0%,#00c6ff 18%,#00ff9d 40%,#ffd84d 60%,#ff8a3d 78%,#ff2d55 100%);
  opacity:.9;
}
.tmarker{
  position:absolute;
  top:-5px;
  width:16px;height:22px;
  transform:translateX(-8px);
  border-radius:10px;
  border:1px solid rgba(255,255,255,.25);
  background:rgba(0,0,0,.30);
  backdrop-filter: blur(6px);
  box-shadow:0 8px 18px rgba(0,0,0,.35);
  transition:left .40s ease;
}
.tmarker:after{
  content:"";
  position:absolute;left:50%;bottom:5px;transform:translateX(-50%);
  width:6px;height:6px;border-radius:99px;
  background:rgba(255,255,255,.85);
  box-shadow:0 0 10px rgba(255,255,255,.35);
}

/* Humidity bar */
.hWrap{
  margin-top:10px; height:12px; border-radius:999px;
  border:1px solid var(--stroke);
  background:rgba(0,0,0,.18);
  overflow:hidden; position:relative;
}
.hFill{
  height:100%; width:0%;
  border-radius:999px;
  background: linear-gradient(90deg, rgba(122,168,255,.30), rgba(62,230,178,.45));
  box-shadow:0 0 18px rgba(122,168,255,.18);
  transition: width .55s cubic-bezier(.22,.9,.22,1);
}
.hPulse{
  position:absolute; inset:0;
  background: linear-gradient(90deg, transparent, rgba(255,255,255,.12), transparent);
  transform: translateX(-100%);
  animation: sweep 2.8s linear infinite;
  opacity:.35; pointer-events:none;
}
@keyframes sweep{0%{transform:translateX(-100%);}100%{transform:translateX(100%);}}

/* Chart: compact so it fits */
.chartWrap{
  margin-top:10px;
  border-radius:16px;
  border:1px solid var(--stroke);
  background:rgba(0,0,0,.16);
  padding:10px;
}
canvas{width:100%; height:110px; display:block;}
.legend{
  margin-top:8px;
  display:flex;gap:10px;flex-wrap:wrap;
  color:var(--muted);font-size:11px;
}
.key{display:flex;align-items:center;gap:8px;padding:6px 10px;border-radius:999px;border:1px solid var(--stroke);background:rgba(0,0,0,.10);}
.swatch{width:10px;height:10px;border-radius:99px;}
.swatch.t{background:#ffd84d; box-shadow:0 0 12px rgba(255,216,77,.25);}
.swatch.h{background:#7aa8ff; box-shadow:0 0 12px rgba(122,168,255,.25);}

.footer{
  margin-top:8px;
  display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap;
  color:var(--muted);font-size:11px;
}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="title">
      <h1>ESP32 Env Panel</h1>
      <div class="sub">Camera + Temp/Humidity + History (No scroll)</div>
    </div>
    <div class="pill"><span id="dot" class="dot"></span><span id="status">Connecting…</span></div>
  </div>

  <!-- Camera card -->
  <div class="card camWrap">
    <div class="camHeader">
      <div>Camera Stream</div>
      <button onclick="reloadStream()">Reload</button>
    </div>
    <div class="camStage">
      <img class="cam" id="cam" src="" alt="camera stream">
    </div>
  </div>

  <!-- Environment card -->
  <div class="card tempGlow" id="envCard">
    <div class="envPad">
      <div class="envTop">
        <div>
          <div class="envLabel">Environment</div>
          <div class="small">Heat: <b id="comfort">--</b></div>
        </div>
        <div class="readings">
          <div class="reading">
            <div class="small">Temp</div>
            <div class="valueRow"><span class="big" id="tempF">--</span><span class="unit">°F</span></div>
            <div class="small">°C <span id="tempC">--</span></div>
          </div>
          <div class="reading">
            <div class="small">Humidity</div>
            <div class="valueRow"><span class="big" id="hum">--</span><span class="unit">%</span></div>
            <div class="small">State <span id="humState">--</span></div>
          </div>
        </div>
      </div>

      <div class="badges">
        <div class="badge">Temp Marker</div>
        <div class="badge">Humidity Bar</div>
      </div>

      <div class="tbarWrap">
        <div class="tbar"></div>
        <div class="tmarker" id="tmarker" style="left:0%"></div>
      </div>

      <div class="hWrap">
        <div class="hFill" id="hFill"></div>
        <div class="hPulse"></div>
      </div>

      <div class="chartWrap">
        <canvas id="chart" width="800" height="260"></canvas>
        <div class="legend">
          <div class="key"><span class="swatch t"></span>Temp (°F)</div>
          <div class="key"><span class="swatch h"></span>Humidity (%)</div>
        </div>
      </div>

      <div class="footer">
        <div>Updated: <span id="updated">--</span></div>
        <div>IP: <span id="ip">--</span></div>
      </div>
    </div>
  </div>
</div>

<script>
const MAX_POINTS = 80;           // fits chart nicely
let tHist=[], hHist=[];
const T_MIN = 20, T_MAX = 110;   // marker/hue mapping scale

function clamp(x,min,max){ return Math.max(min, Math.min(max, x)); }
function lerp(a,b,t){ return a + (b-a)*t; }
function fmt1(x){ return (x===null||x===undefined||Number.isNaN(Number(x))) ? "--" : Number(x).toFixed(1); }

function setStatus(ok, text){
  const dot = document.getElementById('dot');
  dot.classList.remove('good','bad');
  if(ok===true) dot.classList.add('good');
  if(ok===false) dot.classList.add('bad');
  document.getElementById('status').textContent = text;
}

function updateTimestamp(){
  document.getElementById('updated').textContent = new Date().toLocaleString();
}

function tempToHue(tF){
  const p = clamp((tF - T_MIN)/(T_MAX - T_MIN), 0, 1);
  return lerp(210, 0, p); // blue->red
}

function comfortLabel(tF){
  if (tF < 32) return "Freezing";
  if (tF < 50) return "Cold";
  if (tF < 66) return "Cool";
  if (tF < 76) return "Comfortable";
  if (tF < 86) return "Warm";
  return "Hot";
}

function humidityState(h){
  if (h < 30) return "Dry";
  if (h < 60) return "Normal";
  return "Humid";
}

// chart draws BOTH lines (Temp + Humidity) in a single chart
function drawChart(){
  const c=document.getElementById('chart');
  const ctx=c.getContext('2d');

  const cssW=c.clientWidth||800;
  const cssH=c.clientHeight||110;
  const dpr=window.devicePixelRatio||1;
  c.width=Math.floor(cssW*dpr);
  c.height=Math.floor(cssH*dpr);
  ctx.setTransform(dpr,0,0,dpr,0,0);

  const W=cssW, H=cssH;
  ctx.clearRect(0,0,W,H);

  // grid
  ctx.strokeStyle="rgba(255,255,255,0.07)";
  ctx.lineWidth=1;
  for(let i=1;i<4;i++){
    const y=(H*i)/4;
    ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke();
  }

  if(tHist.length<2) return;

  // temp scale auto-fit with guardrails
  const tMin=Math.min(...tHist, 45);
  const tMax=Math.max(...tHist, 95);

  function xFor(i,n){ return (W*i)/(n-1); }
  function yFor(val,min,max){
    const p=(val-min)/((max-min)||1);
    return H - clamp(p,0,1)*H;
  }

  // TEMP line
  ctx.lineWidth=2;
  ctx.strokeStyle="rgba(255,216,77,0.92)";
  ctx.beginPath();
  for(let i=0;i<tHist.length;i++){
    const x=xFor(i,tHist.length);
    const y=yFor(tHist[i], tMin, tMax);
    if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  // HUM line (0..100)
  ctx.strokeStyle="rgba(122,168,255,0.88)";
  ctx.beginPath();
  for(let i=0;i<hHist.length;i++){
    const x=xFor(i,hHist.length);
    const y=yFor(hHist[i], 0, 100);
    if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();
}
window.addEventListener('resize', drawChart);

function pushHistory(tF,h){
  tHist.push(tF); hHist.push(h);
  if(tHist.length>MAX_POINTS) tHist.shift();
  if(hHist.length>MAX_POINTS) hHist.shift();
  drawChart();
}

// Camera stream on :81
function setCamSrc(){
  const base = window.location.origin.replace(/:\d+$/, '');
  document.getElementById('cam').src = base + ":81/stream";
}
function reloadStream(){
  const base = window.location.origin.replace(/:\d+$/, '');
  document.getElementById('cam').src = base + ":81/stream?ts=" + Date.now();
}

async function refresh(){
  try{
    const r=await fetch('/json',{cache:"no-store"});
    const j=await r.json();
    if(j.ok){
      const tF=Number(j.tempF), tC=Number(j.tempC), h=Number(j.humidity);

      document.getElementById('tempF').textContent = fmt1(tF);
      document.getElementById('tempC').textContent = fmt1(tC);
      document.getElementById('hum').textContent   = fmt1(h);
      document.getElementById('humState').textContent = humidityState(h);
      document.getElementById('comfort').textContent = comfortLabel(tF);
      document.getElementById('ip').textContent = j.ip || "--";

      // humidity bar
      document.getElementById('hFill').style.width = clamp(h,0,100) + "%";

      // spectrum glow + marker
      const hue=tempToHue(tF);
      const glow=`0 0 34px hsla(${hue},95%,60%,.30), 0 0 70px hsla(${hue},95%,55%,.16)`;
      const card=document.getElementById('envCard');
      card.style.boxShadow = `var(--shadow), ${glow}`;
      card.style.borderColor = `hsla(${hue},90%,60%,.25)`;

      const p = clamp((tF - T_MIN)/(T_MAX - T_MIN), 0, 1) * 100;
      document.getElementById('tmarker').style.left = p + "%";

      pushHistory(tF,h);
      setStatus(true,"Online");
      updateTimestamp();
    } else {
      setStatus(false,"Sensor error");
    }
  }catch(e){
    setStatus(false,"Disconnected");
  }
}

setStatus(null,"Connecting…");
setCamSrc();
drawChart();
refresh();
setInterval(refresh, 2000);
</script>
</body>
</html>
)rawliteral";

// ===== /json =====
void handleJson() {
  float tC, tF, h;
  bool ok = readDHT(tC, tF, h);

  String json = "{";
  json += "\"ok\":" + String(ok ? "true" : "false") + ",";
  json += "\"tempC\":" + (ok ? String(tC, 1) : "null") + ",";
  json += "\"tempF\":" + (ok ? String(tF, 1) : "null") + ",";
  json += "\"humidity\":" + (ok ? String(h, 1) : "null") + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";

  uiServer.send(200, "application/json", json);
}

// ===== Camera stream handler (:81/stream) =====
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK) return res;

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    char part_buf[64];
    size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, fb->len);

    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if(res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    if(res == ESP_OK) res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

    esp_camera_fb_return(fb);
    fb = NULL;

    if(res != ESP_OK) break;

    vTaskDelay(pdMS_TO_TICKS(120)); // stable for no-PSRAM
  }
  return res;
}

void startStreamServer81(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = 32768;
  httpd_uri_t stream_uri = { .uri="/stream", .method=HTTP_GET, .handler=stream_handler, .user_ctx=NULL };
  if(httpd_start(&stream_httpd, &config) == ESP_OK){
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

bool initCameraNoPsramStable(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;

  config.xclk_freq_hz = 10000000; // 10MHz
  config.pixel_format = PIXFORMAT_JPEG;

  // NO-PSRAM safe
  config.frame_size   = FRAMESIZE_QQVGA; // 160x120
  config.jpeg_quality = 20;
  config.fb_count     = 1;

  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  dht.begin();

  Serial.printf("PSRAM: %s\n", psramFound() ? "YES" : "NO");
  Serial.println("Init camera...");
  if(!initCameraNoPsramStable()){
    Serial.println("Camera failed.");
    while(true) delay(1000);
  }
  Serial.println("Camera OK.");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  startStreamServer81();

  uiServer.on("/", [](){ uiServer.send_P(200, "text/html", INDEX_HTML); });
  uiServer.on("/json", handleJson);
  uiServer.begin();

  Serial.println("Open UI: http://<ESP32_IP>/");
  Serial.println("Stream : http://<ESP32_IP>:81/stream");
}

void loop() {
  uiServer.handleClient();
}
