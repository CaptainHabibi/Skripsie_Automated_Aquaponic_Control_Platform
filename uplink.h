#ifndef UPLINK_H
#define UPLINK_H

#include "common.h"

struct LogRow {
  uint32_t ts_ms;
  char power[8];
  float temp_c;
  float water_cm;
  float ph;
  float ec;
  float ntu;
  char fish[12];
  int rssi;
  float snr;
};

/* Ultra-light struct just for the 1800-point chart history */
struct ChartPoint {
  uint32_t ts;
  float temp;
  float ph;
  float ec;
  float ntu;
};

/* Raw ring: latest only, feeds /api/latest and DSP */
LogRow rawRing[RING_SIZE];
uint16_t rawHead = 0;
uint16_t rawCount = 0;

/* Chart ring: 1 hour at 2-second points = 1800 points */
const uint16_t CHART_RING_SIZE = 1800;
ChartPoint chartRing[CHART_RING_SIZE];
uint16_t chartHead = 0;
uint16_t chartCount = 0;

/* Link supervision */
static uint32_t lastUplinkMs = 0;
static bool linkWasUp = false;
static bool batteryAssumed = false;
static char rxBuf[256];

/* ================================================================
 * CSV
 * ================================================================ */
void appendCsvHeaderIfNew() {
  if (!LittleFS.exists(CSV_PATH)) {
    File f = LittleFS.open(CSV_PATH, "w");
    if (!f) return;
    f.println("timestamp_ms,power,temp_c,water_cm,ph,ec_ms_cm,ntu,fish,rssi_dbm,snr_db");
    f.close();
  }
}

void appendCsvRow(const LogRow& r) {
  File f = LittleFS.open(CSV_PATH, "a");
  if (!f) return;
  if (f.size() > CSV_MAX_BYTES) {
    f.close();
    LittleFS.remove(CSV_PATH);
    appendCsvHeaderIfNew();
    f = LittleFS.open(CSV_PATH, "a");
    if (!f) return;
  }
  f.printf("%lu,%s,%.2f,%.2f,%.2f,%.2f,%.1f,%s,%d,%.2f\n",
           (unsigned long)r.ts_ms, r.power, r.temp_c, r.water_cm,
           r.ph, r.ec, r.ntu, r.fish, r.rssi, r.snr);
  f.close();
}

/* ================================================================
 * Rings & DSP
 * ================================================================ */
void pushRaw(const LogRow& r) {
  rawRing[rawHead] = r;
  rawHead = (rawHead + 1) % RING_SIZE;
  if (rawCount < RING_SIZE) rawCount++;
}

void pushChartWithDSP(uint32_t ts) {
  /* 5-point trailing moving average calculated instantly */
  float sT = 0, sP = 0, sE = 0, sN = 0;
  uint8_t k = 0;
  uint8_t win = (rawCount < MOVAVG_WINDOW) ? rawCount : MOVAVG_WINDOW;
  
  for (uint8_t i = 0; i < win; i++) {
    uint16_t idx = (rawHead + RING_SIZE - 1 - i) % RING_SIZE;
    if (!isnan(rawRing[idx].temp_c)) {
      sT += rawRing[idx].temp_c;
      sP += rawRing[idx].ph;
      sE += rawRing[idx].ec;
      sN += rawRing[idx].ntu;
      k++;
    }
  }

  ChartPoint cp;
  cp.ts = ts;
  if (k > 0) {
    cp.temp = sT / k;
    cp.ph = sP / k;
    cp.ec = sE / k;
    cp.ntu = sN / k;
  } else {
    cp.temp = cp.ph = cp.ec = cp.ntu = NAN;
  }

  chartRing[chartHead] = cp;
  chartHead = (chartHead + 1) % CHART_RING_SIZE;
  if (chartCount < CHART_RING_SIZE) chartCount++;
}

/* ================================================================
 * Link supervision
 * ================================================================ */
