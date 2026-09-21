#ifndef UPLINK_H
#define UPLINK_H

#include "common.h"

struct LogRow {
  uint32_t ts_ms;
  char  power[8];
  float temp_c;
  float water_cm;
  float ph;
  float ec;
  float ntu;
  char  fish[12];
  int   rssi;
  float snr;
};

LogRow ringBuf[RING_SIZE];
uint16_t ringHead = 0;
uint16_t ringCount = 0;
String rxBuf = "";

// ================================================================
// CSV / ring helpers
// ================================================================
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
  f.printf("%lu,%s,%.2f,%.2f,%.2f,%.2f,%.1f,%s,%d,%.2f\n",
           (unsigned long)r.ts_ms, r.power, r.temp_c, r.water_cm,
           r.ph, r.ec, r.ntu, r.fish, r.rssi, r.snr);
  f.close();
}

void pushRing(const LogRow& r) {
  ringBuf[ringHead] = r;
  ringHead = (ringHead + 1) % RING_SIZE;
  if (ringCount < RING_SIZE) ringCount++;
}

// ================================================================
// Parser
// ================================================================
bool extractAfter(const String& src, const char* label, String& out) {
  int idx = src.indexOf(label);
  if (idx < 0) return false;
  int colon = src.indexOf(':', idx);
  if (colon < 0) return false;
  int eol = src.indexOf('\n', colon);
  if (eol < 0) eol = src.length();
  out = src.substring(colon + 1, eol);
  out.trim();
  return true;
}

bool parsePacketToRow(const String& pkt, int rssi, float snr, LogRow& out) {
  String v;
  memset(&out, 0, sizeof(out));
  out.ts_ms = millis();
  out.rssi  = rssi;
  out.snr   = snr;
  if (!extractAfter(pkt, "Power", v)) return false;
  strncpy(out.power, v.c_str(), sizeof(out.power) - 1);
  if (!extractAfter(pkt, "Temp",  v)) return false; out.temp_c   = v.toFloat();
  if (!extractAfter(pkt, "Water", v)) return false; out.water_cm = v.toFloat();
  if (!extractAfter(pkt, "pH",    v)) return false; out.ph       = v.toFloat();
  if (!extractAfter(pkt, "EC",    v)) return false; out.ec       = v.toFloat();
  if (!extractAfter(pkt, "NTU",   v)) return false; out.ntu      = v.toFloat();
  if (!extractAfter(pkt, "Fish",  v)) return false;
  strncpy(out.fish, v.c_str(), sizeof(out.fish) - 1);
  return true;
}

