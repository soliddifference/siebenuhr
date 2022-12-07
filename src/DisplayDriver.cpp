/*  Display.cpp -
	v0.1 tg 20180211 initial setup

  Class to handle the display holding some glyphs (most likely 4) for the 7clock

*/
#include <Arduino.h>

#include "Controller.h"
#include "DisplayDriver.h"
#include <GradientPalettes.h>
#include "SPIFFS.h"
#include <ezTime.h>

DisplayDriver::DisplayDriver():avg_compution_time(100) {
  last_update = millis();
  //mDisplay_effect = DISPLAY_EFFECT_DAYLIGHT_WHEEL;
  for (int i=0; i<4; ++i) {
    glyphs[i] = new Glyph(LEDS_PER_SEGMENT);
  }
}

DisplayDriver::~DisplayDriver () {
  // destructor
}

uint16_t DisplayDriver::setup(bool isFirstTimeSetup) {
  // EEPROM.begin(512);
  // // check if the serial number is already in the EEPROM. If not, this means
  // // that this siebenuhr has never run before
  // uint8_t serialNumber_lowByte = read_from_EEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE);
  // uint8_t serialNumber_highByte = read_from_EEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE);
  // uint16_t serialNumber = (serialNumber_highByte<<8) | serialNumber_lowByte;
  // // according to the ARDUINO documentation, EEPROM.read() returns 255 if no
  // // has been written to the EEPROM before. therefore the siebenuhrs EEPROM
  // // needs to be initialized with the default values and the serial number

  // if(serialNumber == 65535 || serialNumber == 0) {
  //   File dataFile = SPIFFS.open ("/serial_number.txt", FILE_READ);
  //   String serialNumberString;
  //   if (dataFile) {
  //     serialNumberString = dataFile.readStringUntil('\n');
  //     serialNumber = atoi(serialNumberString.c_str());
  //   }
  //   if(serialNumber == 0) {
  //     serialNumber = 9999;
  //   }
  //   serialNumber_lowByte = serialNumber & 0xff;
  //   serialNumber_highByte = (serialNumber >> 8);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE, serialNumber_lowByte, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE, serialNumber_highByte, 0);

  //   // lets save all the default values to EEPROM
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_H, DEFAULT_COLOR.h, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_S, DEFAULT_COLOR.s, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_V, DEFAULT_COLOR.v, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_BRIGHTNESS, 80, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX, 0, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_COLOR_WHEEL_ANGLE, 171, 0);
  //   deferred_saving_to_EEPROM(EEPROM_ADDRESS_TIMEZONE_HOUR, 1, 0);

  //   save_to_EEPROM();
  // }

  _gCurrentPalette = CRGBPalette16( CRGB::Black);
  //_gTargetPalette = CRGBPalette16( gGradientPalettes[0] );
  for(int i=0;  i<4; i++) {
    if(SIEBENUHR_WIRING == SIEBENUHR_WIRING_PARALELL) {
      glyphs[i]->attach(i, i);
    }
    else if(SIEBENUHR_WIRING == SIEBENUHR_WIRING_SERIAL) {
      glyphs[i]->attach(i);
    }
  }
  if(SIEBENUHR_WIRING == SIEBENUHR_WIRING_SERIAL) {
    _all_leds = new CRGB[SEGMENT_COUNT*LEDS_PER_SEGMENT*GLYPH_COUNT];
    FastLED.addLeds<NEOPIXEL, DATA_PIN_0>(_all_leds, SEGMENT_COUNT*LEDS_PER_SEGMENT*GLYPH_COUNT);
  }

  _solidColor.h = siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_H);
  _solidColor.s = siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_S);
  _solidColor.v = siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_V);
  debug_value("bootup HUE", _solidColor.h);
  // FIXME next if can be deleted, once the initatilizations upstairs works
  if(_solidColor.h == 0 && _solidColor.s == 0 && _solidColor.v == 0) {
    _solidColor = DEFAULT_COLOR;
  }
  _solidColor_previous = _solidColor;
  set_color(_solidColor);
  _brightness = siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_BRIGHTNESS);
  set_brightness(_brightness);
  set_timezone_hour(siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_HOUR));
  mColor_wheel_angle = siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_COLOR_WHEEL_ANGLE);
  mDisplay_effect = siebenuhr::Controller::getInstance()->readFromEEPROM(EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX);

  setPower(1);
}