void linkWatchTick() {
  if (!linkWasUp) return;
  if (batteryAssumed) return;

  if ((millis() - lastUplinkMs) >= LINK_LOST_MS) {
    batteryAssumed = true;
    LogRow fake;
    memset(&fake, 0, sizeof(fake));
    fake.ts_ms = millis();
    strncpy(fake.power, "BATT", sizeof(fake.power) - 1);
    fake.temp_c = NAN;
    fake.water_cm = NAN;
    fake.ph = NAN;
    fake.ec = NAN;
    fake.ntu = NAN;
    strncpy(fake.fish, "Unknown", sizeof(fake.fish) - 1);

    appendCsvRow(fake);
    pushRaw(fake);
    Serial.println("[LINK] timeout -> BATT");
  }
}

/* ================================================================
 * Parser
 * ================================================================ */
static bool extractAfter(const char* src, const char* label, char* out, size_t outLen) {
  const char* p = strstr(src, label);
  if (!p) return false;
  p = strchr(p, ':');
  if (!p) return false;
  p++;
  while (*p == ' ' || *p == '\t') p++;
  const char* e = p;
  while (*e && *e != '\n' && *e != '\r') e++;
  size_t n = (size_t)(e - p);
  while (n > 0 && (p[n-1] == ' ' || p[n-1] == '\t')) n--;
  if (n == 0) return false;
  if (n >= outLen) n = outLen - 1;
  memcpy(out, p, n);
  out[n] = '\0';
  return true;
}

static void cleanStr(char* s) {
  for (; *s; s++) {
    if ((unsigned char)*s < 32 || (unsigned char)*s > 126 ||
        *s == '"' || *s == '\\' || *s == ',') *s = '_';
  }
}

bool parsePacketToRow(const char* pkt, int rssi, float snr, LogRow& out) {
  char v[32];
  memset(&out, 0, sizeof(out));
  out.ts_ms = millis();
  out.rssi = rssi;
  out.snr = snr;

  if (!extractAfter(pkt, "Power", out.power, sizeof(out.power))) return false;
  if (!extractAfter(pkt, "Temp", v, sizeof(v))) return false; out.temp_c = atof(v);
  if (!extractAfter(pkt, "Water", v, sizeof(v))) return false; out.water_cm = atof(v);
  if (!extractAfter(pkt, "pH", v, sizeof(v))) return false; out.ph = atof(v);
  if (!extractAfter(pkt, "EC", v, sizeof(v))) return false; out.ec = atof(v);
  if (!extractAfter(pkt, "NTU", v, sizeof(v))) return false; out.ntu = atof(v);
  if (!extractAfter(pkt, "Fish", out.fish, sizeof(out.fish))) return false;

  cleanStr(out.power);
  cleanStr(out.fish);
  return true;
}

/* ================================================================
 * Dashboard HTML
 * ================================================================ */
