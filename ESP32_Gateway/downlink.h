#ifndef DOWNLINK_H
#define DOWNLINK_H

#include "common.h"

bool sendDownlinkCommand(uint8_t cmd) {
  uint32_t now = millis();
  if (now - lastTxMs < TX_MIN_GAP_MS) {
    Serial.printf("[SKIP] cooldown %lu ms\n", now - lastTxMs);
    return false;
  }
  lastTxMs = now;

  LoRa.idle();
  delay(20);
  LoRa.beginPacket();
  LoRa.write(cmd);
  LoRa.endPacket();
  delay(20);
  LoRa.receive();

  Serial.printf("[LoRa TX] 0x%02X\n", cmd);
  return true;
}

void manualTimeoutTick() {
  if (manualMode && (millis() - lastManualBeat > MANUAL_TIMEOUT_MS)) {
    manualMode = false;
    LoRa.idle();
    delay(10);
    LoRa.receive();
    Serial.println("[MODE] Manual timed out -> NORMAL");
  }
}

void handleManualRoot() {
  static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Manual Control</title>
<style>
 body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;margin:16px;background:linear-gradient(135deg,#0f2027,#203a43,#2c5364);background-attachment:fixed;color:#e6edf3}
 h1{font-size:22px;margin:0 0 16px;font-weight:600;letter-spacing:0.5px}
 .glass{background:rgba(255,255,255,0.05);backdrop-filter:blur(12px);-webkit-backdrop-filter:blur(12px);border:1px solid rgba(255,255,255,0.1);border-radius:12px;box-shadow:0 8px 32px rgba(0,0,0,0.2)}
 .mode{padding:14px 18px;margin-bottom:16px;font-size:15px}
 .section{padding:18px;margin-bottom:16px}
 .section h2{font-size:13px;margin:0 0 8px;color:#a1b0c0;text-transform:uppercase;letter-spacing:1px}
 .helper{font-size:12px;color:#ff7b72;margin:0 0 14px;font-weight:500}
 button{background:rgba(31,111,235,0.6);color:#fff;border:1px solid rgba(255,255,255,0.1);padding:14px 20px;border-radius:8px;cursor:pointer;margin-right:8px;margin-bottom:8px;font-size:15px;min-width:130px;backdrop-filter:blur(4px);transition:all 0.2s}
 button.exit{background:rgba(179,71,0,0.6);width:100%;margin-right:0}
 button:hover:not(:disabled){background:rgba(31,111,235,0.8);transform:translateY(-1px)}
 button.exit:hover:not(:disabled){background:rgba(179,71,0,0.8)}
 button:active{transform:scale(0.98)}
 button:disabled{opacity:0.4;cursor:not-allowed}
 .fb{font-size:15px;padding:12px;border-radius:8px;margin-top:16px;min-height:24px;font-weight:600;text-align:center}
 .fb.ok{background:rgba(46,160,67,0.2);color:#7ee787;border:1px solid rgba(46,160,67,0.3)}
 .fb.err{background:rgba(248,81,73,0.2);color:#ff7b72;border:1px solid rgba(248,81,73,0.3)}
 .fb.info{background:rgba(56,139,253,0.2);color:#79c0ff;border:1px solid rgba(56,139,253,0.3)}
</style></head><body>
<h1>Manual Control</h1>
<div class="mode glass">Mode: <span id="mode" style="font-weight:700">?</span></div>
<div class="section glass">
  <h2>Feeder</h2>
  <div class="helper">Wait 5s before sending the next command. LoRa needs a cooldown.</div>
  <button class="cmd" onclick="send('feed')">Feed Fish</button>
</div>
<div class="section glass">
  <h2>Pump Speed</h2>
  <div class="helper">Wait 5s before sending the next command. LoRa needs a cooldown.</div>
  <button class="cmd" onclick="send('pump?lvl=H')">HIGH</button>
  <button class="cmd" onclick="send('pump?lvl=M')">MED</button>
  <button class="cmd" onclick="send('pump?lvl=L')">LOW</button>
</div>
<div class="section glass">
  <h2>Exit</h2>
  <button class="exit" onclick="exitManual()">Back to Normal Mode</button>
</div>
<div class="fb info glass" id="fb" style="backdrop-filter:none">Ready.</div>
<script>
let exiting = false, busy = false, beatTimer = null;
const fb = document.getElementById('fb');
const modeEl = document.getElementById('mode');

function setFb(cls, msg){ fb.className = 'fb glass ' + cls; fb.textContent = msg; }
function setBtns(dis){ document.querySelectorAll('button.cmd').forEach(b => b.disabled = dis); }

async function api(path, timeoutMs){
  const c = new AbortController();
  const t = setTimeout(() => c.abort(), timeoutMs || 5000);
  try {
    return await fetch('/api/' + path, {
      signal: c.signal,
      cache: 'no-store',
      headers: { 'Connection': 'close' }
    });
  } finally { clearTimeout(t); }
}

async function beat(){
  if (exiting || busy) return;
  try {
    const r = await api('enter_manual', 4000);
    modeEl.textContent = await r.text();
    modeEl.style.color = '#7ee787';
  } catch(e) {
    modeEl.textContent = 'NO LINK';
    modeEl.style.color = '#ff7b72';
  }
}

async function send(path){
  if (exiting || busy) return;
  busy = true; setBtns(true);
  setFb('info', 'Sending...');
  try {
    const r = await api(path, 6000);
    const t = await r.text();
    if (r.status === 429) setFb('err', 'Too fast - wait a moment and tap again.');
    else                  setFb(r.ok ? 'ok' : 'err', '[' + r.status + '] ' + t);
  } catch(e) {
    setFb('err', 'Hub did not answer (' + e.name + '). Check the AP and retry.');
  }
  busy = false;
  if (!exiting) setBtns(false);
}

async function exitManual(){
  if (exiting) return;
  exiting = true;
  clearInterval(beatTimer);
  setBtns(true);
  const btn = document.querySelector('button.exit');
  if (btn) btn.disabled = true;
  setFb('info', 'Exiting...');
  try {
    const r = await api('exit_manual', 6000);
    setFb('ok', '[' + r.status + '] ' + await r.text());
  } catch(e) {
    setFb('err', 'Exit failed (' + e.name + '). Manual mode will time out.');
  }
  setTimeout(function(){ window.location.href = '/'; }, 700);
}

beat();
beatTimer = setInterval(beat, 6000);
</script></body></html>
)HTML";
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", PAGE);
}

void handleMode() {
  server.send(200, "text/plain", manualMode ? "MANUAL" : "NORMAL");
}

void handleEnterManual() {
  if (!manualMode) {
    manualMode = true;
    Serial.println("[MODE] Entered MANUAL");
  }
  lastManualBeat = millis();
  server.send(200, "text/plain", "MANUAL");
}

void handleExitManual() {
  if (manualMode) Serial.println("[MODE] Exited MANUAL");
  manualMode = false;
  LoRa.idle();
  delay(10);
  LoRa.receive();
  delay(5);
  server.send(200, "text/plain", "normal mode");
}

void handleFeed() {
  if (!manualMode) { server.send(409, "text/plain", "NOT in manual mode"); return; }
  lastManualBeat = millis();
  if (!sendDownlinkCommand(CMD_MANUAL_FEED)) {
    server.send(429, "text/plain", "cooldown - retry");
    return;
  }
  server.send(200, "text/plain", "FEED sent");
}

void handlePump() {
  if (!manualMode) { server.send(409, "text/plain", "NOT in manual mode"); return; }
  lastManualBeat = millis();

  String lvl = server.arg("lvl");
  lvl.toUpperCase();
  uint8_t cmd;
  if      (lvl == "H") cmd = CMD_PUMP_HIGH;
  else if (lvl == "M") cmd = CMD_PUMP_MED;
  else if (lvl == "L") cmd = CMD_PUMP_LOW;
  else { server.send(400, "text/plain", "lvl must be H/M/L"); return; }

  if (!sendDownlinkCommand(cmd)) {
    server.send(429, "text/plain", "cooldown - retry");
    return;
  }
  server.send(200, "text/plain", "Pump " + lvl + " sent");
}

#endif