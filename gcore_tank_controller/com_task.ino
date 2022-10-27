/*
 * Communication task
 * 
 * Connect to the remote socket on the ESP32-CAM and process images and
 * send commands through the socket as necessary.  Images are parsed into
 * individual jpeg images and stored in the ping-pong buffer shared with
 * disp_task.
 */


// ==========================================================================
// Com Constants
//

// JPEG stream decode state
#define ST_IDLE    0
#define ST_FF_1    1
#define ST_IN_JPEG 2
#define ST_FF_2    3


//
// Maximum number of bytes in an jpeg image chunk to process at a time
//
#define MAX_JPEG_CHUNK 2048


// ==========================================================================
// Com Variables
//

// Last detected slider position
int com_slider_val[2];

// mjpg parser state
int parse_state = ST_IDLE;

// Headlight state (toggles on button press notification)
bool headlight_on = false;

// Socket interface
WiFiClient client;

// jpeg image chunk buffer (allocated in fast memory)
uint8_t jpeg_chunk_buffer[MAX_JPEG_CHUNK];



// ==========================================================================
// Task
//
void com_task(void * parameter)
{
  int chunk_len = 0;
  int read_len = 0;

  while (true) {
    // Wait for WiFi connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
    }

    // Init state
    parse_state = ST_IDLE;
    com_slider_val[0] = 0;
    com_slider_val[1] = 0;
    
    Serial.println("Connecting to server");
    if (client.connect(REMOTE_IP, REMOTE_PORT)) {
      // Process data from the stream in chunks so we can responsively process commands to send
      while (client.connected()) {
        // Process any outgoing commands
        com_process_commands();
        
        // Look for new incoming image data if we don't already have some to process
        if (read_len == 0) {
          read_len = client.available();
        }

        // Process image data
        if (read_len != 0) {
          chunk_len = (read_len > MAX_JPEG_CHUNK) ? MAX_JPEG_CHUNK : read_len;
          com_process_data(chunk_len);
          read_len -= chunk_len;
        } else {
          // Delay to let FreeRTOS scheduler work (WDT)
          delay(10);
        }
      }
      Serial.println("Server disconnected");
      client.stop();
    } else {
      Serial.println("Could not connect to server");
      delay(1000);
    }
  }
}



// ==========================================================================
// Com Routines
//
void com_process_data(int chunk_len)
{
  static bool push_img = false;
  static int com_img_index = 0;  // Ping-pong buffer index
  static uint8_t* imgP;
  int read_len;
  uint8_t* cP = jpeg_chunk_buffer;

  // Read the chunk into a local buffer for faster access
  read_len = client.read(jpeg_chunk_buffer, (size_t) chunk_len);
  
  while (read_len-- > 0) {
   switch (parse_state) {
      case ST_IDLE:
        if (*cP == 0xFF) {
          parse_state = ST_FF_1;
        }
        break;
      
      case ST_FF_1:
        if (*cP == 0xD8) {
          // Start of JPEG image - only load if the buffer is available, otherwise just
          // throw this image away
          if (!imgs[com_img_index].img_ready) {
            push_img = true;
            imgP = imgs[com_img_index].img;
            *imgP++ = 0xFF;
            *imgP++ = *cP;
          } else {
            push_img = false;
          }
          parse_state = ST_IN_JPEG;
        } else {
          parse_state = ST_IDLE;
        }
        break;
      
      case ST_IN_JPEG:
        if (push_img) *imgP++ = *cP;
        if (*cP == 0xFF) {
          parse_state = ST_FF_2;
        }
        break;
      
      case ST_FF_2:
        if (push_img) *imgP++ = *cP;
        if (*cP == 0xD9) {
          // End of JPEG image - Let display task know and move to the next ping-pong half
          parse_state = ST_IDLE;
          if (push_img) {
            imgs[com_img_index].img_len = imgP - imgs[com_img_index].img;
            imgs[com_img_index].img_ready = true;
            if (++com_img_index >= 2) com_img_index = 0;
          }
        } else {
          parse_state = ST_IN_JPEG;
        }
        break;
    }

    cP++;
  }
}


void com_process_commands()
{
  int cur_slider_val;
  
  for (int i=0; i<2; i++) {
    xSemaphoreTake(slider_vals[i].mutex , portMAX_DELAY);
    cur_slider_val = slider_vals[i].val;
    xSemaphoreGive(slider_vals[i].mutex);
    if (cur_slider_val != com_slider_val[i]) {
      com_slider_val[i] = cur_slider_val;
      client.printf("M%d=%d\r", i+1, cur_slider_val);
    }
  }
  
  if (headlight_button_pressed) {
    headlight_button_pressed = false;
    headlight_on = !headlight_on;
    client.printf("L=%d\r", headlight_on ? 1 : 0);
  }
}