/*
    update

    Low level method that updates the display and actually does all the magic
*/
/**************************************************************************/
void DisplayDriver::update() {
  events(); // give the ezTime-lib it's processing cycle.....
  unsigned long now = millis();
  if(now - last_update >= DISPLAY_REFRESH_INTERVAL) {
    unsigned long t_1 = millis();
    switch(mOperation_mode) {
      case OPERATION_MODE_CLOCK_HOURS:            update_clock(); break;
      case OPERATION_MODE_CLOCK_MINUTES:          update_clock(); break;
      case OPERATION_MODE_MESSAGE:                update_display_as_notification(); break;
      case OPERATION_MODE_PROGRESS_BAR_BOTTOM:    update_progress_bar(); break;
      case OPERATION_MODE_PROGRESS_BAR_COMPLETE:  update_progress_bar_complete(); break;
      case OPERATION_MODE_PHOTO_SHOOTING:         update_clock(); break;
    }
    last_update = now;
    if(SIEBENUHR_WIRING == SIEBENUHR_WIRING_SERIAL) {
      for (int i=0; i<4; i++) {
        memmove( &_all_leds[glyphs[i]->_glyph_offset], &glyphs[i]->_leds[0], LEDS_PER_SEGMENT*SEGMENT_COUNT * sizeof( CRGB) );
      }
    }
    FastLED.show();
    unsigned long t_2 = millis();
    avg_compution_time.addValue(t_2-t_1);
    compution_time_update_count++;
  }
  if (compution_time_update_count >= DISPLAY_FREQUENCY) {
    compution_time_update_count = 0;
    if (DEBUG_COMPUTION_TIME) {
      Serial.print("DEBUG    ø Compution Time: ");
      Serial.println(avg_compution_time.getAverage());
    }
  }
}

void DisplayDriver::update_display_as_notification() {
  uint32_t now = millis();
  if(now<mDisplay_notification_until || mDisplay_notification_until == 0) {
    for(int i=0; i<4; i++) {
      glyphs[i]->update_blending_to_next_color();
      glyphs[i]->update();
    }
  }
  else {
    disable_notification();
  }
}

void DisplayDriver::update_clock() {
  // let's start with the operations mode (the WHAT to display)
  if (get_operations_mode() == OPERATION_MODE_CLOCK_HOURS && (minute() != mLast_clock_update || check_for_redraw())) {
    mLast_clock_update = minute();
    char message[4] { 0, 0, 0, 0 };
    message[0] = (int) floor(hour()/10) + '0';
    message[1] = hour()%10 + '0';
    message[2] = (int) floor(minute()/10) + '0';
    message[3] = minute()%10 + '0';
    set_next_message(message);
    print_current_time();
    schedule_redraw();
  }
  else if(get_operations_mode() == OPERATION_MODE_CLOCK_MINUTES && (second() != mLast_clock_update || check_for_redraw())) {
    mLast_clock_update = second();
    char message[4] { 0, 0, 0, 0 };
    message[0] = (int) floor(minute()/10) + '0';
    message[1] = minute()%10 + '0';
    message[2] = (int) floor(second()/10) + '0';
    message[3] = second()%10 + '0';
    set_next_message(message);
    print_current_time();
    schedule_redraw();
  }
  else if(get_operations_mode() == OPERATION_MODE_PHOTO_SHOOTING && (second() != mLast_clock_update || check_for_redraw())) {
    mLast_clock_update = second();
    char message[4] { 0, 0, 0, 0 };
    message[0] = (int) floor(mPhotoshooting_hour/10) + '0';
    message[1] = mPhotoshooting_hour%10 + '0';
    message[2] = (int) floor(mPhotoshooting_minute/10) + '0';
    message[3] = mPhotoshooting_minute%10 + '0';
    set_next_message(message);
    print_current_time();
    schedule_redraw();
  }

  // effect section, now we find out HOW to display the content on the display
  if(check_for_redraw()) {
    switch(mDisplay_effect) {
      case DISPLAY_EFFECT_DAYLIGHT_WHEEL:
      {
        int sec_of_day = hour()*3600+minute()*60;
        // +171 = midnight is blue
        int color_wheel_angle = get_color_wheel_angle();
        //sec_of_day = random(86400);
        int hue_next = (int)((((float)sec_of_day/(float)86400)*255)+color_wheel_angle)%255;
        // CHSV color_of_day_next = CHSV( hue_next, 255, 220);
        // int hue = (int)((((float)(sec_of_day-60)/(float)86400)*255)+color_wheel_angle)%255;
        int saturation = 255;
        int value = 220;
        CHSV color_of_day_next = CHSV( hue_next, saturation, value);
        set_next_color(color_of_day_next, check_for_special_blending_period());
        break;
      }
      case DISPLAY_EFFECT_SOLID_COLOR: {
        set_next_color(_solidColor, check_for_special_blending_period());
        break;
      }
      case DISPLAY_EFFECT_RANDOM_COLOR:
      {
        CHSV color_next = CHSV( random(255), 255, 220);
        set_next_color(color_next, check_for_special_blending_period());
        break;
      }
    }
  }
  for(int i=0; i<4; i++) {
    glyphs[i]->update_blending_to_next_color();
    glyphs[i]->update();
  }
}

