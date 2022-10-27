/*
 * Remote controller for ESP32-CAM-based 2-motor car/tank using gCore to 
 * display the mjpg stream and control the motors and LED headlight using
 * the dual-touch capability of the touchscreen controller.  Designed to 
 * work with the "esp32_cam_tank" sketch running on the ESP32-CAM.
 * 
 * The program displays a pair of sliders on either side of a 320-240 pixel
 * video feed from the ESP32-CAM.  Each slider controls one motor allowing for
 * speed control both backwards and forwards (tank-like).  The gCore power button
 * is re-purposed as a headlight on/off toggle.  Hold it down for five seconds
 * to switch gCore off.
 * 
 * Looks to connect to a WiFi network with the SSID "ESP32-CAM" and password
 * "esp32cam". 
 * 
 * Requires the following libraries:
 *    1. Bodmer's TFT_eSPI library ported to gCore (download from
 *       https://github.com/danjulio/TFT_eSPI)
 *    2. gCore library (https://github.com/danjulio/gCore2)
 *    3. TJpg_Decoder library
 *
 * Copyright 2022 Dan Julio.  All rights reserved.
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "gCore.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <Wire.h>


// ==========================================================================
// User configuration
//

//
// Maximum jpeg image size (images are stored in PSRAM)
// We choose a very large value compared to the typical size of images because we have
// plenty of room up there in PSRAM!
//
#define MAX_JPG_LEN (128 * 1024)

//
// Uncomment to use eSPI_TFT DMA
//
#define USE_DMA


//
// Slider touch sense and draw coordinates
//   Touch detection area is wider than displayed slider because fingers are fat and
//   that allows for more success as the user slides their fingers up and down alongside
//   the image.
#define SLIDER_TOUCH_WIDTH   80
#define SLIDER_TOUCH_HEIGHT  256
#define SLIDER_TOUCH_1_TOP   ((320 - SLIDER_TOUCH_HEIGHT) / 2)
#define SLIDER_TOUCH_1_LEFT  0
#define SLIDER_TOUCH_2_TOP   ((320 - SLIDER_TOUCH_HEIGHT) / 2)
#define SLIDER_TOUCH_2_LEFT  (480 - SLIDER_TOUCH_WIDTH)

#define SLIDER_DRAW_WIDTH    40
#define SLIDER_DRAW_RADIUS   ((SLIDER_DRAW_WIDTH - 2)/2 - 1)
#define SLIDER_DRAW_1_TOP    (SLIDER_TOUCH_1_TOP - (SLIDER_DRAW_WIDTH/2))
#define SLIDER_DRAW_1_BOT    (SLIDER_TOUCH_1_TOP + SLIDER_TOUCH_HEIGHT + (SLIDER_DRAW_WIDTH/2))
#define SLIDER_DRAW_1_LEFT   0
#define SLIDER_DRAW_1_RIGHT  (SLIDER_DRAW_1_LEFT + SLIDER_DRAW_WIDTH - 1)
#define SLIDER_DRAW_1_CENTER (SLIDER_DRAW_1_LEFT + (SLIDER_DRAW_WIDTH/2))
#define SLIDER_DRAW_2_TOP    (SLIDER_TOUCH_2_TOP - (SLIDER_DRAW_WIDTH/2))
#define SLIDER_DRAW_2_BOT    (SLIDER_TOUCH_2_TOP + SLIDER_TOUCH_HEIGHT + (SLIDER_DRAW_WIDTH/2))
#define SLIDER_DRAW_2_LEFT   (480 - SLIDER_DRAW_WIDTH)
#define SLIDER_DRAW_2_RIGHT  (SLIDER_DRAW_2_LEFT + SLIDER_DRAW_WIDTH - 1)
#define SLIDER_DRAW_2_CENTER (SLIDER_DRAW_2_LEFT + (SLIDER_DRAW_WIDTH/2))


//
// Wifi SSID/password strings - change these to connect to a different source
//
#define WIFI_SSID "ESP32-CAM"
#define WIFI_PASS "esp32cam"

//
// Stream URL string - change this to connect to a different source.  It should
// point to a raw mjpeg stream with resolution 480x320 or less.
//
#define REMOTE_IP   "192.168.4.1"
#define REMOTE_PORT 6001


// ==========================================================================
// Global variables
//

// Hardware objects
gCore gc;
TFT_eSPI tft = TFT_eSPI();


// Jpeg image descriptor
typedef struct {
  bool img_ready;
  int img_len;
  uint8_t* img;
} img_descT;

// Jpeg image double buffer - One side is loaded with the next incoming image
// while the other side is being decoded and displayed
img_descT imgs[2];

// Headlight button press detected to pass between tasks
bool headlight_button_pressed = false;

// Slider state shared between between tasks
typedef struct {
  int val;
  SemaphoreHandle_t mutex;
} slider_valT;

slider_valT slider_vals[2];


// Tasks running on different CPUs
TaskHandle_t com_task_handle;
TaskHandle_t disp_task_handle;



// ==========================================================================
// Arduino entry points
//
void setup() {
  Serial.begin(115200);

  // Setup gCore (begins Wire)
  gc.begin();
  gc.power_set_brightness(75);
  gc.power_set_button_short_press_msec(50);

  // Configure the LCD driver
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
#ifdef USE_DMA
  tft.initDMA();
#endif

  // Say hello
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  tft.setCursor(0, 20);
  tft.println("gcore_tank_controller starting");

  // Initialize the touchscreen controller
  if (!ts_begin()) {
    tft.setTextColor(TFT_RED);
    tft.println("Touchscreen initialization failed");
    while (1) {delay(100);}
  }

  // Configure the decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);  // Fastest way to swap bytes for 16bpp
  TJpgDec.setCallback(draw_pixels);

  // Allocate the image buffers in PSRAM
  imgs[0].img_ready = false;
  imgs[0].img = (uint8_t*) heap_caps_malloc(MAX_JPG_LEN, MALLOC_CAP_SPIRAM);
  if (imgs[0].img == NULL) {
    tft.setTextColor(TFT_RED);
    tft.println("Malloc 0 failed");
    while (1) {delay(100);}
  }
  imgs[1].img_ready = false;
  imgs[1].img = (uint8_t*) heap_caps_malloc(MAX_JPG_LEN, MALLOC_CAP_SPIRAM);
  if (imgs[1].img == NULL) {
    tft.setTextColor(TFT_RED);
    tft.println("Malloc 1 failed");
    while (1) {delay(100);}
  }

  // Setup the slider evaluation logic
  slider_init();

  // Initialize the slider touch values and mutexes
  slider_vals[0].val = 0;
  slider_vals[0].mutex = xSemaphoreCreateMutex();
  slider_vals[1].val = 0;
  slider_vals[1].mutex = xSemaphoreCreateMutex();

  // Connect to the camera
  tft.print("Connecting to ");
  tft.print(WIFI_SSID);
  tft.print(" ... ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  tft.println("connected");

  tft.println("Starting tasks");
  delay(1000);   // Delay to let user see

  // Start the display task on the Application CPU.  It will decode and display the
  // JPEG images and slider position.
  // Note: After it starts to draw images code running in the main arduino loop shouldn't
  // draw to the tft
  xTaskCreatePinnedToCore(disp_task, "disp_task", 4096, NULL, 1, &disp_task_handle, 1);
  delay(100);
  
  // Start the communication task on the Protocol CPU.  It will request and process
  // the incoming stream.
  xTaskCreatePinnedToCore(com_task, "com_task", 4096, NULL, 1, &com_task_handle, 0);
}


// Main loop running on CPU 1 polls input I2C devices 
void loop() {
  int cur_slider_val[2];
  
  // Check for touch update
  slider_eval();
  for (int i=0; i<2; i++) {
    cur_slider_val[i] = slider_get_val(i);
    xSemaphoreTake(slider_vals[i].mutex , portMAX_DELAY);
    slider_vals[i].val = cur_slider_val[i];
    xSemaphoreGive(slider_vals[i].mutex);
  }

  // Check for headlight on/off button press
  if (gc.power_button_pressed()) {
    headlight_button_pressed = true;
  }
  
  delay(48);  // Approx 50 mSec total with I2C cycles
}
