// Adafruit_DotStarMatrix example for single DotStar LED matrix.
// Scrolls 'Adafruit' across the matrix.

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_DotStarMatrix.h>
#include <Adafruit_DotStar.h>
#include <Fonts/TomThumb.h>

#if defined(ESP8266)
  #define DATAPIN    13
  #define CLOCKPIN   14
#elif defined(__AVR_ATmega328P__)
  #define DATAPIN    2
  #define CLOCKPIN   4
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define DATAPIN    7
  #define CLOCKPIN   16
#elif defined(TEENSYDUINO)
  #define DATAPIN    9
  #define CLOCKPIN   5
#elif defined(ARDUINO_ARCH_WICED)
  #define DATAPIN    PA4
  #define CLOCKPIN   PB5
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define DATAPIN    27
  #define CLOCKPIN   13
#else // // 32u4, M0, M4, esp32-s2, nrf52840 and 328p
  #define DATAPIN    11
  #define CLOCKPIN   13
#endif

#define SHIFTDELAY 100

// MATRIX DECLARATION:
// Parameter 1 = width of DotStar matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   DS_MATRIX_TOP, DS_MATRIX_BOTTOM, DS_MATRIX_LEFT, DS_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     DS_MATRIX_TOP + DS_MATRIX_LEFT for the top-left corner.
//   DS_MATRIX_ROWS, DS_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   DS_MATRIX_PROGRESSIVE, DS_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type:
//   DOTSTAR_BRG  Pixels are wired for BRG bitstream (most DotStar items)
//   DOTSTAR_GBR  Pixels are wired for GBR bitstream (some older DotStars)
//   DOTSTAR_BGR  Pixels are wired for BGR bitstream (APA102-2020 DotStars)

Adafruit_DotStarMatrix matrix = Adafruit_DotStarMatrix(
                                  12, 6, DATAPIN, CLOCKPIN,
                                  DS_MATRIX_BOTTOM     + DS_MATRIX_LEFT +
                                  DS_MATRIX_ROWS + DS_MATRIX_PROGRESSIVE,
                                  DOTSTAR_BGR);

#define COLOR_BLACK   (matrix.Color(  0,   0,   0))
#define COLOR_RED     (matrix.Color(255,   0,   0))
#define COLOR_GREEN   (matrix.Color(  0, 255,   0))
#define COLOR_BLUE    (matrix.Color(  0,   0, 255))
#define COLOR_YELLOW  (matrix.Color(200, 255,   0))
#define COLOR_CYAN    (matrix.Color(  0, 255, 225))
#define COLOR_PINK    (matrix.Color(255,   0, 220))
#define COLOR_WHITE   (matrix.Color(255, 255, 255))
#define COLOR_MULTI   (matrix.Color(  0,   0,   1)) // special value: use value near black
#define COLOR_SPARKLE (matrix.Color(100, 100, 100)) // special value

const uint16_t primaryColors[] = {
  COLOR_RED, COLOR_GREEN, COLOR_BLUE
};

const uint16_t text_colors[] = {
  COLOR_RED,
  COLOR_YELLOW,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_BLUE,
  COLOR_PINK,
};
const int text_color_len = sizeof (text_colors) / sizeof (uint16_t);

enum bright_e {
  bright_dim = 1,
  bright_mid = 2,
  bright_max = 20, // too bright
};

enum speed_e {
  speed_slow = SHIFTDELAY,
  speed_med_slow = (unsigned long)((double)SHIFTDELAY/1.5),
  speed_mid  = SHIFTDELAY/2,
  speed_fast = SHIFTDELAY/3, // smooth!
};

struct msg_s {
  uint16_t    msg_color;
  bool        do_blink;
  int         bounce_count;
  uint8_t     brightness;
  unsigned long shift_ms;
  const char *msg_str;
};
struct msg_s msg_list[] = {
  { COLOR_MULTI,   false, 4, bright_dim, speed_fast,  "YOUR NAME", },
  { COLOR_MULTI,   false, 3, bright_dim, speed_fast, "HI", },
  { COLOR_GREEN,   false, 0, bright_dim, speed_med_slow, "HAPPY BIRTHDAY!", },
  { COLOR_SPARKLE, false, 0, bright_dim, speed_mid,  "I'M GOING ON AN ADVENTURE!", },
  { COLOR_MULTI,   false, 0, bright_dim, speed_slow, "ARE YOU GOING?", },
  { COLOR_WHITE,   false, 2, bright_mid, speed_slow,  "LET'S GO!!", },
};
int msg_count = sizeof (msg_list) / sizeof (msg_list[0]);