void DisplayDriver::update_progress_bar() {
  for(int i=0; i<4; i++) {
    glyphs[i]->update_progress_bar();
  }
}

void DisplayDriver::update_progress_bar_complete() {
  for(int i=0; i<4; i++) {
    glyphs[i]->update_progress_bar_complete();
  }
}

/**************************************************************************/
/*
    set_next_message

    Low level routine that sets the next message. A message is something that
    is displayed until told otherwise. In contrast to a message one can also
    show a notification.
    A notification is only displayed for a specified time.
*/
/**************************************************************************/
void DisplayDriver::set_next_message(char message[4], int fade_interval) {
  //Serial.print("Next message: ");
  for(int i=0; i<4; i++) {
    mCurrent_message[i] = message[i];
    glyphs[i]->set_next_char(message[i], fade_interval);
    //Serial.print(message[i]);
  }
  //Serial.println("");
}

/**************************************************************************/
/*
    set_notification

    Display a notifcation.
    A notification is only displayed for a specified time and then, the
    contents shown before the notification will be redisplayed.
    if milliseconds are ommited, it will default to 0 for infinite display
    of this notification
*/
/**************************************************************************/
void DisplayDriver::set_notification(String notificationString, uint32_t milliseconds) {
    char notification[4];
    strncpy(notification, notificationString.c_str(),4);
    set_notification( notification, milliseconds);
}

void DisplayDriver::set_notification(char notification[4], uint32_t milliseconds) {
  if(mOperation_mode != OPERATION_MODE_MESSAGE) {
    mOperation_mode_before_notification = mOperation_mode;
    //mMessage_prior_to_notification = mCurrent_message;
    strncpy(mMessage_prior_to_notification, mCurrent_message, 4);
    mSolid_color_prior_to_notification = _solidColor;
  }
  mOperation_mode = OPERATION_MODE_MESSAGE;
  mNotification_set = 1;
  if (milliseconds>0) {
    mDisplay_notification_until = millis() + milliseconds;
  }
  else {
    mDisplay_notification_until = 0;
  }
  set_color(NOTIFICATION_COLOR);
  set_next_color(NOTIFICATION_COLOR, 1);
  set_next_message(notification, 0);
}

/**************************************************************************/
/*
    disable_notification

    Manually disable the display of a notification (prior to its TTL).
*/
/**************************************************************************/
void DisplayDriver::disable_notification() {
  mDisplay_notification_until = millis();
  if (  mNotification_set ) {
    mOperation_mode = mOperation_mode_before_notification;
    strncpy(mCurrent_message, mMessage_prior_to_notification, 4);
    mNotification_set = 0;
    // set the display color again
    _solidColor = mSolid_color_prior_to_notification;
  }
  // force the clock to update on the next cycle
  schedule_redraw();
}

