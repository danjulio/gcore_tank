/*
 * Support functions related to the headlight and status LEDs
 */

// ==========================================================================
// LED API
//
void led_init()
{
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_led, LOW);      /* Headlight LED is active high */
  pinMode(pin_status, OUTPUT);
  digitalWrite(pin_status, HIGH);  /* Status LED is active low */
}


void led_set_headlight_on()
{
  digitalWrite(pin_led, HIGH);
}


void led_set_headlight_off()
{
  digitalWrite(pin_led, LOW);
}


void led_set_status_on()
{
  digitalWrite(pin_status, LOW);
}


void led_set_status_off()
{
  digitalWrite(pin_status, HIGH);
}
