/*
 * Remote control car/tank controller based on an ESP32-CAM from AI-Thinker.  Streams video to a remote control
 * which controls two motors for left/right drive and the ESP32-CAM flash LED as a headlight.
 * 
 *  - Generates a WiFi hotspot named "ESP32-CAM" with password "esp32cam"
 *  - Opens a socket to communicate with an external remote control
 *  - When connected sends a QVGA sized (320x240 pixel) MJPG stream over the socket to the remote control
 *  - Inteprets a simple ASCII command set for motor and LED control
 *    
 * Command set strings are terminated with a <CR> (Carriage Return - hex value 0x0D)
 *    M<N>=<V>  - <N> = 1 or 2 for motor channel, V = motor speed -255 to 255 (full reverse to full forward)
 *    L=<V>     - <V> = 0 or 1 to turn the LED off or on
 *  
 * MJPG stream images are delimited by 0xFFD8 and 0xFFD9 byte sequences.
 *  
 * Socket is at IP 192.168.4.1:6001
 *  
 * Designed to work with brushed DC motors controlled by a typical H-bridge motor driver that takes a
 * direction two 25 kHz PWM pins for control of each motor.  Put a 0.1 uF cap across the terminals of each  
 * motor.  You'll probably have to fool around with the polarity of each motor after you build your tank
 * and load the code to get them going in the correct direction.  See the github repository for more information.
 *  
 * The small red LED on the AI Thinker board is used as status.
 *  
 * Power on this controller first and then connect to it with the remote control.
 * 
 * Can be built using either the AI-Thinker or WROVER board types in Arduino.  The AI-Thinker ESP32-CAM
 * board requires an external USB-UART interface and manually grounding IO0 when powering on to enter bootloader
 * mode.  It is most helpful to disconnect the motor/H-bridge circuitry when building if you don't want the
 * tank crawling off your desk...
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
#include "esp_camera.h"
#include <WiFi.h>

// ==========================================================================
// User configuration
//

//
// Pins (configured for AI-Thinker ESP32-CAM)
//

// Camera pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Headlight LED
const int pin_led = 4;

// Motor Control pins
const int pin_m1_pwmA = 2;
const int pin_m1_pwmB = 14;
const int pin_m2_pwmA = 15;
const int pin_m2_pwmB = 13;

// Motor PWM frequency
#define PWM_FREQ  25000

// Status LED
const int pin_status = 33;




// ==========================================================================
// Internal global constants (shouldn't have to be changed)



//
// Camera configuration
//
#define XCLK_FREQ_HZ 16500000;


//
// Network configuration
//
#define PORT 6001

//
// Maximum jpeg image size (images are stored in PSRAM)
// We choose a very large value compared to the typical size of images because we have
// plenty of room up there in PSRAM!
//
#define MAX_JPG_LEN (128 * 1024)

//
// Wifi configuration
//
const char* ssid = "ESP32-CAM";
const char* password = "esp32cam";



// ==========================================================================
// Global variables
//

// State
bool is_connected = false;

// Jpeg image descriptor
typedef struct {
  bool img_ready;
  int img_len;
  uint8_t* img;
} img_descT;

// Jpeg image double buffer - One side is loaded with the next incoming image
// while the other side is being decoded and displayed
img_descT imgs[2];

// Tasks running on different CPUs
TaskHandle_t cam_task_handle;
TaskHandle_t socket_task_handle;

// Socket interface
WiFiServer wifiServer(PORT);



// ==========================================================================
// Arduino entry points
//
void setup() {
  Serial.begin(115200);
  Serial.println("esp32_cam_tank");

  // Allocate the image buffers in PSRAM
  imgs[0].img_ready = false;
  imgs[0].img = (uint8_t*) heap_caps_malloc(MAX_JPG_LEN, MALLOC_CAP_SPIRAM);
  if (imgs[0].img == NULL) {
    Serial.println("Malloc 0 failed");
    error_blink();
  }
  imgs[1].img_ready = false;
  imgs[1].img = (uint8_t*) heap_caps_malloc(MAX_JPG_LEN, MALLOC_CAP_SPIRAM);
  if (imgs[1].img == NULL) {
    Serial.println("Malloc 1 failed");
    error_blink();
  }

  // We assume that these init functions will be successful (no reason they shouldn't be)
  led_init();
  motor_init();
  wifi_init();
  
  // Blink the status LED as a hello
  led_set_status_on();
  delay(1000);
  led_set_status_off();

  // Hang with a fast status blink if we can't initialize the camera
  if (!camera_init()) {
   error_blink();
  }

  // Finally start tasks
  xTaskCreatePinnedToCore(camera_task, "cam_task", 8192, NULL, 1, &cam_task_handle, 1);       // App CPU
  xTaskCreatePinnedToCore(socket_task, "socket_task", 4096, NULL, 1, &socket_task_handle, 0); // Protocol CPU
  Serial.println("esp32_cam_tank ready to go");
}


void loop() {
  // Main loop does nothing
  delay(1000);
}


// ==========================================================================
// Subroutines
//

// Called on a fatal error - does not return
void error_blink()
{
  while (1) {
    led_set_status_on();
    delay(100);
    led_set_status_off();
    delay(100);
   }
}