int DisplayDriver::is_notification_set() {
  return mNotification_set;
}

void DisplayDriver::get_current_message(char* current_message) {
  for(int i=0; i < 4; ++i){
    current_message[i] = mCurrent_message[i];
  }
}

void DisplayDriver::set_next_color(CHSV color, int interval_ms) {
  for(int i=0; i<4; i++) {
    glyphs[i]->set_next_color(color, interval_ms);
  }
}

/**************************************************************************/
/*
    set_color

    Low level routine to set a new color for all 4 glyphs.
*/
/**************************************************************************/

void DisplayDriver::set_color(const struct CHSV& color) {
  _solidColor = color;
  for(int i=0;  i<4; i++) {
    glyphs[i]->set_color(color);
  }
}


void DisplayDriver::save_and_set_new_default_solid_color(const struct CHSV& color) {
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_H, color.h);
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_S, color.s);
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_V, color.v);
  set_color(color);
}

uint8_t DisplayDriver::getPower() {
  return _power;
}
void DisplayDriver::setPower(uint8_t value) {
  if (_power != value) {
    _power = value;
    if (value == 1) {
      _solidColor = _solidColor_previous;
      set_color(_solidColor);
      _powerIsDown = false;
    }
    else {
      _solidColor_previous = _solidColor;
      set_color(CHSV(0, 0, 0));
      _powerIsDown = true;
    }
  }

}

CHSV DisplayDriver::get_solid_color()
{
  return _solidColor;
}

String DisplayDriver::getSolidColorHex()
{
  // FIXME: totally broken since move to HSV-mode - need to be rewritten
  char hex[7] = {0};
  sprintf(hex,"%02X%02X%02X",_solidColor.h,_solidColor.s,_solidColor.v); //convert to an hexadecimal string. Lookup sprintf for what %02X means.
  return hex;
}

void DisplayDriver::set_display_effect(uint8_t value)
{
  // don't wrap around at the ends
  if (value < 0)
  value = 0;
  else if (value >= _display_effects_count)
  value = _display_effects_count - 1;
  mDisplay_effect = value;
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX, mDisplay_effect, 30000);
}

uint8_t DisplayDriver::get_display_effect()
{
  return mDisplay_effect;
}

String DisplayDriver::get_display_effect_json()
{
  String json = "{";
  json += "\"index\":" + String(mDisplay_effect);
  json += ",\"name\":\"" + String(_display_effects[mDisplay_effect]) + "\"";
  json += "}";
  return json;
}

const char* DisplayDriver::get_display_effect_short()
{
  return _display_effects_short[mDisplay_effect];
}

// increase or decrease the current display_effect number, and wrap around at the ends
void DisplayDriver::adjust_and_save_new_display_effect(bool up)
{
  if (up)
    mDisplay_effect++;
  else
    mDisplay_effect--;
  // wrap around at the ends
  if (mDisplay_effect < 0)
  mDisplay_effect = 0;
  else if (mDisplay_effect >= _display_effects_count)
  mDisplay_effect = _display_effects_count - 1;
  Serial.print("current display effect: ");
  Serial.println(mDisplay_effect);
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX, mDisplay_effect, 30000);
}

void DisplayDriver::set_operations_mode(uint8_t mode) {
  mOperation_mode  = mode;
  if (mode == OPERATION_MODE_PHOTO_SHOOTING) {
    set_display_effect(DISPLAY_EFFECT_SOLID_COLOR);
  }
}

uint8_t DisplayDriver::get_operations_mode() {
  return mOperation_mode;
}

/*
Geters and setters for the brightness
*/

uint8_t DisplayDriver::get_brightness() {
  return _brightness;
}

void DisplayDriver::set_brightness(uint8_t value) {
  // don't wrap around at the ends
  //value = round(value * 2.5);
  if (value > 255) value = 255;
  else if (value < 0) value = 0;
  _brightness = value;
  FastLED.setBrightness(_brightness);
}

void DisplayDriver::save_and_set_new_default_brightness(uint8_t value) {
  if (value > 255) value = 255;
  else if (value < 0) value = 0;
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_BRIGHTNESS, value);
  set_brightness(value);
}

