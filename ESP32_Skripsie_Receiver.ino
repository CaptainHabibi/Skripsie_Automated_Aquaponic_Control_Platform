#include "common.h"
#include "uplink.h"
#include "downlink.h"

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[INIT] Aquaponic Receiver Hub");

  if (!LittleFS.begin(true)) Serial.println("[ERR] LittleFS mount failed");
  else { appendCsvHeaderIfNew(); Serial.println("[OK] LittleFS mounted"); }

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("[ERR] LoRa init failed");
    while (1) delay(1000);
  }
  LoRa.setTxPower(17);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(7);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  LoRa.setSyncWord(0x12);
  LoRa.receive();
  Serial.println("[OK] LoRa listening @ 433 MHz");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[OK] AP '%s' up. http://%s/\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());

  server.on("/",              handleUplinkRoot);
  server.on("/api/series",    handleSeries);
  server.on("/api/log.csv",   handleCsvDownload);
  server.on("/chart.js",      handleChartJs);

  server.on("/manual",            handleManualRoot);
  server.on("/api/mode",          handleMode);
  server.on("/api/enter_manual",  handleEnterManual);
  server.on("/api/exit_manual",   handleExitManual);
  server.on("/api/feed",          handleFeed);
  server.on("/api/pump",          handlePump);

  server.onNotFound([](){ server.send(404, "text/plain", "not found"); });
  server.begin();
  Serial.println("[OK] HTTP server started");
}

void loop() {
  server.handleClient();
  if (!manualMode) uplinkTick();
  delay(2);
}