#include <CaptainHabibi-Skripsie_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// ======================================================
// 1. PIN DEFINITIONS (DFRobot FireBeetle 2 ESP32-S3-CAM)
// ======================================================
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     45
#define SIOD_GPIO_NUM     1
#define SIOC_GPIO_NUM     2
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       46
#define Y7_GPIO_NUM       8
#define Y6_GPIO_NUM       7
#define Y5_GPIO_NUM       4
#define Y4_GPIO_NUM       41
#define Y3_GPIO_NUM       40
#define Y2_GPIO_NUM       39
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     42
#define PCLK_GPIO_NUM     5
#define HEARTBEAT_LED_PIN 21

// ======================================================
// 2. UART & TOKEN DEFINITIONS
// ======================================================
#define STM32_TX_PIN       43
#define STM32_RX_PIN       44

#define TOKEN_NORMAL       0x01
#define TOKEN_HUNGRY       0x02
#define TOKEN_NIGHT        0x03
#define CMD_ENTER_SLEEP    0xAA
#define CMD_WAKE_MAINS     0x55

HardwareSerial STM_Serial(1);

// ======================================================
// 3. STATE
// ======================================================
bool is_asleep = false;
unsigned long last_blink_time = 0;
bool led_state = false;

volatile bool g_stream_paused = false;

// WiFi streaming
const char *ssid = "Aquaponic_Cam";
const char *password = "fish1234";
static httpd_handle_t stream_httpd = NULL;

// ======================================================
// 4. EDGE IMPULSE CAMERA SETUP
// ======================================================
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

static bool debug_nn = false;
static bool is_initialised = false;
uint8_t *snapshot_buf;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 16000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 3,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,
};

bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

// ======================================================
// 5. WIFI STREAMING
// ======================================================
static const char STREAM_PAGE[] PROGMEM =
    "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Aquaponic Cam</title>"
    "<style>body{font-family:system-ui;background:#111;color:#eee;margin:0;padding:16px}"
    "h2{margin:0 0 8px 0}"
    "img{max-width:100%;border-radius:8px;display:block;transform:rotate(180deg)}"
    "</style></head><body>"
    "<h2>Aquaponic Cam</h2>"
    "<img src='/stream'>"
    "</body></html>";

static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Connection", "close");
    return httpd_resp_send(req, STREAM_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req) {
    esp_err_t res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    if (res != ESP_OK) return res;
    char part_buf[96];
    while (true) {
        if (g_stream_paused) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        size_t hlen = snprintf(part_buf, sizeof(part_buf),
            "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
            (unsigned)fb->len);
        res = httpd_resp_send_chunk(req, part_buf, hlen);
        if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        if (res == ESP_OK) res = httpd_resp_send_chunk(req, "\r\n", 2);
        esp_camera_fb_return(fb);
        if (res != ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    return res;
}

void start_camera_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 5;
    config.send_wait_timeout = 5;

    httpd_uri_t root_uri = { "/", HTTP_GET, root_handler, NULL };
    httpd_uri_t stream_uri = { "/stream", HTTP_GET, stream_handler, NULL };

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &root_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("[OK] HTTP server started");
    } else {
        Serial.println("[ERR] HTTP server start failed");
        stream_httpd = NULL;
    }
}

void stop_camera_server() {
    if (stream_httpd != NULL) {
        httpd_stop(stream_httpd);
        stream_httpd = NULL;
    }
}

// ======================================================
// 6. SLEEP / WAKE HANDLERS
// ======================================================
void enter_sleep() {
    Serial.println("[SLEEP] ESP32 entering standby");
    stop_camera_server();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    if (is_initialised) {
        ei_camera_deinit();
        is_initialised = false;
    }
    setCpuFrequencyMhz(80);
    is_asleep = true;
}

void wake_up() {
    Serial.println("[WAKE] ESP32 resuming");
    setCpuFrequencyMhz(240);
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    if (!WiFi.softAP(ssid, password)) {
        Serial.println("[ERR] WiFi AP failed");
    } else {
        Serial.print("[OK] WiFi AP: ");
        Serial.println(WiFi.softAPIP());
    }
    if (!is_initialised) {
        if (ei_camera_init()) {
            Serial.println("[OK] Camera ready");
            start_camera_server();
        } else {
            Serial.println("[ERR] Camera init failed");
        }
    }
    is_asleep = false;
}

// ======================================================
// 7. SETUP
// ======================================================
void setup() {
    Serial.begin(115200);
    STM_Serial.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
    pinMode(HEARTBEAT_LED_PIN, OUTPUT);
    digitalWrite(HEARTBEAT_LED_PIN, LOW);
    Serial.println("[INIT] ESP32-S3 Booting...");

    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    if (snapshot_buf == nullptr) {
        Serial.println("[ERR] Failed to allocate snapshot buffer");
    }
    wake_up(); // Bring up camera, WiFi, and HTTP server
    Serial.println("[READY] Inference loop started");
}