/**************************************************************************/
/*
    get_brightness_index

    Get the closest index of the brighness map index based on the current brighntness
*/
/**************************************************************************/

// static const uint8_t _brightnessIndexCount = 7;
// uint8_t _brightnessMap[_brightnessCount] = { 5, 10, 20, 40, 100, 180, 255 };
// int _brightnessIndex = 5;
// uint8_t _brightness = _brightnessMap[_brightnessIndex];


uint8_t DisplayDriver::get_brightness_index() {
  int minDelta = 255;
  int minDeltaIndex = 0;
  for(int i=0; i<_brightnessIndexCount; i++) {
    int delta = abs(_brightness - _brightnessMap[i]);
    if (delta <= minDelta) {
      minDelta = delta;
      minDeltaIndex = i;
    }
  }
  return minDeltaIndex;
}

/**************************************************************************/
/*
    set_brightness_index

    set the new default brightness index. The input is of type int to prevent
    overflow of the input value, as limiting is happening with the method
 */
/**************************************************************************/

void DisplayDriver::set_brightness_index(int value) {
  // don't wrap around at the ends
  //value = round(value * 2.5);

  if (value > _brightnessIndexCount - 1) value = _brightnessIndexCount - 1;
  else if (value < 0) value = 0;
  set_brightness(_brightnessMap[value]);
}

/**************************************************************************/
/*
    save_and_set_new_default_brightness_index

    set and save the new default brightness index. The input is of type int
    to prevent overflow of the input value, as limiting is happening within
    the set_brightness_index method.
 */
/**************************************************************************/

void DisplayDriver::save_and_set_new_default_brightness_index(int value) {
  set_brightness_index(value);
  value = get_brightness_index();
  save_and_set_new_default_brightness(_brightnessMap[value]);
}


/**************************************************************************/
/*
    get_timezone_hour

    return the TZ as hourly delta to UTC
*/
/**************************************************************************/

uint8_t DisplayDriver::get_timezone_hour() {
  return mTimezone;
}

/**************************************************************************/
/*
    set_timezone_hour

    set and save the current timezone as hourly delta to UTC
 */
/**************************************************************************/

void DisplayDriver::set_timezone_hour(int value) {
  mTimezone = value;
  // FIXME EZTIME timezone can no longer be set
  // Timezone myTZ;
  // myTZ.setLocation(F("Europe/Zurich"));
  // Serial.println(myTZ.dateTime());
  //m_pNTP->setTimeZone(mTimezone, 0);
  //set_brightness(_brightnessMap[value]);
}

/**************************************************************************/
/*
    save_and_set_new_default_timezone_hour

    set and save the new default timezone as hourly delta to UTC
 */
/**************************************************************************/

void DisplayDriver::save_and_set_new_default_timezone_hour(int value) {
  set_timezone_hour(value);
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_TIMEZONE_HOUR, value);
}

/**************************************************************************/
/*
    set the timezone for ezTime as default
 */
/**************************************************************************/

void DisplayDriver::set_new_default_timezone(String inTimezone) {
  _ezTimeTimezone.setLocation(inTimezone);
  _ezTimeTimezone.setDefault();
}

uint8_t DisplayDriver::get_color_wheel_angle() {
  return mColor_wheel_angle;
}

void DisplayDriver::set_color_wheel_angle(uint8_t value) {
  // don't wrap around at the ends
  if (value > 255) value = 255;
  else if (value < 0) value = 0;
  mColor_wheel_angle = value;
  //update_clock(true);
  schedule_redraw();
}

void DisplayDriver::save_new_color_wheel_angle(uint8_t value) {
  if (value > 255) value = 255;
  else if (value < 0) value = 0;
  siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_COLOR_WHEEL_ANGLE, value);
  set_color_wheel_angle(value);
}

/**************************************************************************/
/*
    schedule_redraw

    Tell the display to redraw itself on the next update-cycle.
    This routine allows for an async redraw call that will not temper with
    the update cycle but to tell the clock that it will have to redraw itself
    (mostly, because of another message has reached its TTL).
*/
/**************************************************************************/
void DisplayDriver::schedule_redraw() {
  mRedraw_scheduled = true;
}

