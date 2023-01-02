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

DisplayDriver::DisplayDriver() : avg_compution_time(100)
{
	last_update = millis();
	// mDisplay_effect = DISPLAY_EFFECT_DAYLIGHT_WHEEL;
	for (int i = 0; i < 4; ++i)
	{
		glyphs[i] = new Glyph(LEDS_PER_SEGMENT);
	}
}

void DisplayDriver::setup(bool isFirstTimeSetup)
{
	setPower(0);

	siebenuhr::Controller *_inst = siebenuhr::Controller::getInstance(); // just for convinience

	_gCurrentPalette = CRGBPalette16(CRGB::Black);
	for (int i = 0; i < 4; i++) {
		if (SIEBENUHR_WIRING == SIEBENUHR_WIRING_PARALELL) {
			glyphs[i]->attach(i, i);
		}
		else if (SIEBENUHR_WIRING == SIEBENUHR_WIRING_SERIAL) {
			glyphs[i]->attach(i);
		}
	}

	if (SIEBENUHR_WIRING == SIEBENUHR_WIRING_SERIAL)
	{
		_all_leds = new CRGB[SEGMENT_COUNT * LEDS_PER_SEGMENT * GLYPH_COUNT];
		FastLED.addLeds<NEOPIXEL, DATA_PIN_0>(_all_leds, SEGMENT_COUNT * LEDS_PER_SEGMENT * GLYPH_COUNT);
	}

	_solidColor.h = _inst->readFromEEPROM(EEPROM_ADDRESS_H);
	_solidColor.s = _inst->readFromEEPROM(EEPROM_ADDRESS_S);
	_solidColor.v = _inst->readFromEEPROM(EEPROM_ADDRESS_V);

	_inst->debugValue("Bootup HUE", _solidColor.h);
	
	// FIXME next if can be deleted, once the initatilizations upstairs works
	if (_solidColor.h == 0 && _solidColor.s == 0 && _solidColor.v == 0)
	{
		_solidColor = DEFAULT_COLOR;
	}

	_solidColor_previous = _solidColor;
	setColor(_solidColor);
	_brightness = _inst->readFromEEPROM(EEPROM_ADDRESS_BRIGHTNESS);
	setBrightness(_brightness);
	setTimezoneHour(_inst->readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_HOUR));
	mColor_wheel_angle = _inst->readFromEEPROM(EEPROM_ADDRESS_COLOR_WHEEL_ANGLE);
	mDisplay_effect = _inst->readFromEEPROM(EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX);

	setPower(1);
}

/*
		update

		Low level method that updates the display and actually does all the magic
*/
/**************************************************************************/
void DisplayDriver::update()
{
	events(); // give the ezTime-lib it's processing cycle.....
	unsigned long now = millis();
	if (now - last_update >= DISPLAY_REFRESH_INTERVAL)
	{
		unsigned long t_1 = millis();
		switch (mOperation_mode)
		{
		case OPERATION_MODE_CLOCK_HOURS:
			update_clock();
			break;
		case OPERATION_MODE_CLOCK_MINUTES:
			update_clock();
			break;
		case OPERATION_MODE_MESSAGE:
			update_display_as_notification();
			break;
		case OPERATION_MODE_PROGRESS_BAR_BOTTOM:
			update_progress_bar();
			break;
		case OPERATION_MODE_PROGRESS_BAR_COMPLETE:
			update_progress_bar_complete();
			break;
		case OPERATION_MODE_PHOTO_SHOOTING:
			update_clock();
			break;
		}

		last_update = now;
		if (SIEBENUHR_WIRING == SIEBENUHR_WIRING_SERIAL)
		{
			for (int i = 0; i < 4; i++)
			{
				memmove(&_all_leds[glyphs[i]->_glyph_offset], &glyphs[i]->_leds[0], LEDS_PER_SEGMENT * SEGMENT_COUNT * sizeof(CRGB));
			}
		}

		FastLED.show();
		unsigned long t_2 = millis();
		avg_compution_time.addValue(t_2 - t_1);
		compution_time_update_count++;
	}

	if (compution_time_update_count >= DISPLAY_FREQUENCY)
	{
		compution_time_update_count = 0;
		if (DEBUG_COMPUTION_TIME)
		{
			Serial.print("DEBUG    ø Compution Time: ");
			Serial.println(avg_compution_time.getAverage());
		}
	}
}

void DisplayDriver::update_display_as_notification()
{
	uint32_t now = millis();
	if (now < mDisplay_notification_until || mDisplay_notification_until == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			glyphs[i]->update_blending_to_next_color();
			glyphs[i]->update();
		}
	}
	else
	{
		disableNotification();
	}
}

