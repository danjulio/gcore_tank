/*
 * Support functions related to motor control
 */


// ==========================================================================
// Motor constants
//

// PWM channels (avoid using channels also used by the camera library
#define PWM_CH_1A 4
#define PWM_CH_1B 5
#define PWM_CH_2A 6
#define PWM_CH_3B 7

// PWM Bits
#define PWM_NUM_BITS 10



// ==========================================================================
// Motor API
//
void motor_init()
{
  double f;
  
  pinMode(pin_m1_pwmA, OUTPUT);
  if ((f = ledcSetup(PWM_CH_1A, PWM_FREQ, PWM_NUM_BITS)) != PWM_FREQ) {
    Serial.printf("ledcSetup 1A returned %f (expected %d)\n", f, PWM_FREQ);
  }
  ledcAttachPin(pin_m1_pwmA, PWM_CH_1A);
  ledcWrite(PWM_CH_1A, 0);
  
  pinMode(pin_m1_pwmB, OUTPUT);
  if ((f = ledcSetup(PWM_CH_1B, PWM_FREQ, PWM_NUM_BITS)) != PWM_FREQ) {
    Serial.printf("ledcSetup 1B returned %f (expected %d)\n", f, PWM_FREQ);
  }
  ledcAttachPin(pin_m1_pwmB, PWM_CH_1B);
  ledcWrite(PWM_CH_1B, 0);

  pinMode(pin_m2_pwmA, OUTPUT);
  if ((f = ledcSetup(PWM_CH_2A, PWM_FREQ, PWM_NUM_BITS)) != PWM_FREQ) {
    Serial.printf("ledcSetup 2A returned %f (expected %d)\n", f, PWM_FREQ);
  }
  ledcAttachPin(pin_m2_pwmA, PWM_CH_2A);
  ledcWrite(PWM_CH_2A, 0);
  
  pinMode(pin_m2_pwmB, OUTPUT);
  if ((f = ledcSetup(PWM_CH_3B, PWM_FREQ, PWM_NUM_BITS)) != PWM_FREQ) {
    Serial.printf("ledcSetup 2B returned %f (expected %d)\n", f, PWM_FREQ);
  }
  ledcAttachPin(pin_m2_pwmB, PWM_CH_3B);
  ledcWrite(PWM_CH_3B, 0);
}


// motor : select motor (1: m1, 2: m2)
// speed : signed speed value (-255 to 255)
void motor_set(int motor, int speed)
{
  int mapped_speed;

  if (abs(speed) < 32) {
    mapped_speed = 0;
  } else {
    mapped_speed = map(abs(speed), 32, 255, 640, 1023);
  }
  
  if (motor == 1) {
    if (speed < 0) {
      ledcWrite(PWM_CH_1A, 0);
      ledcWrite(PWM_CH_1B, mapped_speed);
    } else {
      ledcWrite(PWM_CH_1A, mapped_speed);
      ledcWrite(PWM_CH_1B, 0);
    }
  } else if (motor == 2) {
    if (speed < 0) {
      ledcWrite(PWM_CH_2A, 0);
      ledcWrite(PWM_CH_3B, mapped_speed);
    } else {
      ledcWrite(PWM_CH_2A, mapped_speed);
      ledcWrite(PWM_CH_3B, 0);
    }
  }
}
