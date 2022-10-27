/*
 * Support for dual sliders.  Hacky, I know. This should be an object for
 * each slider but I was being lazy.
 */

// ==========================================================================
// Slider constants
//

//
// Jump amount for each evaluation when returning to 0
//
#define SLIDER_JUMP_VAL 64



// ==========================================================================
// Slider Variables
//

//
// State
//
bool raw_slider_touched[2];
int raw_slider_val[2];



// ==========================================================================
// Slider API
//
void slider_init()
{
  raw_slider_val[0] = 0;
  raw_slider_val[1] = 0;
}


void slider_eval()
{
  int i;
  int n;
  int v;
  int num_hits;
  int16_t x, y;
  int touch_val[2];

  // Assume no touch at start - will be set if necessary
  raw_slider_touched[0] = false;
  raw_slider_touched[1] = false;
  
  // Look for touch and see if it hits our slider detection areas
  num_hits = ts_get_num_points();
  if (num_hits > 0) {
    for (i=0; i<num_hits; i++) {
      ts_get_point(i, &x, &y);
      if (slider_hit(x, y, &n, &v)) {
        touch_val[n] = v;
        raw_slider_touched[n] = true;
      }
    }
  }

  // Update slider values
  for (i=0; i<2; i++) {
    if (raw_slider_touched[i]) {
      // Touched sliders update value immediately
      raw_slider_val[i] = touch_val[i];
    } else {
      // Untouched sliders return to 0 over a short time
      if (raw_slider_val[i] != 0) {
        n = (abs(raw_slider_val[i]) > SLIDER_JUMP_VAL) ? SLIDER_JUMP_VAL : abs(raw_slider_val[i]);
        if (raw_slider_val[i] > 0) n = -1 * n;  // Decrease positive values, increase negative values
        raw_slider_val[i] += n;
      }
    }
  }
}


int slider_get_val(int n)
{
  return raw_slider_val[n];
}


// ==========================================================================
// Slider routines
//
// Check x, y point against both sliders and return true if there is a hit, setting the
// slider number (n) and value (v).  A slight trick: We count any touch lower or higher
// than the slider as still being in the slider but at the lowest or highest value since
// it is so easy to overrun them.
bool slider_hit(int x, int y, int* n, int* v)
{
  bool success = false;
  int i;
  int top;
  int bot;
  int left;
  int right;
  int raw_pos;

  for (i=0; i<2; i++) {
    top = (i == 0) ? SLIDER_TOUCH_1_TOP : SLIDER_TOUCH_2_TOP;
    bot = top + SLIDER_TOUCH_HEIGHT;
    left = (i == 0) ? SLIDER_TOUCH_1_LEFT : SLIDER_TOUCH_2_LEFT;
    right = left + SLIDER_TOUCH_WIDTH;

    if ((x >= left) && (x <= right)) {
      // Clip to top/bottom
      if (y < top) {
        y = top;
      } else if (y > bot) {
        y = bot;
      }
      success = true;
      *n = i;
      break;
    }
  }

  if (success) {
    // Convert position to value (-255 to 255)
    raw_pos = y - top;  // 0 to SLIDER_TOUCH_HEIGHT
    if (raw_pos <= (SLIDER_TOUCH_HEIGHT / 2)) {
      // Scale (top to (SLIDER_TOUCH_HEIGHT/2) -> 255 to 0
      *v = 255 - ((255 * raw_pos)/(SLIDER_TOUCH_HEIGHT/2));
    } else {
      // Scale (SLIDER_TOUCH_HEIGHT/2) to bot -> 0 to -255
      *v = (-255 * (raw_pos - (SLIDER_TOUCH_HEIGHT/2)))/(SLIDER_TOUCH_HEIGHT/2);
    }
    //Serial.printf("slider %d = %d\n", *n, *v);
  }
  return success;
}
