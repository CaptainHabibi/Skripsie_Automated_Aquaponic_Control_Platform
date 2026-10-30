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

/* One smoothed sample for the charts */
struct ChartPoint {
  uint32_t ts;
  float temp;
  float ph;
  float ec;
  float ntu;
};

/* Raw ring: latest only, feeds /api/latest */
LogRow rawRing[RING_SIZE];
uint16_t rawHead = 0;
uint16_t rawCount = 0;

/* ---- Two-tier chart storage -------------------------------------------
 *  recentRing : one smoothed point per packet, last ~5 min  (live detail)
 *  histRing   : 1 min averages of the above, last 8 h        (long view)
 * ---------------------------------------------------------------------- */
const uint16_t RECENT_SIZE    = 150;
const uint16_t HIST_SIZE      = 480;
const uint32_t HIST_BUCKET_MS = 60000; // 1 minute buckets for clean 2-hour smoothness

ChartPoint recentRing[RECENT_SIZE];
uint16_t recentHead = 0, recentCount = 0;
ChartPoint histRing[HIST_SIZE];
uint16_t histHead = 0, histCount = 0;

/* Link supervision & metadata trackers for CSV sync */
static uint32_t lastUplinkMs = 0;
static bool linkWasUp = false;
static bool batteryAssumed = false;
static char rxBuf[256];

