/*
 * Support functions related to Wifi
 */

// ==========================================================================
// Wifi API
void wifi_init()
{
 WiFi.mode(WIFI_AP);
 WiFi.softAP(ssid, password); 
}


bool wifi_connected()
{
  return (WiFi.status() == WL_CONNECTED);
}