void DisplayDriver::update_clock()
{
	if (get_operations_mode() == OPERATION_MODE_CLOCK_HOURS && (minute() != mLast_clock_update || checkForRedraw()))
	{
		mLast_clock_update = minute();
		char message[4]{0, 0, 0, 0};
		message[0] = (int)floor(hour() / 10) + '0';
		message[1] = hour() % 10 + '0';
		message[2] = (int)floor(minute() / 10) + '0';
		message[3] = minute() % 10 + '0';
		setMessage(message);
		printCurrentTime();
		scheduleRedraw();
	}
	else if (get_operations_mode() == OPERATION_MODE_CLOCK_MINUTES && (second() != mLast_clock_update || checkForRedraw()))
	{
		mLast_clock_update = second();
		char message[4]{0, 0, 0, 0};
		message[0] = (int)floor(minute() / 10) + '0';
		message[1] = minute() % 10 + '0';
		message[2] = (int)floor(second() / 10) + '0';
		message[3] = second() % 10 + '0';
		setMessage(message);
		printCurrentTime();
		scheduleRedraw();
	}
	else if (get_operations_mode() == OPERATION_MODE_PHOTO_SHOOTING && (second() != mLast_clock_update || checkForRedraw()))
	{
		mLast_clock_update = second();
		char message[4]{0, 0, 0, 0};
		message[0] = (int)floor(mPhotoshooting_hour / 10) + '0';
		message[1] = mPhotoshooting_hour % 10 + '0';
		message[2] = (int)floor(mPhotoshooting_minute / 10) + '0';
		message[3] = mPhotoshooting_minute % 10 + '0';
		setMessage(message);
		printCurrentTime();
		scheduleRedraw();
	}

	// effect section, now we find out HOW to display the content on the display
	if (checkForRedraw()) {
		switch (mDisplay_effect) {
			case DISPLAY_EFFECT_DAYLIGHT_WHEEL:
			{
				int sec_of_day = hour() * 3600 + minute() * 60;
				int color_wheel_angle = get_color_wheel_angle();
				int hue_next = (int)((((float)sec_of_day / (float)86400) * 255) + color_wheel_angle) % 255;
				int saturation = 255;
				int value = 220;
				CHSV color_of_day_next = CHSV(hue_next, saturation, value);
				setNextColor(color_of_day_next, getSpecialBlendingPeriod());
				break;
			}

			case DISPLAY_EFFECT_SOLID_COLOR:
			{
				setNextColor(_solidColor, getSpecialBlendingPeriod());
				break;
			}

			case DISPLAY_EFFECT_RANDOM_COLOR:
			{
				CHSV color_next = CHSV(random(255), 255, 220);
				setNextColor(color_next, getSpecialBlendingPeriod());
				break;
			}
		}
	}
	for (int i = 0; i < 4; i++)
	{
		glyphs[i]->update_blending_to_next_color();
		glyphs[i]->update();
	}
}

void DisplayDriver::update_progress_bar()
{
	for (int i = 0; i < 4; i++)
	{
		glyphs[i]->update_progress_bar();
	}
}

void DisplayDriver::update_progress_bar_complete()
{
	for (int i = 0; i < 4; i++)
	{
		glyphs[i]->update_progress_bar_complete();
	}
}

/**************************************************************************/
/*
		A message is something that is displayed until told otherwise. 
		In contrast to a message one can also show a notification.
		A notification is only displayed for a specified time.
*/
/**************************************************************************/

void DisplayDriver::setMessage(char message[4], int fade_interval)
{
	for (int i = 0; i < 4; i++)
	{
		mCurrent_message[i] = message[i];
		glyphs[i]->set_next_char(message[i], fade_interval);
	}
}

void DisplayDriver::getMessage(char *current_message)
{
	for (int i = 0; i < 4; ++i)
	{
		current_message[i] = mCurrent_message[i];
	}
}

/**************************************************************************/
/*		
		A notification is only displayed for a specified time and then, the
		contents shown before the notification will be redisplayed.
		if milliseconds are ommited, it will default to 0 for infinite display
		of this notification
*/
/**************************************************************************/

void DisplayDriver::setNotification(String notificationString, uint32_t milliseconds)
{
	char notification[4];
	strncpy(notification, notificationString.c_str(), 4);
	setNotification(notification, milliseconds);
}

void DisplayDriver::setNotification(const char* notificationString, uint32_t milliseconds)
{
	char notification[4];
	strncpy(notification, notificationString, 4);
	setNotification(notification, milliseconds);
}

