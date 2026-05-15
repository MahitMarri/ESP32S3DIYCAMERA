#include <Arduino.h>
#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// ===== XIAO ESP32-S3 SENSE CAMERA PINS (OV3660) =====
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

TFT_eSPI tft = TFT_eSPI();

bool camera_ready = false;
unsigned long frame_count = 0;
unsigned long fps_timer = 0;

// TJpg_Decoder callback — pushes each decoded MCU block to the display
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void startCamera() {
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
    config.frame_size   = FRAMESIZE_QVGA;      // 320x240 — matches ILI9341
    config.jpeg_quality = 12;                    // lower = better quality, slower
    config.fb_count     = 2;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location  = CAMERA_FB_IN_PSRAM;

    esp_err_t err = esp_camera_init(&config);
    if (err == ESP_OK) {
        camera_ready = true;
        Serial.println("Camera ready (OV3660 JPEG QVGA)");
    } else {
        Serial.printf("Camera init failed: 0x%x\n", err);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // Kill all wireless radios
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    // Hardware reset the display
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);
    delay(50);
    digitalWrite(TFT_RST, LOW);
    delay(150);
    digitalWrite(TFT_RST, HIGH);
    delay(150);

    // Initialise ILI9341
    tft.init();
    tft.setRotation(1);          // landscape 320x240
    tft.fillScreen(TFT_BLACK);

    // Set up JPEG decoder
    TJpgDec.setJpgScale(1);      // 1:1 — no scaling
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(60, 110);
    tft.println("Starting camera...");

    startCamera();

    if (!camera_ready) {
        tft.fillScreen(TFT_RED);
        tft.setCursor(30, 110);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextSize(2);
        tft.println("CAMERA FAILED");
    }

    fps_timer = millis();
}

void loop() {
    if (!camera_ready) return;

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return;

    // Decode JPEG and push to display
    TJpgDec.drawJpg(0, 0, (const uint8_t *)fb->buf, fb->len);
    esp_camera_fb_return(fb);

    // FPS counter on Serial
    frame_count++;
    unsigned long now = millis();
    if (now - fps_timer >= 1000) {
        Serial.printf("FPS: %.1f\n", frame_count * 1000.0f / (now - fps_timer));
        frame_count = 0;
        fps_timer = now;
    }
}
