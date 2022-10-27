/*
 * Camera task - gets images and stores them in a ping-pong buffer for transmission by the socket_task
 */


// ==========================================================================
// Task variables
//


// ==========================================================================
// Task
//
void camera_task(void * parameter)
{
  int camera_img_index = 0;
  camera_fb_t * fb = NULL;
  esp_err_t res;
  size_t jpg_img_len;

  delay(100);
  Serial.println("camera_task starting");
  
  // Tasks never complete
  while (1) {
    if (is_connected) {
      if (!imgs[camera_img_index].img_ready) {
        // Get an image and store it in the ping-pong buffer
        fb = esp_camera_fb_get();
        if (fb) {
          imgs[camera_img_index].img_len = fb->len;
          memcpy(imgs[camera_img_index].img, fb->buf, fb->len);
          imgs[camera_img_index].img_ready = true;
          camera_img_index = (camera_img_index == 0) ? 1 : 0;
          esp_camera_fb_return(fb);
          fb = NULL;
        } else {
          Serial.println("Failed to acquire image");
        }
      } else {
        delay(10);
      }
    } else {
      // Keep in reset
      camera_img_index = 0;
      imgs[0].img_ready = false;
      imgs[1].img_ready = false;
      delay(100);
    }
  }
}
