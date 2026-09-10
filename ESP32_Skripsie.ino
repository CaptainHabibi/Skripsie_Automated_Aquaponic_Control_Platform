#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// =================================================================
// 1. PIN DEFINITIONS & CONSTANTS
// =================================================================
#define HEARTBEAT_LED_PIN 21 

// Hardware UART link to STM32
#define STM32_TX_PIN      43   // ESP32 TX (GPIO 43) -> STM32 PA10 (RX)
#define STM32_RX_PIN      44   // ESP32 RX (GPIO 44) -> STM32 PA9  (TX)

// Behavioral Tokens
#define TOKEN_NORMAL      0x01
#define TOKEN_HUNGRY      0x02
#define CMD_ENTER_SLEEP   0xAA
#define CMD_WAKE_MAINS    0x55

// Wi-Fi Access Point Credentials
const char *ssid     = "Aquaponic_Cam";
const char *password = "fish1234";

// Camera Pins (DFRobot FireBeetle 2 / Standard S3-CAM)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     45
#define SIOD_GPIO_NUM      1
#define SIOC_GPIO_NUM      2
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       46
#define Y7_GPIO_NUM        8
#define Y6_GPIO_NUM        7
#define Y5_GPIO_NUM        4
#define Y4_GPIO_NUM       41
#define Y3_GPIO_NUM       40
#define Y2_GPIO_NUM       39
#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM     42
#define PCLK_GPIO_NUM      5

// =================================================================
// 2. GLOBAL HANDLES & STATE
// =================================================================
HardwareSerial STM_Serial(1);
static httpd_handle_t stream_httpd = NULL;

bool is_asleep = false;
bool system_peripherals_active = false;

unsigned long last_blink_time = 0;
bool led_state = false;

unsigned long last_token_time = 0;
const unsigned long TOKEN_INTERVAL = 10000; // 10s periodic test trigger

// =================================================================
// 3. CAMERA & WEB SERVER SUBROUTINES
// =================================================================
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  char part_buf[64];

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      res = ESP_FAIL;
      break;
    }
    size_t hlen = snprintf(part_buf, 64, "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n", 2);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;
  }
  return res;
}

void start_camera_server() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t stream_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void stop_camera_server() {
  if (stream_httpd != NULL) {
    httpd_stop(stream_httpd);
    stream_httpd = NULL;
  }
}

bool init_camera() {
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;
  config.fb_count     = 2;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = CAMERA_FB_IN_PSRAM;

  return (esp_camera_init(&config) == ESP_OK);
}

// =================================================================
// 4. FULL ESP32 FUNCTIONALITY BLOCK (Called only if NOT asleep)
// =================================================================
void run_full_system() {
  // Bring up camera and Wi-Fi once upon entering active mode
  if (!system_peripherals_active) {
    Serial.println("[WAKE] Initializing Camera and Wi-Fi Subsystems...");
    if (init_camera()) {
      Serial.println("[OK] Camera Initialized.");
    } else {
      Serial.println("[ERR] Camera Init Failed.");
    }

    WiFi.softAP(ssid, password);
    Serial.print("[OK] Wi-Fi AP Online. Stream: http://");
    Serial.println(WiFi.softAPIP());

    start_camera_server();
    system_peripherals_active = true;
  }

  // --- Computer Vision & AI Inference Hook (For Tomorrow) ---
  // When model is trained:
  // 1. Grab frame: camera_fb_t *fb = esp_camera_fb_get();
  // 2. Run Edge Impulse classifier: run_classifier(...)
  // 3. Evaluate results & send token
  // 4. esp_camera_fb_return(fb);

  // Bench Test: Periodic Hungry Token upstream to STM32
  unsigned long current_millis = millis();
  if (current_millis - last_token_time >= TOKEN_INTERVAL) {
    last_token_time = current_millis;
    Serial.println("[TX] Sending TOKEN_HUNGRY (0x02) to STM32...");
    STM_Serial.write(TOKEN_HUNGRY);
  }
}

// =================================================================
// 5. LOW-POWER STANDBY HELPER
// =================================================================
void shut_down_peripherals() {
  Serial.println("[SLEEP] Shutting down high-draw peripherals...");
  stop_camera_server();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_camera_deinit();
  system_peripherals_active = false;
}

// =================================================================
// 6. SETUP & MAIN LOOP
// =================================================================
void setup() {
  Serial.begin(115200);
  STM_Serial.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

  pinMode(HEARTBEAT_LED_PIN, OUTPUT);
  digitalWrite(HEARTBEAT_LED_PIN, LOW);

  Serial.println("[INIT] ESP32-S3 Ready.");
}

void loop() {
  unsigned long current_millis = millis();

  // 1. Heartbeat LED (Slow 2-second blink when asleep, 1 Hz when awake)
  uint32_t blink_rate = is_asleep ? 1000 : 500;
  if (current_millis - last_blink_time >= blink_rate) {
    last_blink_time = current_millis;
    led_state = !led_state;
    digitalWrite(HEARTBEAT_LED_PIN, led_state);
  }

  // 2. Listen for Power State Commands from STM32 over UART
  if (STM_Serial.available()) {
    uint8_t cmd = STM_Serial.read();
    Serial.printf("[UART RX] Received byte from STM32: 0x%02X\n", cmd);

    if (cmd == CMD_ENTER_SLEEP && !is_asleep) {
      is_asleep = true;
      shut_down_peripherals();
      Serial.println("[PWR] Battery Power Active -> Standby Engaged.");
    } 
    else if (cmd == CMD_WAKE_MAINS && is_asleep) {
      is_asleep = false;
      Serial.println("[PWR] Mains Restored -> Resuming System.");
    }
  }

  // 3. Execute Full Application Only When Awake
  if (!is_asleep) {
    run_full_system();
  }
}