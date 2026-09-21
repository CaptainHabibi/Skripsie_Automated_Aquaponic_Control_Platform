#ifndef COMMON_H
#define COMMON_H

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS     5
#define LORA_RST   14
#define LORA_DIO0  -1
#define LORA_BAND  433E6

const char* AP_SSID = "Aquaponic_Hub";
const char* AP_PASS = "fish1234";
const char* CSV_PATH = "/log.csv";

const uint8_t CMD_MANUAL_FEED = 0xC1;
const uint8_t CMD_PUMP_HIGH   = 0xC2;
const uint8_t CMD_PUMP_MED    = 0xC3;
const uint8_t CMD_PUMP_LOW    = 0xC4;

const uint16_t RING_SIZE = 100;

WebServer server(80);
volatile bool manualMode = false;

#endif