void handleUplinkRoot() {
  static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Aquaponic Hub</title>
<script src="/chart.js"></script>
<style>
body{font-family:-apple-system,BlinkMacSystemFont,"SF Pro Text","Segoe UI",Roboto,sans-serif;margin:20px;color:#f5f5f7;background:radial-gradient(circle at 0% 0%,#2a2d43 0%,#0e1621 100%);background-attachment:fixed;line-height:1.5}
h1{font-size:28px;margin:0 0 20px;font-weight:700;letter-spacing:-0.5px}
.nav{margin-bottom:24px;display:flex;gap:16px;align-items:center}
button{background:rgba(10,132,255,0.8);color:#fff;border:1px solid rgba(255,255,255,0.15);padding:10px 20px;border-radius:10px;cursor:pointer;font-size:15px;font-weight:500;backdrop-filter:blur(10px);-webkit-backdrop-filter:blur(10px);transition:all 0.2s ease;box-shadow:0 4px 12px rgba(0,0,0,0.15)}
button:active{transform:scale(0.96);opacity:0.8}
a{color:#0a84ff;text-decoration:none;font-weight:500;font-size:15px;transition:opacity 0.2s}
a:hover{opacity:0.8}
.glass{background:rgba(255,255,255,0.03);backdrop-filter:blur(24px);-webkit-backdrop-filter:blur(24px);border:1px solid rgba(255,255,255,0.08);border-radius:16px;box-shadow:0 8px 32px rgba(0,0,0,0.2)}
.row{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:16px;margin-bottom:20px}
.card{padding:18px;display:flex;flex-direction:column;justify-content:center}
.card .k{font-size:12px;color:#86868b;text-transform:uppercase;letter-spacing:1px;font-weight:600;margin-bottom:6px}
.card .v{font-size:28px;font-weight:700;letter-spacing:-0.5px;color:#fff}
.charts{display:grid;grid-template-columns:1fr 1fr;gap:16px}
.chart-box{padding:18px;height:260px;display:flex;flex-direction:column}
.chart-title{font-size:14px;margin:0 0 12px;font-weight:600;flex:0 0 auto;letter-spacing:0.3px}
.canvas-wrap{position:relative;flex:1 1 auto;min-height:0}
.canvas-wrap canvas{position:absolute;top:0;left:0;width:100%;height:100%}
@media (max-width:600px){.charts{grid-template-columns:1fr}.chart-box{height:220px}}
</style></head><body>
<h1>Aquaponic Bio-Grid</h1>
<div class="nav">
  <button onclick="goManual()">Manual Control</button>
  <a href="/api/log.csv" download>Download CSV</a>
</div>
<div class="row">
  <div class="card glass"><div class="k">Power</div><div class="v" id="v-power">--</div></div>
  <div class="card glass"><div class="k">Temp</div><div class="v" id="v-temp">--</div></div>
  <div class="card glass"><div class="k">Water</div><div class="v" id="v-water">--</div></div>
  <div class="card glass"><div class="k">pH</div><div class="v" id="v-ph">--</div></div>
  <div class="card glass"><div class="k">EC</div><div class="v" id="v-ec">--</div></div>
  <div class="card glass"><div class="k">NTU</div><div class="v" id="v-ntu">--</div></div>
  <div class="card glass"><div class="k">Fish</div><div class="v" id="v-fish">--</div></div>
  <div class="card glass"><div class="k">RSSI</div><div class="v" id="v-rssi">--</div></div>
</div>

<div class="charts">
  <div class="chart-box glass">
    <div class="chart-title" style="color:#ff7b72">Temperature (C)</div>
    <div class="canvas-wrap"><canvas id="chart-temp"></canvas></div>
  </div>
  <div class="chart-box glass">
    <div class="chart-title" style="color:#79c0ff">pH</div>
    <div class="canvas-wrap"><canvas id="chart-ph"></canvas></div>
  </div>
  <div class="chart-box glass">
    <div class="chart-title" style="color:#7ee787">EC (mS/cm)</div>
    <div class="canvas-wrap"><canvas id="chart-ec"></canvas></div>
  </div>
  <div class="chart-box glass">
    <div class="chart-title" style="color:#d2a8ff">Turbidity (NTU)</div>
    <div class="canvas-wrap"><canvas id="chart-ntu"></canvas></div>
  </div>
</div>

<script>
const haveChart = (typeof Chart !== 'undefined');

// Smoother chart styling to match the Apple aesthetic
const baseOpts = () => ({
  animation: false, responsive: true, maintainAspectRatio: false,
  plugins: { legend: { display: false } },
  scales: {
    y: { border: { display: false }, grid: { color: 'rgba(255,255,255,0.04)' }, ticks: { color: '#86868b', font: { size: 11 } } },
    x: { border: { display: false }, grid: { color: 'rgba(255,255,255,0.04)' }, ticks: { color: '#86868b', font: { size: 11 }, maxTicksLimit: 6 } }
  },
  elements: { line: { borderWidth: 2, tension: 0.4 }, point: { radius: 0 } }
});

function mk(id, col, bg) {
  if (!haveChart) return null;
  return new Chart(document.getElementById(id), {
    type:'line',
    data:{labels:[],datasets:[{data:[],borderColor:col,backgroundColor:bg,fill:true}]},
    options: baseOpts()});
}
const chT = mk('chart-temp','#ff7b72','rgba(255,123,114,0.15)');
const chP = mk('chart-ph',  '#79c0ff','rgba(121,192,255,0.15)');
const chE = mk('chart-ec',  '#7ee787','rgba(126,231,135,0.15)');
const chN = mk('chart-ntu', '#d2a8ff','rgba(210,168,255,0.15)');

function autoRange(chart) {
  if (!chart) return;
  const d = chart.data.datasets[0].data.filter(v => typeof v === 'number' && !isNaN(v));
  if (!d.length) return;
  let lo = Math.min(...d), hi = Math.max(...d);
  if (lo === hi) { lo -= 1; hi += 1; }
  const pad = (hi - lo) * 0.15 || 0.5;
  chart.options.scales.y.min = lo - pad;
  chart.options.scales.y.max = hi + pad;
}

function upd(chart, labels, data) {
  if (!chart) return;
  chart.data.labels = labels;
  chart.data.datasets[0].data = data;
  autoRange(chart);
  chart.update('none');
}

function fmt(x, digits){
  if (x === null || x === undefined || isNaN(x)) return '--';
  return Number(x).toFixed(digits);
}

function fmtClock(ms) {
  const totalSec = Math.floor(ms / 1000);
  const h = Math.floor(totalSec / 3600) % 24;
  const m = Math.floor(totalSec / 60) % 60;
  const s = totalSec % 60;
  const pad = n => n < 10 ? '0' + n : '' + n;
  return pad(h) + ':' + pad(m) + ':' + pad(s);
}

async function tick(){
  try {
    const r = await fetch('/api/latest?v=2', {cache:'no-store'});
    if (r.ok) {
      const live = await r.json();
      if (live && live.ok) {
        document.getElementById('v-power').textContent = live.power || '--';
        document.getElementById('v-temp').textContent  = fmt(live.temp_c, 1) + (isNaN(live.temp_c) ? '' : ' C');
        document.getElementById('v-water').textContent = fmt(live.water_cm, 1) + (isNaN(live.water_cm) ? '' : ' cm');
        document.getElementById('v-ph').textContent    = fmt(live.ph, 2);
        document.getElementById('v-ec').textContent    = fmt(live.ec, 2) + (isNaN(live.ec) ? '' : ' mS/cm');
        document.getElementById('v-ntu').textContent   = fmt(live.ntu, 1) + (isNaN(live.ntu) ? '' : ' NTU');
        document.getElementById('v-fish').textContent  = live.fish || '--';
        document.getElementById('v-rssi').textContent  = (live.rssi || 0) + ' dBm';
      }
    }
  } catch(e) {}

  try {
    const j = await (await fetch('/api/series', {cache:'no-store'})).json();
    if (j.rows && j.rows.length) {
      const labels = j.rows.map(r => fmtClock(r.ts_ms));
      upd(chT, labels, j.rows.map(r => r.temp_c));
      upd(chP, labels, j.rows.map(r => r.ph));
      upd(chE, labels, j.rows.map(r => r.ec));
      upd(chN, labels, j.rows.map(r => r.ntu));
    }
  } catch(e) {}

  setTimeout(tick, 2000);
}

function goManual(){
  window.location.href = '/manual';
}

tick();
</script></body></html>
)HTML";
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", PAGE);
}

/* ================================================================
 * /api/latest
 * ================================================================ */
void handleLatest() {
  static char out[400];

  if (rawCount == 0) {
    server.send(200, "application/json", "{\"ok\":false}");
    return;
  }

  uint16_t idx = (rawHead + RING_SIZE - 1) % RING_SIZE;
  LogRow& r = rawRing[idx];

  auto fnum = [](char* buf, size_t n, float v) {
    if (isnan(v)) snprintf(buf, n, "null");
    else          snprintf(buf, n, "%.2f", v);
  };
  char t[16], w[16], p[16], e[16], nt[16];
  fnum(t,  sizeof(t),  r.temp_c);
  fnum(w,  sizeof(w),  r.water_cm);
  fnum(p,  sizeof(p),  r.ph);
  fnum(e,  sizeof(e),  r.ec);
  fnum(nt, sizeof(nt), r.ntu);

  snprintf(out, sizeof(out),
    "{\"ok\":true,\"power\":\"%s\",\"temp_c\":%s,\"water_cm\":%s,"
    "\"ph\":%s,\"ec\":%s,\"ntu\":%s,\"fish\":\"%s\",\"rssi\":%d}",
    r.power, t, w, p, e, nt, r.fish, r.rssi);
  
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

/* ================================================================
 * /api/series — 1h window, 2s updates, streamed over HTTP
 * ================================================================ */
void handleSeries() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", ""); 
  
  server.sendContent("{\"rows\":[");

  char chunk[128];
  
  for (uint16_t i = 0; i < chartCount; i++) {
    uint16_t idx = (chartHead + CHART_RING_SIZE - chartCount + i) % CHART_RING_SIZE;
    ChartPoint& cp = chartRing[idx];

    auto fnum = [](char* buf, size_t n, float v) {
      if (isnan(v)) snprintf(buf, n, "null");
      else          snprintf(buf, n, "%.2f", v);
    };
    
    char t[16], p[16], e[16], nt[16];
    fnum(t,  sizeof(t),  cp.temp);
    fnum(p,  sizeof(p),  cp.ph);
    fnum(e,  sizeof(e),  cp.ec);
    fnum(nt, sizeof(nt), cp.ntu);

    snprintf(chunk, sizeof(chunk),
      "%s{\"ts_ms\":%lu,\"temp_c\":%s,\"ph\":%s,\"ec\":%s,\"ntu\":%s}",
      (i == 0 ? "" : ","), (unsigned long)cp.ts, t, p, e, nt);
      
    server.sendContent(chunk);
  }
  
  server.sendContent("]}");
  server.sendContent("");
}

void handleCsvDownload() {
  if (!LittleFS.exists(CSV_PATH)) { server.send(404, "text/plain", "No log yet"); return; }
  File f = LittleFS.open(CSV_PATH, "r");
  if (!f) { server.send(500, "text/plain", "open failed"); return; }
  server.streamFile(f, "text/csv");
  f.close();
}

void handleChartJs() {
  File f = LittleFS.open("/chart.js", "r");
  if (!f) { server.send(404, "text/plain", "chart.js missing"); return; }
  server.sendHeader("Cache-Control", "public, max-age=86400");
  server.streamFile(f, "application/javascript");
  f.close();
}

/* ================================================================
 * Uplink tick
 * ================================================================ */
bool uplinkTick() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return false;

  size_t n = 0;
  while (LoRa.available() && n < sizeof(rxBuf) - 1) rxBuf[n++] = (char)LoRa.read();
  rxBuf[n] = '\0';
  while (LoRa.available()) LoRa.read();

  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  LogRow row;
  if (parsePacketToRow(rxBuf, rssi, snr, row)) {
    pushRaw(row);
    pushChartWithDSP(row.ts_ms);
    appendCsvRow(row);

    lastUplinkMs = millis();
    if (!linkWasUp) {
      linkWasUp = true;
    }
    if (batteryAssumed) {
      batteryAssumed = false;
    }
  }
  return true;
}

#endif