void DisplayDriver::setNotification(char notification[4], uint32_t milliseconds)
{
	if (mOperation_mode != OPERATION_MODE_MESSAGE)
	{
		mOperation_mode_before_notification = mOperation_mode;
		// mMessage_prior_to_notification = mCurrent_message;
		strncpy(mMessage_prior_to_notification, mCurrent_message, 4);
		mSolid_color_prior_to_notification = _solidColor;
	}

	mOperation_mode = OPERATION_MODE_MESSAGE;
	_bNotificationSet = true;
	if (milliseconds > 0)
	{
		mDisplay_notification_until = millis() + milliseconds;
	}
	else
	{
		mDisplay_notification_until = 0;
	}

	setColor(NOTIFICATION_COLOR);
	setNextColor(NOTIFICATION_COLOR, 1);
	setMessage(notification, 0);
}

void DisplayDriver::disableNotification()
{
	// Manually disable the display of a notification (prior to its TTL).
	mDisplay_notification_until = millis();
	if (_bNotificationSet)
	{
		mOperation_mode = mOperation_mode_before_notification;
		strncpy(mCurrent_message, mMessage_prior_to_notification, 4);
		_bNotificationSet = false;
		_solidColor = mSolid_color_prior_to_notification;
	}

	// force the clock to update on the next cycle
	scheduleRedraw();
}

int DisplayDriver::isNotificationSet()
{
	return _bNotificationSet;
}

/**************************************************************************/
/*
		set_color

		Low level routine to set a new color for all 4 glyphs.
*/
/**************************************************************************/

void DisplayDriver::setColor(const struct CHSV &color, bool saveToEEPROM)
{
	_solidColor = color;
	for (int i = 0; i < 4; i++) {
		glyphs[i]->set_color(color);
	}

	if (saveToEEPROM) {
		siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_H, color.h);
		siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_S, color.s);
		siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_V, color.v);
	}
}

void DisplayDriver::setNextColor(CHSV color, int interval_ms)
{
	for (int i = 0; i < 4; i++)
	{
		glyphs[i]->set_next_color(color, interval_ms);
	}
}

CHSV DisplayDriver::getColor()
{
	return _solidColor;
}

uint8_t DisplayDriver::getPower()
{
	return _power;
}

void DisplayDriver::setPower(uint8_t value)
{
	if (_power != value)
	{
		_power = value;
		if (value == 1)
		{
			_solidColor = _solidColor_previous;
			setColor(_solidColor);
			_powerIsDown = false;
		}
		else
		{
			_solidColor_previous = _solidColor;
			setColor(CHSV(0, 0, 0));
			_powerIsDown = true;
		}
	}
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

const char *DisplayDriver::get_display_effect_short()
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

void DisplayDriver::set_operations_mode(uint8_t mode)
{
	mOperation_mode = mode;
	if (mode == OPERATION_MODE_PHOTO_SHOOTING)
	{
		set_display_effect(DISPLAY_EFFECT_SOLID_COLOR);
	}
}

uint8_t DisplayDriver::get_operations_mode()
{
	return mOperation_mode;
}

/*
Geters and setters for the brightness
*/

uint8_t DisplayDriver::getBrightness()
{
	return _brightness;
}

void DisplayDriver::setBrightness(uint8_t value, bool saveToEEPROM)
{
	if (value > 255)
		value = 255;
	else if (value < 0)
		value = 0;

	if (saveToEEPROM)
	{
		siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_BRIGHTNESS, value);
	}

	_brightness = value;
	FastLED.setBrightness(_brightness);
}

/*
Geters and setters for the brightness, based on brightness map
*/

uint8_t DisplayDriver::getBrightnessIndex()
{
	int minDelta = 255;
	int minDeltaIndex = 0;
	for (int i = 0; i < _brightnessIndexCount; i++)
	{
		int delta = abs(_brightness - _brightnessMap[i]);
		if (delta <= minDelta)
		{
			minDelta = delta;
			minDeltaIndex = i;
		}
	}
	return minDeltaIndex;
}

void DisplayDriver::setBrightnessIndex(int value, bool saveToEEPROM)
{
	if (value > _brightnessIndexCount - 1)
	{
		value = _brightnessIndexCount - 1;
	}
	else if (value < 0)
	{
		value = 0;
	}
	setBrightness(_brightnessMap[value], saveToEEPROM);
}

/*
Geters and setters for the timezone hour delta
*/

uint8_t DisplayDriver::getTimezoneHour()
{
	return mTimezone;
}

void DisplayDriver::setTimezoneHour(int value, bool saveToEEPROM)
{
	mTimezone = value;
	// FIXME EZTIME timezone can no longer be set
	// Timezone myTZ;
	// myTZ.setLocation(F("Europe/Zurich"));
	// Serial.println(myTZ.dateTime());
	// m_pNTP->setTimeZone(mTimezone, 0);
	// set_brightness(_brightnessMap[value]);

	if (saveToEEPROM) {
		siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_TIMEZONE_HOUR, value);
	}
}

