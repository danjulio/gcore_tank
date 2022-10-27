/*
 * Display task
 * 
 * Decode and display images from the ping-pong buffer. Also update the slider
 * positions based on the shared slider_vals data structure. Display the average
 * frames-per-sec under the image.
 *
 * This task is typically the bottleneck because of the JPEG decode (the more complex
 * and larger the iamge the slower the decode).
 */
 

// ==========================================================================
// Display Constants
//

// TFT_eSPI internal DMA buffer
#define DMA_BUF_LEN    512

// Number of frames to use to compute average FPS
#define NUM_FPS_FRAMES 5



// ==========================================================================
// Display Variables
//
uint16_t dma_buf[DMA_BUF_LEN/2];

// Last drawn slider position
int disp_slider_val[2];



// ==========================================================================
// Task
//
void disp_task(void * parameter)
{
  int disp_img_index = 0;  // Ping-pong buffer index
  int frame_count = 0;
  int i;
  int cur_slider_val[2];
  float fps;
  JRESULT res;
  unsigned long timestamp;

  // Clear screen of text
  tft.fillScreen(TFT_BLACK);

  // Draw slider outlines
  tft.drawRect(SLIDER_DRAW_1_LEFT, SLIDER_DRAW_1_TOP, SLIDER_DRAW_WIDTH, SLIDER_TOUCH_HEIGHT + SLIDER_DRAW_WIDTH, TFT_DARKGREY);
  tft.drawRect(SLIDER_DRAW_2_LEFT, SLIDER_DRAW_2_TOP, SLIDER_DRAW_WIDTH, SLIDER_TOUCH_HEIGHT + SLIDER_DRAW_WIDTH, TFT_DARKGREY);
  for (i=0; i<2; i++) {
    disp_slider_val[i] = 0;
    draw_slider_control(i, disp_slider_val[i], false);
  }
  
  while (true) {    
    // Update touch controls if necessary
    for (i=0; i<2; i++) {
      xSemaphoreTake(slider_vals[i].mutex , portMAX_DELAY);
      cur_slider_val[i] = slider_vals[i].val;
      xSemaphoreGive(slider_vals[i].mutex);
      if (cur_slider_val[i] != disp_slider_val[i]) {
        draw_slider_control(i, disp_slider_val[i], true);  // Erase old value
        disp_slider_val[i] = cur_slider_val[i];
        draw_slider_control(i, disp_slider_val[i], false); // Draw new value
      }
    }
    
    // Display available images
    if (imgs[disp_img_index].img_ready) {
      if (frame_count == 0) {
        timestamp = millis();
      }
    
#ifdef USE_DMA
      tft.startWrite();
#endif
      res = TJpgDec.drawJpg(80, 40, imgs[disp_img_index].img, (uint32_t) imgs[disp_img_index].img_len);
#ifdef USE_DMA
      tft.endWrite();
#endif
      if (res != JDR_OK) {
        Serial.printf("drawJpg failed with %d\n", (int) res);
      }

      // Setup for next image
      imgs[disp_img_index].img_ready = false;
      if (++disp_img_index >= 2) disp_img_index = 0;

      // Update FPS in middle bottom
      if (++frame_count == NUM_FPS_FRAMES) {
        fps = (frame_count * 1000.0) / (float) (millis() - timestamp);
        frame_count = 0;
        tft.fillRect(215, 310, 30, 10, TFT_BLACK);
        tft.setCursor(215, 310);
        tft.print(fps);
      }
    }

    // Allow FreeRTOS scheduler access (WDT)
    delay(10);
  }
}



// ==========================================================================
// JPEG decode display callback
//
bool draw_pixels(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) return false;

#ifdef USE_DMA
  // Load the buffer for an asynchronous DMA (control falls out of pushImageDMA before
  // DMA is finished by copying bitmap to dma_buf and using that for the DMA)
  tft.dmaWait();
  tft.pushImageDMA(x, y, w, h, bitmap, dma_buf);
#else
  tft.pushImage(x, y, w, h, bitmap);
#endif

  // Return true to decode next block
  return true;
}


// ==========================================================================
// disp_task routines
//
void draw_slider_control(int n, int v, bool erase_control)
{
  int x, y;
  int top;

  // Compute center point of control
  x = (n == 0) ? SLIDER_DRAW_1_CENTER : SLIDER_DRAW_2_CENTER;

  top = (n == 0) ? SLIDER_TOUCH_1_TOP : SLIDER_TOUCH_2_TOP;
  if (v >= 0) {
    // Top half: v = 0 - 255 maps to y = (top + (SLIDER_TOUCH_HEIGHT/2)) to top [0-255 -> 168-40]
    y = top + (SLIDER_TOUCH_HEIGHT/2) - ((SLIDER_TOUCH_HEIGHT/2) * v / 255);
  } else {
    // Bottom half: v = -1 - -255 maps to y = (top + (SLIDER_TOUCH_HEIGHT/2) + 1) to (top + SLIDER_TOUCH_HEIGHT)
    y = top + (SLIDER_TOUCH_HEIGHT/2) + (SLIDER_TOUCH_HEIGHT/2)*(-1 * v)/255;
  }

  if (erase_control) {
    tft.fillCircle(x, y, SLIDER_DRAW_RADIUS, TFT_BLACK);
  } else {
    tft.fillCircle(x, y, SLIDER_DRAW_RADIUS, TFT_OLIVE);
  }
}
