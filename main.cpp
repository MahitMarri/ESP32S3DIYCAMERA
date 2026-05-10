#include <Arduino.h>
#include "esp_camera.h"
#include "WiFi.h"
#include "esp_bt.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// ===== BUTTON PIN =====
#define capturePin D0

// ===== CORRECT XIAO ESP32-S3 SENSE PINOUT =====
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39
#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

bool camera_ready = false;
int imageCount = 1; // Renamed variable to avoid colliding with the core driver naming structure
camera_config_t my_cam_config;

void configureCameraSettings() {
  my_cam_config.ledc_channel = LEDC_CHANNEL_0;
  my_cam_config.ledc_timer = LEDC_TIMER_0;
  my_cam_config.pin_d0 = Y2_GPIO_NUM;
  my_cam_config.pin_d1 = Y3_GPIO_NUM;
  my_cam_config.pin_d2 = Y4_GPIO_NUM;
  my_cam_config.pin_d3 = Y5_GPIO_NUM;
  my_cam_config.pin_d4 = Y6_GPIO_NUM;
  my_cam_config.pin_d5 = Y7_GPIO_NUM;
  my_cam_config.pin_d6 = Y8_GPIO_NUM;
  my_cam_config.pin_d7 = Y9_GPIO_NUM;
  my_cam_config.pin_xclk = XCLK_GPIO_NUM;
  my_cam_config.pin_pclk = PCLK_GPIO_NUM;
  my_cam_config.pin_vsync = VSYNC_GPIO_NUM;
  my_cam_config.pin_href = HREF_GPIO_NUM;
  my_cam_config.pin_sccb_sda = SIOD_GPIO_NUM;
  my_cam_config.pin_sccb_scl = SIOC_GPIO_NUM;
  my_cam_config.pin_pwdn = PWDN_GPIO_NUM;
  my_cam_config.pin_reset = RESET_GPIO_NUM;
  my_cam_config.xclk_freq_hz = 20000000;
  my_cam_config.pixel_format = PIXFORMAT_JPEG;
}

void startStreamingMode() {
  if(camera_ready) esp_camera_deinit();
  configureCameraSettings();
  my_cam_config.frame_size = FRAMESIZE_VGA; // Fast 640x480 preview stream
  my_cam_config.jpeg_quality = 12;
  my_cam_config.fb_count = 2; // Double buffering
  my_cam_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  if (esp_camera_init(&my_cam_config) == ESP_OK) {
    camera_ready = true;
  }
}

void takeAndSaveHighResSnapshot() {
  if(camera_ready) {
    esp_camera_deinit();
    camera_ready = false;
  }
  delay(100);
  configureCameraSettings();
  my_cam_config.frame_size = FRAMESIZE_QXGA; // Native 3MP (2048 x 1536)
  // ===== MODIFY THIS LINE FOR MAXIMUM QUALITY =====
  my_cam_config.jpeg_quality = 5; // Lower number = massive increase in image detail/crispness
  my_cam_config.fb_count = 1;
  my_cam_config.grab_mode = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&my_cam_config) != ESP_OK) {
    Serial.println("High-res initialization aborted");
    startStreamingMode();
    return;
  }
  // ... [rest of your takeAndSaveHighResSnapshot code remains exactly the same]
  delay(300);
  for(int i=0; i<2; i++) {
    camera_fb_t * dummy = esp_camera_fb_get();
    if(dummy) esp_camera_fb_return(dummy);
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame buffer capture timed out.");
    esp_camera_deinit();
    startStreamingMode();
    return;
  }
  if(SD.begin(21)) {
    char filename[32]; // Fixed buffer syntax allocation configuration
    sprintf(filename, "/photo_%d.jpg", imageCount);
    Serial.printf("Writing to SD card: %s\n", filename);
    File file = SD.open(filename, FILE_WRITE);
    if(file) {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.println("SUCCESS: Photo saved cleanly!");
      imageCount++;
    }
    else {
      Serial.println("Write permission denied. Check lock tab.");
    }
    SD.end();
  }
  else {
    Serial.println("Card mount timeout.");
  }
  esp_camera_fb_return(fb);
  esp_camera_deinit();
  delay(100);
  startStreamingMode();
}

void setup() {
  Serial.begin(115200);
  pinMode(capturePin, INPUT_PULLUP);
  WiFi.mode(WIFI_OFF);
  btStop();
  startStreamingMode();
  Serial.println("Streaming initialized. Press button to trigger snapshot save.");
}

void loop() {
  if (digitalRead(capturePin) == 0) {
    takeAndSaveHighResSnapshot();
    while(digitalRead(capturePin) == 0) {
      delay(10);
    }
  }
  if (camera_ready) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      Serial.write("FRAME", 5);
      uint32_t size = fb->len;
      Serial.write((uint8_t*)&size, 4);
      Serial.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);
    }
  }
  delay(20);
}