/**************************************************************************/
/*
		set the timezone for ezTime as default
 */
/**************************************************************************/

void DisplayDriver::set_new_default_timezone(String inTimezone)
{
	_ezTimeTimezone.setLocation(inTimezone);
	_ezTimeTimezone.setDefault();
}

uint8_t DisplayDriver::get_color_wheel_angle()
{
	return mColor_wheel_angle;
}

void DisplayDriver::set_color_wheel_angle(uint8_t value)
{
	// don't wrap around at the ends
	if (value > 255)
		value = 255;
	else if (value < 0)
		value = 0;
	mColor_wheel_angle = value;
	// update_clock(true);
	scheduleRedraw();
}

void DisplayDriver::save_new_color_wheel_angle(uint8_t value)
{
	if (value > 255)
		value = 255;
	else if (value < 0)
		value = 0;
	siebenuhr::Controller::getInstance()->writeToEEPROM(EEPROM_ADDRESS_COLOR_WHEEL_ANGLE, value);
	set_color_wheel_angle(value);
}

/**************************************************************************/
/*
		Check if a redraw has been scheduled. If so, return true and unschedule
		the scheduled redraw (as it will have to be executed after callig this
		method)
*/
/**************************************************************************/
bool DisplayDriver::checkForRedraw()
{
	if (_bRedrawScheduled) {
		_bRedrawScheduled = false;
		return true;
	}
	return false;
}

/**************************************************************************/
/*
		Tell the display to redraw itself on the next update-cycle.
		This routine allows for an async redraw call that will not temper with
		the update cycle but to tell the clock that it will have to redraw itself
		(mostly, because of another message has reached its TTL).
*/
/**************************************************************************/
void DisplayDriver::scheduleRedraw()
{
	_bRedrawScheduled = true;
}

/**************************************************************************/
/*
		Tell the display to use a non_standard (BLENDIG_PERIOD is default)
		blending period (in milliseconds) on the next redraw. Mostly used for
		showing mNotifications or menu items that are triggered by the knob
		(there the display needs to show an immediate reaction and therefore
		a blending period of 0).

		It also schedules a redraw (!) - until a case is found, where this
		should not be the case.
*/
/**************************************************************************/
void DisplayDriver::scheduleRedraw(int blending_period)
{
	// if blending_period != default value set it
	if (blending_period != BLENDIG_PERIOD)
		_nNextRedrawBlendingPeriod = blending_period;
	if (_nNextRedrawBlendingPeriod < 2 * DISPLAY_REFRESH_INTERVAL)
		_nNextRedrawBlendingPeriod = 2 * DISPLAY_REFRESH_INTERVAL;

	scheduleRedraw();
}

/**************************************************************************/
/*
		check if we hould blend with a special blending period and if yes, return
		and reset it. Otherwise return BLENDIG_PERIOD (the default)
*/
/**************************************************************************/
int DisplayDriver::getSpecialBlendingPeriod()
{
	if (_nNextRedrawBlendingPeriod != BLENDIG_PERIOD) {
		int blendingPeriod = _nNextRedrawBlendingPeriod;
		_nNextRedrawBlendingPeriod = BLENDIG_PERIOD;
		return blendingPeriod;
	}

	return BLENDIG_PERIOD;
}

String DisplayDriver::getStatus()
{
	String json = "{";

	json += "\"power\":" + String(_power) + ",";
	json += "\"brightness\":" + String(_brightness) + ",";

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
		if (i < _display_effects_count - 1)
			json += ",";
	}
	json += "]";

	json += "}";
	return json;
}

struct CRGB DisplayDriver::convertHexToRGB(String hex)
{
	int number = (int)strtol(&hex[0], NULL, 16);
	int r = number >> 16;
	int g = number >> 8 & 0xFF;
	int b = number & 0xFF;
	struct CRGB rgbColor = CRGB(r, g, b);
	return rgbColor;
}

String DisplayDriver::convertRGBToHex(struct CRGB rgbColor)
{
	return formatString("%02X-%02X-%02X", rgbColor.r, rgbColor.g, rgbColor.b);
}

void DisplayDriver::printCurrentTime()
{
	if (millis() - _nLastTimePrintUpdate > 1000) {
		_nLastTimePrintUpdate = millis();
		siebenuhr::Controller::getInstance()->debugMessage(formatString("%02d:%02d:%02d %d.%d.%d", hour(), minute(), second(), day(), month(), year()));
	}
}
