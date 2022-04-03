#ifndef DISPLAYDRIVER_H
#define DISPLAYDRIVER_H
#include "SiebenUhr.h"
#include "Glyph.h"
#include <EEPROM.h>
#include <RunningAverage.h>
#include <ezTime.h>
//#include <NtpClientLib.h>


class DisplayDriver {

static const int DEBUG_COMPUTION_TIME = 0;

public:
  DisplayDriver();
  ~DisplayDriver();
  uint16_t setup();
  void get_current_message(char* current_message);
  void set_notification(String notification, uint32_t milliseconds=0);
  void set_notification(char notification[4], uint32_t milliseconds=0);

  int is_notification_set();
  void disable_notification();
  void update();

  void set_color(const struct CHSV& color);
  void save_and_set_new_default_solid_color(const struct CHSV& color);
  void set_next_color(CHSV color, int interval_ms);

  String getStatus();
  uint8_t getPower();
  void setPower(uint8_t power);
  struct CRGB hexToRGB(String hex);
  String RGBToHex(struct CRGB rgbColor);
  // void setSolidColor(CRGB color);
  // void setSolidColor(uint8_t r, uint8_t g, uint8_t b);
  struct CHSV get_solid_color();
  String getSolidColorHex();

  /* set and get the current display effect of the clock. The display effect
     defines, _how_things are displayed on the clockmode in contrast to the
     operations mode that defines _what_ is being displayed */
  void set_display_effect(uint8_t value);
  uint8_t get_display_effect();
  void adjust_and_save_new_display_effect(bool up);
  String get_display_effect_json();
  const char* get_display_effect_short();


  void setPalette(uint8_t value);
  uint8_t getCurrentPalette();
  CRGBPalette16 _gCurrentPalette;
  uint8_t get_brightness();
  //void adjustBrightness(bool up);
  void set_brightness(uint8_t value);
  void save_and_set_new_default_brightness(uint8_t value);

  uint8_t get_brightness_index();
  void set_brightness_index(int value);
  void save_and_set_new_default_brightness_index(int value);

  uint8_t get_saturation();
  void save_and_set_new_default_saturation(int value);

  /* the color wheel changes the color of the clock one around the color
     wheel once in 24 hours. This angle define, at what time it should be
     which color. Factory preset is midnight = blue = 171 */
  uint8_t get_color_wheel_angle();
  void set_color_wheel_angle(uint8_t value);
  void save_new_color_wheel_angle(uint8_t value);

  void set_timezone_hour(int value);
  uint8_t get_timezone_hour();
  void save_and_set_new_default_timezone_hour(int value);

  void set_new_default_timezone(String inTimezone);

  void set_operations_mode(uint8_t mode);
  uint8_t get_operations_mode();

  void schedule_redraw();
  void schedule_redraw_with_special_blending_period(int blending_period);

  uint8_t mPhotoshooting_hour = 19;
  uint8_t mPhotoshooting_minute = 37;

private:
  Timezone _ezTimeTimezone;
  Glyph * glyphs[4];
  CRGB* _all_leds;
  uint8_t mNotification_set = 0; // is a notification currently set?
  uint8_t mLast_clock_update; // when was the clock updated the last time
  uint8_t _power;
  static const uint8_t _brightnessIndexCount = 43;
  uint8_t _brightnessMap[_brightnessIndexCount] = { 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 21, 24, 27, 30, 34, 38, 42, 46, 51, 56, 61, 66, 72, 78, 84, 92, 99, 106, 113, 120, 128, 136, 144, 153, 162, 171, 181, 191, 201, 212, 223, 235, 255 };
  int _brightnessIndex;
  uint8_t _brightness = _brightnessMap[_brightnessIndex];
  uint8_t mColor_wheel_angle = 0; // Index number of which color angle is current
  int8_t mTimezone = 1; // is a notification currently set?


  // char* _display_effects[11] = {
  // "Solid Color", "Color Waves", "Rainbow",  "Rainbow With Glitter", "Confetti", "Sinelon", "Juggle", "Pride", "Color BPM", "Palette Test", "Fire"
  // };
  const char* _display_effects[3] = {
    "Color Wheel", "Solid Color", "Random Color"
  };
  const char* _display_effects_short[3] = {
    "24h", "conS", "RAnd"
  };
  uint8_t _display_effects_count = 3;

  const char* _operation_mode[5] = {
    "Clock (hours)", "Clock (minutes)", "Message", "Progress Bar Bottom", "Progress Bar Complete"
  };
  uint8_t _operation_mode_count = 5;

  struct CHSV _solidColor;
  struct CHSV _solidColor_previous;
  uint8_t _gHue = 0; // rotating "base color" used by many of the display effects
  int _gHueSpeed = 20;
  unsigned long _gHueSpeedPrev = 0;        // will store last time LED was updated
  int _pChangeSpeed = 3000;
  unsigned long _pChangeSpeedPrev = 0;        // will store last time LED was updated
  int _pBlendingSpeed = 10000;
  bool _powerIsDown = false;
  unsigned int _autoPlayTimeout = 0;
  unsigned int mOperation_mode = OPERATION_MODE_CLOCK_HOURS;
  uint8_t mDisplay_effect = DISPLAY_EFFECT_DAYLIGHT_WHEEL;
  unsigned int mOperation_mode_before_notification = 0; // which ops mode was displayed before we displayed a notification? (in order to switch back after it)
  uint32_t mDisplay_notification_until = 0; // how many ms should a notification be displayed
  unsigned long last_update;

  char mCurrent_message[4];
  char mMessage_prior_to_notification[4];
  struct CHSV mSolid_color_prior_to_notification;

  bool mRedraw_scheduled = false; // set to true, if the clock requires a redraw
  int mNext_redraw_blending_period = BLENDIG_PERIOD; // for the next redraw, use this blending period
  // analyse the current compution time for the update cycle (for debugging)
  RunningAverage avg_compution_time;
  int compution_time_update_count = 0;

  // void set_next_message(int message[4]);
  void set_next_message(char message[4], int fade_interval=BLENDIG_PERIOD);

  // update the display for the different clock-modes
  void update_clock();
  void update_progress_bar();
  void update_progress_bar_complete();
  void update_display_as_notification();
  bool check_for_redraw();
  int check_for_special_blending_period();

  int _nLastTimePrintUpdate = 0;
  void print_current_time();

  // flag to store, wheter a deferred saving to EEPROM is scheduled
  bool deferred_saving_to_EEPROM_scheduled = false;
  // array to store the times, when the deferred EEPROM-saving needs to take place
  uint32_t deferred_saving_to_EEPROM_at[EEPROM_ADDRESS_COUNT] = { 0, 0, 0, 0, 0, 0};
  // array to store the values that have to be written, once the deferred EEPROM-saving takes place
  uint8_t  deferred_saving_to_EEPROM_value[EEPROM_ADDRESS_COUNT] = { 0, 0, 0, 0, 0, 0};

  void save_to_EEPROM();
  void deferred_saving_to_EEPROM(const int EEPROM_address, uint8_t value, uint32_t delay=10000);
  uint8_t read_from_EEPROM(uint8_t EEPROM_address);



};

#endif
