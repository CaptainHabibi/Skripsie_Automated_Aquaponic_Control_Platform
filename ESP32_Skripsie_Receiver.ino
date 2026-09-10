#include <SPI.h>
#include <LoRa.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled!
#endif

// Pin Definitions for VSPI SX1278
#define LORA_SCK     18
#define LORA_MISO    19
#define LORA_MOSI    23
#define LORA_SS       5
#define LORA_RST     14
#define LORA_DIO0    -1    

#define LORA_BAND    433E6

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[INIT] Starting LoRa Receiver Gateway...");

  SerialBT.begin("Aquaponic_LoRa_Hub");
  Serial.println("[OK] Bluetooth Serial Active: 'Aquaponic_LoRa_Hub'");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("[ERR] LoRa Receiver Init Failed! Check wiring & 3.3V rail.");
    while (1) delay(1000);
  }

  // Match STM32 RF parameters exactly
  LoRa.setSignalBandwidth(125E3); // 125 kHz
  LoRa.setSpreadingFactor(7);     // SF7
  LoRa.setCodingRate4(5);         // 4/5
  LoRa.enableCrc();               // CRC Enabled
  LoRa.setSyncWord(0x12);         // Match STM32 0x12 sync word

  Serial.println("[OK] LoRa Listening on 433 MHz...");
}

void loop() {
  // Check if a radio packet has arrived from the STM32
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    String incoming_payload = "";

    // Read payload from radio buffer
    while (LoRa.available()) {
      incoming_payload += (char)LoRa.read();
    }

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    // 1. Print to Serial Monitor
    Serial.print("\n--- [LoRa Packet Received] ---\n");
    Serial.print(incoming_payload);
    Serial.printf("RF Stats: RSSI = %d dBm | SNR = %.2f dB\n", rssi, snr);

    // 2. Stream to Phone over Bluetooth
     if (SerialBT.hasClient()) {
      SerialBT.print(incoming_payload);       // already framed with ==== borders
      SerialBT.printf("Signal: %d dBm | SNR: %.2f dB\r\n", rssi, snr);
      SerialBT.println();                     // blank line before next update
    }
  }

  // Handle phone downlink commands
  if (SerialBT.available()) {
    char cmd = SerialBT.read();
    Serial.printf("[BT RX] Transmitting Downlink Command: 0x%02X\n", cmd);
    
    LoRa.beginPacket();
    LoRa.write(cmd);
    LoRa.endPacket();
  }

  delay(20);
}