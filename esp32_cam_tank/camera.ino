/*
 * Functions related to the camera
 */

// ==========================================================================
// Camera variables
//
camera_config_t cam_config;
sensor_t* cam_dev;


// ==========================================================================
// Camera API
//
bool camera_init()
{
  esp_err_t err;
  
  cam_config.ledc_channel = LEDC_CHANNEL_0;
  cam_config.ledc_timer = LEDC_TIMER_0;
  cam_config.pin_d0 = Y2_GPIO_NUM;
  cam_config.pin_d1 = Y3_GPIO_NUM;
  cam_config.pin_d2 = Y4_GPIO_NUM;
  cam_config.pin_d3 = Y5_GPIO_NUM;
  cam_config.pin_d4 = Y6_GPIO_NUM;
  cam_config.pin_d5 = Y7_GPIO_NUM;
  cam_config.pin_d6 = Y8_GPIO_NUM;
  cam_config.pin_d7 = Y9_GPIO_NUM;
  cam_config.pin_xclk = XCLK_GPIO_NUM;
  cam_config.pin_pclk = PCLK_GPIO_NUM;
  cam_config.pin_vsync = VSYNC_GPIO_NUM;
  cam_config.pin_href = HREF_GPIO_NUM;
  cam_config.pin_sscb_sda = SIOD_GPIO_NUM;
  cam_config.pin_sscb_scl = SIOC_GPIO_NUM;
  cam_config.pin_pwdn = PWDN_GPIO_NUM;
  cam_config.pin_reset = RESET_GPIO_NUM;
  cam_config.xclk_freq_hz = XCLK_FREQ_HZ;
  cam_config.pixel_format = PIXFORMAT_JPEG;
  //cam_config.frame_size = FRAMESIZE_CIF;
  cam_config.frame_size = FRAMESIZE_QVGA;
  cam_config.jpeg_quality = 15;
  cam_config.fb_count = 2;

  err = esp_camera_init(&cam_config);
  if (err != ESP_OK) {
    delay(100);
    Serial.printf("Could not start camera.  err = %d\n", err);
    return false;
  }

  cam_dev = esp_camera_sensor_get();
  switch (cam_dev->id.PID) {
    case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
    case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
    case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
    case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
    default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
  }

  // Following can be uncommented to modify camera settings for a better image if you desire.
  //
  //cam_dev->set_quality(s, val);       // 10 to 63
  //cam_dev->set_brightness(s, 0);      // -2 to 2
  //cam_dev->set_contrast(s, 0);        // -2 to 2
  //cam_dev->set_saturation(s, 0);      // -2 to 2
  //cam_dev->set_special_effect(s, 0);  // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  //cam_dev->set_whitebal(s, 1);        // aka 'awb' in the UI; 0 = disable , 1 = enable
  //cam_dev->set_awb_gain(s, 1);        // 0 = disable , 1 = enable
  //cam_dev->set_wb_mode(s, 0);         // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  //cam_dev->set_exposure_ctrl(s, 1);   // 0 = disable , 1 = enable
  //cam_dev->set_aec2(s, 0);            // 0 = disable , 1 = enable
  //cam_dev->set_ae_level(s, 0);        // -2 to 2
  //cam_dev->set_aec_value(s, 300);     // 0 to 1200
  //cam_dev->set_gain_ctrl(s, 1);       // 0 = disable , 1 = enable
  //cam_dev->set_agc_gain(s, 0);        // 0 to 30
  //cam_dev->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  //cam_dev->set_bpc(s, 0);             // 0 = disable , 1 = enable
  //cam_dev->set_wpc(s, 1);             // 0 = disable , 1 = enable
  //cam_dev->set_raw_gma(s, 1);         // 0 = disable , 1 = enable
  //cam_dev->set_lenc(s, 1);            // 0 = disable , 1 = enable
  //cam_dev->set_hmirror(s, 0);         // 0 = disable , 1 = enable
  //cam_dev->set_vflip(s, 0);           // 0 = disable , 1 = enable
  //cam_dev->set_dcw(s, 1);             // 0 = disable , 1 = enable
  //cam_dev->set_colorbar(s, 0);        // 0 = disable , 1 = enable

  return true;
}