/**************************************************************************/
/*
    check_for_redraw

    Check if a redraw has been scheduled. If so, return true and unschedule
    the scheduled redraw (as it will have to be executed after callig this
    method)
*/
/**************************************************************************/
bool DisplayDriver::check_for_redraw() {
  if(mRedraw_scheduled) {
    mRedraw_scheduled = false;
    return true;
  }
  return false;
}

/**************************************************************************/
/*
    schedule_special_blending_period_for_next_redraw

    Tell the display to use a non_standard (BLENDIG_PERIOD is default)
    blending period (in milliseconds) on the next redraw. Mostly used for
    showing mNotifications or menu items that are triggered by the knob
    (there the display needs to show an immediate reaction and therefore
    a blending period of 0).

    It also schedules a redraw (!) - until a case is found, where this
    should not be the case.
*/
/**************************************************************************/
void DisplayDriver::schedule_redraw_with_special_blending_period(int blending_period) {
  // if blending_period != default value set it
  if(blending_period != BLENDIG_PERIOD)
    mNext_redraw_blending_period = blending_period;
  if(mNext_redraw_blending_period < 2*DISPLAY_REFRESH_INTERVAL)
    mNext_redraw_blending_period = 2*DISPLAY_REFRESH_INTERVAL;
  mRedraw_scheduled = true;
}


/**************************************************************************/
/*
    check_for_special_blending_period

    check if we hould blend with a special blending period and if yes, return
    and reset it. Otherwise return BLENDIG_PERIOD (the default)
*/
/**************************************************************************/
int DisplayDriver::check_for_special_blending_period() {
  if(mNext_redraw_blending_period != BLENDIG_PERIOD) {
    int blending_period = mNext_redraw_blending_period;
    mNext_redraw_blending_period = BLENDIG_PERIOD;
    return blending_period;
  }
  else {
    return BLENDIG_PERIOD;
  }
}

String DisplayDriver::getStatus()
{
  String json = "{";

  json += "\"power\":" + String(_power) + ",";
  json += "\"brightness\":" + String(_brightness) + ",";
  // json += "\"speed\":" + String(_speed) + ",";

  json += "\"current_display_effect\":{";
  json += "\"index\":" + String(mDisplay_effect);
  json += ",\"name\":\"" + String(_display_effects[mDisplay_effect]) + "\"}";

  // FIXME totally broken since move to HSV color.
  json += ",\"solidColor\":{";
  json += "\"r\":" + String(_solidColor.h);
  json += ",\"g\":" + String(_solidColor.s);
  json += ",\"b\":" + String(_solidColor.v);
  json += "}";

  json += ",\"hueSpeed\":" + String(_gHueSpeed);
  json += ",\"paletteSpeed\":" + String(round(_pChangeSpeed));
  json += ",\"blendingSpeed\":" + String(_pBlendingSpeed);

  json += ",\"display_effects\":[";
  for (uint8_t i = 0; i < _display_effects_count; i++)
  {
    json += "\"" + String(_display_effects[i]) + "\"";
    // json += "\"foo\"";
    if (i < _display_effects_count - 1)
    json += ",";
  }
  json += "]";

  json += "}";
  return json;
}

struct CRGB DisplayDriver::hexToRGB(String hex) {
  int number = (int) strtol( &hex[0], NULL, 16);
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;
  struct CRGB rgbColor = CRGB(r, g, b);
  return rgbColor;
}

String DisplayDriver::RGBToHex(struct CRGB rgbColor) {
  static char hex[7] = {0};
  sprintf(hex,"%02X-%02X-%02X",rgbColor.r,rgbColor.g,rgbColor.b); //convert to an hexadecimal string. Lookup sprintf for what %02X means.
  return String(hex);
}

void DisplayDriver::print_current_time() {
    if (millis()-_nLastTimePrintUpdate > 1000) {
        _nLastTimePrintUpdate = millis();
        Serial.println(formatString("%02d:%02d:%02d %d.%d.%d", hour(), minute(), second(), day(), month(), year()));
    }
}