static char   latestPower[8]  = "MAINS";
static float  latestWater     = 0.0f;
static char   latestFish[12]  = "Normal";
static int    latestRssi      = 0;
static float  latestSnr       = 0.0f;

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
  f.printf("%lu,%s,%.1f,%.1f,%.2f,%.3f,%.1f,%s,%d,%.2f\n",
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

/* 1 min history bucket accumulator */
static float    bT = 0, bP = 0, bE = 0, bN = 0;
static uint16_t cT = 0, cP = 0, cE = 0, cN = 0;
static uint32_t bucketStart = 0;
static bool     bucketOpen  = false;

static void accumulateHist(const ChartPoint& cp) {
  if (!bucketOpen) { bucketStart = cp.ts; bucketOpen = true; }
  if (!isnan(cp.temp)) { bT += cp.temp; cT++; }
  if (!isnan(cp.ph))   { bP += cp.ph;   cP++; }
  if (!isnan(cp.ec))   { bE += cp.ec;   cE++; }
  if (!isnan(cp.ntu))  { bN += cp.ntu;  cN++; }

  if (cp.ts - bucketStart >= HIST_BUCKET_MS) {
    ChartPoint h;
    h.ts   = cp.ts;
    h.temp = cT ? bT / cT : NAN;
    h.ph   = cP ? bP / cP : NAN;
    h.ec   = cE ? bE / cE : NAN;
    h.ntu  = cN ? bN / cN : NAN;
    histRing[histHead] = h;
    histHead = (histHead + 1) % HIST_SIZE;
    if (histCount < HIST_SIZE) histCount++;

    // CSV logs the exact same smoothed historical point shown on the graph
    LogRow csvRow;
    csvRow.ts_ms = h.ts;
    strncpy(csvRow.power, latestPower, sizeof(csvRow.power) - 1);
    csvRow.temp_c   = h.temp;
    csvRow.water_cm = latestWater;
    csvRow.ph       = h.ph;
    csvRow.ec       = h.ec;
    csvRow.ntu      = h.ntu;
    strncpy(csvRow.fish, latestFish, sizeof(csvRow.fish) - 1);
    csvRow.rssi     = latestRssi;
    csvRow.snr      = latestSnr;
    appendCsvRow(csvRow);

    bT = bP = bE = bN = 0;
    cT = cP = cE = cN = 0;
    bucketOpen = false;
  }
}

/* ================================================================
 * DOUBLE SMOOTHING ALGORITHM
 * ================================================================ */
void smooth_row(LogRow& row) {
  // Stage 1: 10-point moving average
  static float buf_10_t[10] = {0}, buf_10_p[10] = {0}, buf_10_e[10] = {0}, buf_10_n[10] = {0};
  static uint8_t head_10 = 0, count_10 = 0;
  
  // Stage 2: 3-point moving average of Stage 1 outputs
  static float buf_3_t[3] = {0}, buf_3_p[3] = {0}, buf_3_e[3] = {0}, buf_3_n[3] = {0};
  static uint8_t head_3 = 0, count_3 = 0;

  buf_10_t[head_10] = row.temp_c;
  buf_10_p[head_10] = row.ph;
  buf_10_e[head_10] = row.ec;
  buf_10_n[head_10] = row.ntu;
  
  head_10 = (head_10 + 1) % 10;
  if (count_10 < 10) count_10++;

  float sum_10_t = 0, sum_10_p = 0, sum_10_e = 0, sum_10_n = 0;
  uint8_t c_10_t = 0, c_10_p = 0, c_10_e = 0, c_10_n = 0;
  for (uint8_t i = 0; i < count_10; i++) {
    if (!isnan(buf_10_t[i])) { sum_10_t += buf_10_t[i]; c_10_t++; }
    if (!isnan(buf_10_p[i])) { sum_10_p += buf_10_p[i]; c_10_p++; }
    if (!isnan(buf_10_e[i])) { sum_10_e += buf_10_e[i]; c_10_e++; }
    if (!isnan(buf_10_n[i])) { sum_10_n += buf_10_n[i]; c_10_n++; }
  }
  
  float avg_10_t = c_10_t ? sum_10_t / c_10_t : NAN;
  float avg_10_p = c_10_p ? sum_10_p / c_10_p : NAN;
  float avg_10_e = c_10_e ? sum_10_e / c_10_e : NAN;
  float avg_10_n = c_10_n ? sum_10_n / c_10_n : NAN;

  buf_3_t[head_3] = avg_10_t;
  buf_3_p[head_3] = avg_10_p;
  buf_3_e[head_3] = avg_10_e;
  buf_3_n[head_3] = avg_10_n;
  
  head_3 = (head_3 + 1) % 3;
  if (count_3 < 3) count_3++;

  float sum_3_t = 0, sum_3_p = 0, sum_3_e = 0, sum_3_n = 0;
  uint8_t c_3_t = 0, c_3_p = 0, c_3_e = 0, c_3_n = 0;
  for (uint8_t i = 0; i < count_3; i++) {
    if (!isnan(buf_3_t[i])) { sum_3_t += buf_3_t[i]; c_3_t++; }
    if (!isnan(buf_3_p[i])) { sum_3_p += buf_3_p[i]; c_3_p++; }
    if (!isnan(buf_3_e[i])) { sum_3_e += buf_3_e[i]; c_3_e++; }
    if (!isnan(buf_3_n[i])) { sum_3_n += buf_3_n[i]; c_3_n++; }
  }

  row.temp_c = c_3_t ? sum_3_t / c_3_t : NAN;
  row.ph     = c_3_p ? sum_3_p / c_3_p : NAN;
  row.ec     = c_3_e ? sum_3_e / c_3_e : NAN;
  row.ntu    = c_3_n ? sum_3_n / c_3_n : NAN;
}

void pushChartPoint(const LogRow& row) {
  ChartPoint cp;
  cp.ts = row.ts_ms;
  cp.temp = row.temp_c;
  cp.ph = row.ph;
  cp.ec = row.ec;
  cp.ntu = row.ntu;

  recentRing[recentHead] = cp;
  recentHead = (recentHead + 1) % RECENT_SIZE;
  if (recentCount < RECENT_SIZE) recentCount++;

  accumulateHist(cp);
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
.card .v{font-size:28px;font-weight:700;letter-spacing:-0.5px;color:#fff;white-space:nowrap}
.card .u{font-size:14px;font-weight:500;color:#86868b;letter-spacing:0}
.range{font-size:13px;color:#86868b;margin:0 0 12px}
.charts{display:grid;grid-template-columns:1fr 1fr;gap:16px}
.chart-box{padding:18px;height:280px;display:flex;flex-direction:column}
.chart-title{font-size:14px;margin:0 0 12px;font-weight:600;flex:0 0 auto;letter-spacing:0.3px}
.canvas-wrap{position:relative;flex:1 1 auto;min-height:0}
.canvas-wrap canvas{position:absolute;top:0;left:0;width:100%;height:100%}
@media (max-width:600px){.charts{grid-template-columns:1fr}.chart-box{height:240px}}
</style></head><body>
<h1>Aquaponic Bio-Grid</h1>
<div class="nav">
  <button onclick="goManual()">Manual Control</button>
  <a href="/api/log.csv" download>Download CSV</a>
</div>
<div class="row">
  <div class="card glass"><div class="k">Power</div><div class="v" id="v-power">--</div></div>
  <div class="card glass"><div class="k">Temp</div><div class="v"><span id="v-temp">--</span><span class="u"> C</span></div></div>
  <div class="card glass"><div class="k">Water</div><div class="v"><span id="v-water">--</span><span class="u"> cm</span></div></div>
  <div class="card glass"><div class="k">pH</div><div class="v" id="v-ph">--</div></div>
  <div class="card glass"><div class="k">EC</div><div class="v"><span id="v-ec">--</span><span class="u"> mS/cm</span></div></div>
  <div class="card glass"><div class="k">NTU</div><div class="v"><span id="v-ntu">--</span><span class="u"> NTU</span></div></div>
  <div class="card glass"><div class="k">Fish</div><div class="v" id="v-fish">--</div></div>
  <div class="card glass"><div class="k">RSSI</div><div class="v"><span id="v-rssi">--</span><span class="u"> dBm</span></div></div>
</div>
<div class="range" id="range">Waiting for data...</div>

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
const WINDOW_MS = 2 * 60 * 60 * 1000;
const GRID = 'rgba(255,255,255,0.05)', AX = '#86868b';

const CFG = {
  temp:{id:'chart-temp', key:'t', col:'#ff7b72', bg:'rgba(255,123,114,0.15)', dec:1, minSpan:1.0, unit:' C'},
  ph:  {id:'chart-ph',   key:'p', col:'#79c0ff', bg:'rgba(121,192,255,0.15)', dec:1, minSpan:0.5, unit:''},
  ec:  {id:'chart-ec',   key:'e', col:'#7ee787', bg:'rgba(126,231,135,0.15)', dec:1, minSpan:0.2, unit:' mS/cm'},
  ntu: {id:'chart-ntu',  key:'n', col:'#d2a8ff', bg:'rgba(210,168,255,0.15)', dec:1, minSpan:20,  unit:' NTU'}
};

const pad = n => n < 10 ? '0' + n : '' + n;
function fmtTime(ms, withSec) {
  const d = new Date(ms);
  return pad(d.getHours()) + ':' + pad(d.getMinutes()) + (withSec ? ':' + pad(d.getSeconds()) : '');
}

const STEPS = [10, 30, 60, 120, 300, 600, 900, 1800, 3600].map(s => s * 1000);
let tickStep = 600000;
function pickStep(span) {
  for (const s of STEPS) if (span / s <= 6) return s;
  return STEPS[STEPS.length - 1];
}

function niceStep(x) {
  const p = Math.pow(10, Math.floor(Math.log10(x)));
  const f = x / p;
  return (f <= 1 ? 1 : f <= 2 ? 2 : f <= 5 ? 5 : 10) * p;
}

function mk(cfg) {
  if (!haveChart) return null;
  return new Chart(document.getElementById(cfg.id), {
    type: 'line',
    data: { datasets: [{
      data: [], borderColor: cfg.col, backgroundColor: cfg.bg, fill: true,
      borderWidth: 2, pointRadius: 0, pointHoverRadius: 4,
      cubicInterpolationMode: 'monotone', spanGaps: true
    }]},
    options: {
      animation: false, responsive: true, maintainAspectRatio: false,
      parsing: false, normalized: true,
      interaction: { mode: 'nearest', axis: 'x', intersect: false },
      plugins: {
        legend: { display: false },
        tooltip: { displayColors: false, callbacks: {
          title: it => fmtTime(it[0].parsed.x, true),
          label: c => c.parsed.y.toFixed(cfg.dec) + cfg.unit
        }}
      },
      scales: {
        x: {
          type: 'linear', border: { display: false }, grid: { color: GRID },
          ticks: { color: AX, font: { size: 11 }, maxRotation: 0, autoSkip: false,
                   includeBounds: false, callback: v => fmtTime(v, tickStep < 60000) },
          afterBuildTicks: sc => {
            const off = -new Date().getTimezoneOffset() * 60000;
            const first = Math.ceil((sc.min - off) / tickStep) * tickStep + off;
            const t = [];
            for (let v = first; v <= sc.max; v += tickStep) t.push({ value: v });
            sc.ticks = t;
          }
        },
        y: {
          border: { display: false }, grid: { color: GRID },
          ticks: { color: AX, font: { size: 11 }, includeBounds: false,
                   callback: v => Number(v).toFixed(cfg.dec) }
        }
      }
    }
  });
}

const charts = {};
for (const k in CFG) charts[k] = mk(CFG[k]);

function setY(chart, cfg, pts) {
  let lo = Infinity, hi = -Infinity;
  for (const p of pts) if (p.y !== null) { if (p.y < lo) lo = p.y; if (p.y > hi) hi = p.y; }
  if (lo > hi) return;
  const mid  = (lo + hi) / 2;
  const span = Math.max((hi - lo) * 1.2, cfg.minSpan);
  const unit = Math.pow(10, -cfg.dec);
  const step = Math.max(unit, niceStep(span / 4));
  const y = chart.options.scales.y;
  y.min = Math.floor((mid - span / 2) / step + 1e-9) * step;
  y.max = Math.ceil ((mid + span / 2) / step - 1e-9) * step;
  y.ticks.stepSize = step;
}

function applySeries(j) {
  if (!j || !j.a || !j.a.length) return;
  const now = Date.now(), n = j.a.length;
  const xs = j.a.map(age => now - age);
  const xMax = xs[n - 1];
  const xMin = Math.max(xs[0], xMax - WINDOW_MS);
  const span = Math.max(xMax - xMin, 60000);
  tickStep = pickStep(span);

  for (const k in CFG) {
    const ch = charts[k]; if (!ch) continue;
    const arr = j[CFG[k].key], pts = [];
    for (let i = 0; i < n; i++)
      if (xs[i] >= xMin) pts.push({ x: xs[i], y: (arr[i] === null ? null : arr[i]) });
    ch.data.datasets[0].data = pts;
    ch.options.scales.x.min = xMax - span;
    ch.options.scales.x.max = xMax;
    setY(ch, CFG[k], pts);
    ch.update('none');
  }
  const mins = Math.round(span / 60000);
  document.getElementById('range').textContent =
    'Showing last ' + (mins >= 120 ? '2 hours' : mins >= 2 ? mins + ' minutes' : 'minute') + ' - updates live';
}

const isNum = x => x !== null && x !== undefined && !isNaN(x);
const fmt = (x, d) => isNum(x) ? Number(x).toFixed(d) : '--';
const setText = (id, t) => { document.getElementById(id).textContent = t; };

async function getJSON(url) {
  const r = await fetch(url, { cache: 'no-store' });
  if (!r.ok) throw new Error(r.status);
  return r.json();
}

let n = 0;
async function tick() {
  if (!document.hidden) {
    try {
      const live = await getJSON('/api/latest?v=3');
      if (live && live.ok) {
        setText('v-power', live.power || '--');
        setText('v-temp',  fmt(live.temp_c, CFG.temp.dec));
        setText('v-water', fmt(live.water_cm, 1));
        setText('v-ph',    fmt(live.ph, CFG.ph.dec));
        setText('v-ec',    fmt(live.ec, CFG.ec.dec));
        setText('v-ntu',   fmt(live.ntu, CFG.ntu.dec));
        setText('v-fish',  live.fish || '--');
        setText('v-rssi',  isNum(live.rssi) ? live.rssi : '--');
      }
    } catch (e) {}

    if (n % 2 === 0) {
      try { applySeries(await getJSON('/api/series')); } catch (e) {}
    }
    n++;
  }
  setTimeout(tick, 2000);
}

function goManual() { window.location.href = '/manual'; }
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
  char ts_[16], w[16], ps[16], es[16], ns[16];
  fnum(ts_, sizeof(ts_), r.temp_c);
  fnum(w,   sizeof(w),   r.water_cm);
  fnum(ps,  sizeof(ps),  r.ph);
  fnum(es,  sizeof(es),  r.ec);
  fnum(ns,  sizeof(ns),  r.ntu);

  snprintf(out, sizeof(out),
    "{\"ok\":true,\"power\":\"%s\",\"temp_c\":%s,\"water_cm\":%s,"
    "\"ph\":%s,\"ec\":%s,\"ntu\":%s,\"fish\":\"%s\",\"rssi\":%d}",
    r.power, ts_, w, ps, es, ns, r.fish, r.rssi);

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

/* ================================================================
 * /api/series
 * ================================================================ */
void handleSeries() {
  const uint32_t nowMs = millis();

  uint32_t oldestRecent = 0xFFFFFFFFUL;
  if (recentCount) oldestRecent = recentRing[(recentHead + RECENT_SIZE - recentCount) % RECENT_SIZE].ts;

  uint16_t histSend = 0;
  while (histSend < histCount) {
    const ChartPoint& h = histRing[(histHead + HIST_SIZE - histCount + histSend) % HIST_SIZE];
    if (h.ts >= oldestRecent) break;
    histSend++;
  }
  const uint16_t total = histSend + recentCount;

  auto pointAt = [&](uint16_t i) -> const ChartPoint& {
    if (i < histSend) return histRing[(histHead + HIST_SIZE - histCount + i) % HIST_SIZE];
    return recentRing[(recentHead + RECENT_SIZE - recentCount + (i - histSend)) % RECENT_SIZE];
  };

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", "no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.send(200, "application/json", "");

  static char buf[1024];
  size_t n = 0;
  auto flush = [&]() { if (n) { buf[n] = '\0'; server.sendContent(buf); n = 0; } };
  auto put = [&](const char* s) {
    size_t l = strlen(s);
    if (n + l >= sizeof(buf)) flush();
    memcpy(buf + n, s, l);
    n += l;
  };

  auto emit = [&](const char* name, uint8_t field, bool first) {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%s\"%s\":[", first ? "" : ",", name);
    put(tmp);
    for (uint16_t i = 0; i < total; i++) {
      const ChartPoint& cp = pointAt(i);
      float v = 0; int dec = 2;
      switch (field) {
        case 0: snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)(nowMs - cp.ts)); v = 1; dec = -1; break;
        case 1: v = cp.temp; dec = 2; break;
        case 2: v = cp.ph;   dec = 2; break;
        case 3: v = cp.ec;   dec = 3; break;
        case 4: v = cp.ntu;  dec = 1; break;
      }
      if (dec >= 0) {
        if (isnan(v)) snprintf(tmp, sizeof(tmp), "null");
        else          snprintf(tmp, sizeof(tmp), "%.*f", dec, v);
      }
      if (i) put(",");
      put(tmp);
    }
    put("]");
  };

  put("{");
  emit("a", 0, true);
  emit("t", 1, false);
  emit("p", 2, false);
  emit("e", 3, false);
  emit("n", 4, false);
  put("}");
  flush();
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
    // Keep metadata updated for CSV rows
    strncpy(latestPower, row.power, sizeof(latestPower) - 1);
    latestWater = row.water_cm;
    strncpy(latestFish, row.fish, sizeof(latestFish) - 1);
    latestRssi = row.rssi;
    latestSnr = row.snr;

    // Apply short-term double smoothing algorithm
    smooth_row(row);
    
    // Store smoothed data in rings & accumulate historical smoothed buckets
    pushRaw(row);
    pushChartPoint(row);

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