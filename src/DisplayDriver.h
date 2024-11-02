#ifndef _7U_DISPLAYDRIVER_H
#define _7U_DISPLAYDRIVER_H

#include "SiebenUhr.h"
#include "Glyph.h"
#include <EEPROM.h>
#include <RunningAverage.h>
#include <ezTime.h>

struct MessageExt {
	char message[4];
	CHSV color[4];
};

class DisplayDriver
{
	static const int DEBUG_COMPUTION_TIME = 0;

public:
	DisplayDriver();
	~DisplayDriver() = default;
	
	void setup(bool isFirstTimeSetup);

	// void setMessage(String messageString, int fade_interval = BLENDIG_PERIOD);
	void setMessage(char message[4], int fade_interval = BLENDIG_PERIOD);
	void getMessage(char *current_message);
	void setMessageExt(const struct MessageExt &msg, int fade_interval = BLENDIG_PERIOD);

	void setSolidColorHSV(CHSV color, int interval_ms = BLENDIG_PERIOD, bool saveToEEPROM = false);
	void setSolidColorRGB(CRGB color, int interval_ms = BLENDIG_PERIOD, bool saveToEEPROM = false);

	void setColorHSV(CHSV color, int interval_ms = BLENDIG_PERIOD);
	struct CHSV getColorHSV();

	void setColorRGB(CRGB color, int interval_ms = BLENDIG_PERIOD);
	struct CRGB getColorRGB();

	void setNotification(String notification, uint32_t milliseconds = 0);
	void setNotification(const char* notification, uint32_t milliseconds = 0);
	void setNotification(char notification[4], uint32_t milliseconds = 0);
	int isNotificationSet();
	void disableNotification();

	void update(bool wifiConnected, bool NTPEnabled);

	uint8_t getPower();
	void setPower(bool power);

	struct CRGB convertHexToRGB(String hex);
	String convertRGBToHex(struct CRGB rgbColor);

	/* set and get the current display effect of the clock. The display effect
		 defines, _how_things are displayed on the clockmode in contrast to the
		 operations mode that defines _what_ is being displayed */
	void setDisplayEffect(uint8_t value);
	uint8_t getDisplayEffect();
	void adjustAndSaveNewDisplayEffect(bool up);
	const char *getDisplayEffectShort();

	void setPalette(uint8_t value);
	uint8_t getCurrentPalette();
	CRGBPalette16 _gCurrentPalette;

	int getBrightness();
	void setBrightness(int value, bool saveToEEPROM = true);

	uint8_t getColorWheelAngle();
	void setColorWheelAngle(uint8_t value, bool saveToEEPROM = true);

	void setNewDefaultTimezone(String inTimezone);

	void setOperationMode(uint8_t mode);
	uint8_t getOperationMode();

	void scheduleRedraw();
	void scheduleRedraw(int blending_period);

	uint8_t _photoShootingHour = 19;
	uint8_t _photoShootingMinute = 37;

private:
	Timezone _ezTimeTimezone;
	Glyph *_glyphs[4];
	CRGB *_leds;
	
	bool _bNotificationSet = false;
	uint8_t _nLastUpdate;		 // when was the clock updated the last time
	uint8_t _nLastClockUpdate;		 // when was the clock updated the last time

	uint8_t _bPower;
	struct CHSV _solidColorBeforeShutdown;

	uint8_t _nBrightness = 128;
	uint8_t _nColorWheelAngle = 0; // Index number of which color angle is current
	int8_t _nTimezone = 1;						// is a notification currently set?

	const char *_display_effects[3] = {
			"Color Wheel", "Solid Color", "Random Color"};
	const char *_display_effects_short[3] = {
			"24h", "conS", "RAnd"};
	uint8_t _display_effects_count = 3;

	const char *_operation_mode[5] = {
			"Clock (hours)", "Clock (minutes)", "Message", "Progress Bar Bottom", "Progress Bar Complete"};
	uint8_t _operation_mode_count = 5;

	struct CRGB _colorCurrent; // this is the current color of the clock
	struct CHSV _solidColor; // this is thecolor, if the clock is set to a solid color. It's NOT the current color
	uint8_t _nHue = 0; // rotating "base color" used by many of the display effects
	uint16_t _nHueSpeed = 20;
	uint16_t _nHueSpeedPrev = 0; // will store last time LED was updated
	uint16_t _nChangeSpeed = 3000;
	uint16_t _pChangeSpeedPrev = 0; // will store last time LED was updated
	uint16_t _nBlendingSpeed = 10000;
	uint16_t _autoPlayTimeout = 0;
	uint8_t _nOperationMode = OPERATION_MODE_CLOCK_HOURS;
	uint8_t _nDisplayEffect = DISPLAY_EFFECT_DAYLIGHT_WHEEL;
	char _currentMessage[GLYPH_COUNT];

	uint32_t _nDisplayNotificationUntil = 0;
	uint16_t _nOperationModeBeforeNotification = 0;
	char _messageBeforeNotification[GLYPH_COUNT];
	struct CHSV _solidColorBeforeNotification;

	bool _bRedrawScheduled = false;						// set to true, if the clock requires a redraw
	int _nNextRedrawBlendingPeriod = BLENDIG_PERIOD; 	// for the next redraw, use this blending period

	// analyse the current compution time for the update cycle (for debugging)
	RunningAverage _avgComputionTime;
	int _nComputionTimeUpdateCount = 0;

	// update the display for the different clock-modes
	void updateClock();
	void updateDisplayAsNotification();
	void update_progress_bar();
	void update_progress_bar_complete();

	bool checkForRedraw();
	int getSpecialBlendingPeriod();

	int _nLastTimePrintUpdate = 0;
	void printCurrentTime();
};

#endif
