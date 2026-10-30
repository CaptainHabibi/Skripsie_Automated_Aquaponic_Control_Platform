#ifndef COMMON_H
#define COMMON_H

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS     5
#define LORA_RST   14
#define LORA_DIO0  -1
#define LORA_BAND  433E6

const char* AP_SSID  = "Aquaponic_Hub";
const char* AP_PASS  = "fish1234";
const char* CSV_PATH = "/log.csv";

const size_t   CSV_MAX_BYTES = 2 * 1024 * 1024;

const uint8_t  CMD_MANUAL_FEED = 0xC1;
const uint8_t  CMD_PUMP_HIGH   = 0xC2;
const uint8_t  CMD_PUMP_MED    = 0xC3;
const uint8_t  CMD_PUMP_LOW    = 0xC4;

const uint16_t RING_SIZE = 20;
const uint8_t  MOVAVG_WINDOW = 15;

const uint32_t TX_MIN_GAP_MS     = 800;
const uint32_t MANUAL_TIMEOUT_MS = 15000;
const uint32_t LINK_LOST_MS      = 15000;

inline WebServer      server(80);
inline volatile bool  manualMode     = false;
inline uint32_t       lastTxMs       = 0;
inline uint32_t       lastManualBeat = 0;

#endif