void setup() {
  Serial.begin(115200);
 
  // uncomment to have wait
  //while (!Serial) delay(500); 

  Serial.println("\nDotstar Matrix Wing");
  matrix.begin();
  matrix.setFont(&TomThumb);
  matrix.setTextWrap(false);
  matrix.setBrightness(bright_dim);

  for (byte i = 0; i < 3; i++) {
    matrix.fillScreen(primaryColors[i]);
    matrix.show();
    delay(500);
  }
}

int pass = 0;
int color_idx = 0;
int msg_idx = 0;
int bounce_idx = 0;

int screen_width = 12;
int blank_width = 8;
int x_start = matrix.width();
int shift_dir = -1; // start with first character and shift them left (-x) to see more
                  // 1: start with last character and shift them right, +x, to see more.
                  //  (better w/ short messages, b/c we don't read that way.)
int x_shift = x_start; // start at position for doing -1 (move to the left) direction.
bool blink_state = false;

void loop() {
  const char *msg_str;
  int msg_len;
  int msg_width;
  int x_end;
  uint16_t char_color;
  struct msg_s *this_msg;

  this_msg = &msg_list[msg_idx]; 
  msg_str = this_msg->msg_str;
  msg_len = strlen(msg_str);
  msg_width = msg_len * 4;
  x_end = -(msg_width + blank_width - screen_width);

  matrix.fillScreen(0);
  matrix.setCursor(x_shift, 5);

  if (this_msg->msg_color == COLOR_MULTI) {
    color_idx = 0;
  }
  for (byte char_idx = 0; char_idx < msg_len; char_idx++) {
    // set the color
    if (this_msg->msg_color == COLOR_MULTI) {
      // each character is always the same color.
      char_color = text_colors[color_idx % text_color_len];
      if (msg_str[char_idx] != ' ') { // only if we are not drawing a space
        color_idx++; // move on to the net color
      }
    } else if (this_msg->msg_color == COLOR_SPARKLE) {
      // the same character is a different color every time it is drawn -- very hard to look at.
      if (color_idx > text_color_len) {
        color_idx = 0;
      }
      char_color = text_colors[color_idx];
      if (msg_str[char_idx] != ' ') { // only if we are not drawing a space
        color_idx++; // move on to the net color
      }
    } else { // NOT COLOR_MULTI or Sparkle
      char_color = this_msg->msg_color;
    }
    matrix.setTextColor(char_color);
    // write the letter
    matrix.print(msg_str[char_idx]);
  }

  // Move left
  bool next_message = false;
  if (shift_dir < 0) {
    x_shift--;
    if (x_shift < x_end) {
      bounce_idx++;
      if (bounce_idx <= this_msg->bounce_count) {
        shift_dir = -shift_dir; // bounce other direction
      } else { // bounced enough
        next_message = true; // go on to the next message
      }
    }
  } else { //if (shift_dir > 0) {
    x_shift++;
    if (x_shift > x_start) {
      bounce_idx++;
      if (bounce_idx <= this_msg->bounce_count) {
        shift_dir = -shift_dir; // bounce other direction
      } else { // bounced enough
        next_message = true; // go on to the next message
      }
    }
  }

  if (this_msg->do_blink) {
    if (blink_state) {
      matrix.fillScreen(COLOR_BLACK);
    }
    blink_state = !blink_state;
  }

  if (next_message) {
    msg_idx++;
    bounce_idx = 0;
    this_msg = &msg_list[msg_idx]; 
    if (msg_idx >= msg_count) {
      msg_idx = 0;
      this_msg = &msg_list[msg_idx]; 
    }
    matrix.setBrightness(this_msg->brightness);
    shift_dir = -1; // each message starts by doing -1 (move to the left) direction.
    x_shift = x_start; // start at the position for moving to the left
  }

  matrix.show();
  delay(this_msg->shift_ms); // character scrolling delay depends on message
}