// ======================================================
// 8. MAIN LOOP
// ======================================================
void loop() {
    unsigned long now = millis();
    uint32_t rate = is_asleep ? 1000 : 500;
    if (now - last_blink_time >= rate) {
        last_blink_time = now;
        led_state = !led_state;
        digitalWrite(HEARTBEAT_LED_PIN, led_state);
    }

    // Handle UART commands from STM32
    if (STM_Serial.available()) {
        uint8_t cmd = STM_Serial.read();
        if (cmd == CMD_ENTER_SLEEP && !is_asleep) {
            Serial.printf("[UART RX] 0x%02X (sleep)\n", cmd);
            enter_sleep();
        } else if (cmd == CMD_WAKE_MAINS && is_asleep) {
            Serial.printf("[UART RX] 0x%02X (wake)\n", cmd);
            wake_up();
        }
        /* else: periodic heartbeat - ignore, do not spam the log */
    }

    // If asleep, do nothing except blink
    if (is_asleep) {
        delay(50);
        return;
    }

    // Run inference
    if (ei_sleep(5) != EI_IMPULSE_OK) return;

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        Serial.println("[ERR] Failed to capture image");
        ei_sleep(2000);
        return;
    }

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("[ERR] Failed to run classifier (%d)\n", err);
        ei_sleep(2000);
        return;
    }

    // Find highest confidence label
    String top_label = "";
    float top_confidence = 0.0;
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        float val = result.classification[i].value;
        if (val > top_confidence) {
            top_confidence = val;
            top_label = String(ei_classifier_inferencing_categories[i]);
        }
    }

    // Map this frame's label to a token
    uint8_t frame_token;
    if (top_label == "Feeding_Behaviour") frame_token = TOKEN_HUNGRY;
    else if (top_label == "Normal_Behaviour") frame_token = TOKEN_NORMAL;
    else if (top_label == "Night") frame_token = TOKEN_NIGHT;
    else frame_token = TOKEN_NORMAL; // safe default

    // Send the single-frame token directly to the STM32
    STM_Serial.write(frame_token);

    const char *fr_str = (frame_token == TOKEN_HUNGRY) ? "HUNGRY" : (frame_token == TOKEN_NIGHT) ? "NIGHT" : "NORMAL";
    Serial.printf("[ML] Top: '%s' (%.2f) -> %s\n", top_label.c_str(), top_confidence, fr_str);

    ei_sleep(2000); // 2 seconds between inferences
}


// ======================================================
// 9. EDGE IMPULSE CAMERA FUNCTIONS
// ======================================================
bool ei_camera_init(void) {
    if (is_initialised) return true;

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s == NULL) {
        Serial.println("[ERR] sensor_get failed");
        return false;
    }

    /* 180° rotation: flip both axes (do NOT set only one - that mirrors) */
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);

    /* Optional tuning - keep if your board needs it */
    if (s->id.PID == OV3660_PID) {
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }

    Serial.printf("[CAM] PID=0x%04x vflip=%d hmirror=%d\n", s->id.PID, s->status.vflip, s->status.hmirror);

    is_initialised = true;
    return true;
}

void ei_camera_deinit(void) {
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        Serial.println("Camera deinit failed");
        return;
    }
    is_initialised = false;
}

/* [FIX] Software 180° rotation for the RGB888 buffer fed to the classifier.
   Reverses pixel order; ~5-10 ms on ESP32-S3 with PSRAM. */
static void rotate180_rgb888(uint8_t *buf, uint32_t w, uint32_t h) {
    uint32_t n = w * h;
    for (uint32_t i = 0; i < n / 2; i++) {
        uint32_t j = n - 1 - i;
        uint8_t r0 = buf[i*3+0], g0 = buf[i*3+1], b0 = buf[i*3+2];
        buf[i*3+0] = buf[j*3+0];
        buf[i*3+1] = buf[j*3+1];
        buf[i*3+2] = buf[j*3+2];
        buf[j*3+0] = r0;
        buf[j*3+1] = g0;
        buf[j*3+2] = b0;
    }
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    bool do_resize = false;
    if (!is_initialised) return false;

    g_stream_paused = true;
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        g_stream_paused = false;
        return false;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);
    g_stream_paused = false;

    if (!converted) {
        Serial.println("Conversion failed");
        return false;
    }

    /* [FIX] Software 180° rotation - belt and braces with the ISP flip */
    rotate180_rgb888(snapshot_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS);

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        do_resize = true;
    }

    if (do_resize) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }

    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    return 0;
}