// ================================================================
// Uplink dashboard HTML
// ================================================================
void handleUplinkRoot() {
  static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Aquaponic Hub</title>
<script src="/chart.js"></script>
<style>
 body{font-family:sans-serif;margin:16px;background:#0e1621;color:#e6edf3}
 h1{font-size:18px;margin:0 0 12px}
 .row{display:flex;gap:12px;flex-wrap:wrap}
 .card{background:#16202b;padding:12px 16px;border-radius:8px;flex:1;min-width:160px}
 .card .k{font-size:12px;color:#8b98a5}
 .card .v{font-size:22px;font-weight:600;margin-top:4px}
 button{background:#1f6feb;color:#fff;border:0;padding:12px 20px;border-radius:6px;cursor:pointer;font-size:15px}
 canvas{background:#16202b;border-radius:8px;padding:6px;margin-top:8px;width:100%;display:block}
 .chart-label{font-size:12px;margin-top:10px;margin-bottom:0}
 a{color:#58a6ff}
</style></head><body>
<h1>Aquaponic Bio-Grid</h1>
<div style="margin-bottom:12px">
  <button onclick="goManual()">Manual Control</button>
  <a href="/api/log.csv" download style="margin-left:12px">Download CSV</a>
</div>
<div class="row">
  <div class="card"><div class="k">Power</div><div class="v" id="v-power">--</div></div>
  <div class="card"><div class="k">Temp</div><div class="v" id="v-temp">--</div></div>
  <div class="card"><div class="k">Water</div><div class="v" id="v-water">--</div></div>
  <div class="card"><div class="k">pH</div><div class="v" id="v-ph">--</div></div>
  <div class="card"><div class="k">EC</div><div class="v" id="v-ec">--</div></div>
  <div class="card"><div class="k">NTU</div><div class="v" id="v-ntu">--</div></div>
  <div class="card"><div class="k">Fish</div><div class="v" id="v-fish">--</div></div>
  <div class="card"><div class="k">RSSI</div><div class="v" id="v-rssi">--</div></div>
</div>

<div class="chart-label" style="color:#ff7b72">Temperature (°C)  —  range 18 to 28</div>
<canvas id="chart-temp" height="70"></canvas>
<div class="chart-label" style="color:#79c0ff">pH  —  range 5 to 9</div>
<canvas id="chart-ph" height="70"></canvas>
<div class="chart-label" style="color:#7ee787">EC (mS/cm)  —  range 0 to 3</div>
<canvas id="chart-ec" height="70"></canvas>
<div class="chart-label" style="color:#d2a8ff">Turbidity (NTU)  —  range 0 to 100</div>
<canvas id="chart-ntu" height="70"></canvas>

<script>
const baseOpts = (min, max) => ({
  animation: false, responsive: true,
  plugins: { legend: { display: false } },
  scales: {
    y: { min, max, grid: { color: '#1f2a37' }, ticks: { color: '#8b98a5' } },
    x: { grid: { color: '#1f2a37' }, ticks: { color: '#8b98a5', maxTicksLimit: 6 } }
  },
  elements: { line: { borderWidth: 2, tension: 0.25 }, point: { radius: 0 } }
});
const chT = new Chart(document.getElementById('chart-temp'), {
  type:'line', data:{labels:[],datasets:[{label:'T',data:[],borderColor:'#ff7b72',backgroundColor:'rgba(255,123,114,0.15)',fill:true}]},
  options: baseOpts(18, 28)});
const chP = new Chart(document.getElementById('chart-ph'), {
  type:'line', data:{labels:[],datasets:[{label:'pH',data:[],borderColor:'#79c0ff',backgroundColor:'rgba(121,192,255,0.15)',fill:true}]},
  options: baseOpts(5, 9)});
const chE = new Chart(document.getElementById('chart-ec'), {
  type:'line', data:{labels:[],datasets:[{label:'EC',data:[],borderColor:'#7ee787',backgroundColor:'rgba(126,231,135,0.15)',fill:true}]},
  options: baseOpts(0, 3)});
const chN = new Chart(document.getElementById('chart-ntu'), {
  type:'line', data:{labels:[],datasets:[{label:'NTU',data:[],borderColor:'#d2a8ff',backgroundColor:'rgba(210,168,255,0.15)',fill:true}]},
  options: baseOpts(0, 100)});
async function tick(){
  try{
    const j = await (await fetch('/api/series')).json();
    const L = j.rows[j.rows.length-1];
    if(L){
      document.getElementById('v-power').textContent = L.power;
      document.getElementById('v-temp').textContent  = L.temp_c + ' C';
      document.getElementById('v-water').textContent = L.water_cm + ' cm';
      document.getElementById('v-ph').textContent    = L.ph;
      document.getElementById('v-ec').textContent    = L.ec + ' mS/cm';
      document.getElementById('v-ntu').textContent   = L.ntu + ' NTU';
      document.getElementById('v-fish').textContent  = L.fish;
      document.getElementById('v-rssi').textContent  = L.rssi + ' dBm';
    }
    const labels = j.rows.map(r => (r.ts_ms/1000).toFixed(0)+'s');
    chT.data.labels=labels; chT.data.datasets[0].data=j.rows.map(r=>r.temp_c); chT.update('none');
    chP.data.labels=labels; chP.data.datasets[0].data=j.rows.map(r=>r.ph);     chP.update('none');
    chE.data.labels=labels; chE.data.datasets[0].data=j.rows.map(r=>r.ec);     chE.update('none');
    chN.data.labels=labels; chN.data.datasets[0].data=j.rows.map(r=>r.ntu);    chN.update('none');
  }catch(e){console.error(e);}
}
async function goManual(){
  try{ await fetch('/api/enter_manual'); window.location.href = '/manual'; }catch(e){}
}
setInterval(tick, 3000); tick();
</script></body></html>
)HTML";
  server.send_P(200, "text/html", PAGE);
}

// ================================================================
// Uplink API handlers
// ================================================================
void handleSeries() {
  if (ESP.getFreeHeap() < 20000) { server.send(503, "application/json", "{\"rows\":[]}"); return; }
  DynamicJsonDocument doc(12288);
  JsonArray arr = doc.createNestedArray("rows");
  uint16_t n = ringCount;
  for (uint16_t i = 0; i < n; i++) {
    uint16_t idx = (ringHead + RING_SIZE - n + i) % RING_SIZE;
    LogRow& r = ringBuf[idx];
    JsonObject o = arr.createNestedObject();
    o["ts_ms"]=r.ts_ms; o["power"]=r.power; o["temp_c"]=r.temp_c;
    o["water_cm"]=r.water_cm; o["ph"]=r.ph; o["ec"]=r.ec;
    o["ntu"]=r.ntu; o["fish"]=r.fish; o["rssi"]=r.rssi;
  }
  String out; serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleCsvDownload() {
  if (!LittleFS.exists(CSV_PATH)) { server.send(404,"text/plain","No log yet"); return; }
  File f = LittleFS.open(CSV_PATH, "r");
  server.streamFile(f, "text/csv");
  f.close();
}

void handleChartJs() {
  File f = LittleFS.open("/chart.js", "r");
  if (!f) { server.send(404,"text/plain","chart.js missing"); return; }
  server.streamFile(f, "application/javascript");
  f.close();
}

// ================================================================
// Uplink tick — call from main loop when not in manual mode
// ================================================================
void uplinkTick() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  rxBuf = "";
  while (LoRa.available()) rxBuf += (char)LoRa.read();
  int   rssi = LoRa.packetRssi();
  float snr  = LoRa.packetSnr();
  Serial.printf("[RX] %d B, RSSI %d, SNR %.2f\n", packetSize, rssi, snr);

  LogRow row;
  if (parsePacketToRow(rxBuf, rssi, snr, row)) {
    pushRing(row);
    appendCsvRow(row);
    Serial.printf("[LOG] ts=%lu T=%.1f pH=%.2f EC=%.2f\n",
                  (unsigned long)row.ts_ms, row.temp_c, row.ph, row.ec);
  }
}

#endif