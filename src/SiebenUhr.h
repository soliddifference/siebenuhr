#ifndef SEVENCLOCK_H
#define SEVENCLOCK_H

#define FASTLED_INTERNAL
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>
#include <SPIFFS.h>
#include <EEPROM.h>

#define QUINLEDBOARD 	// PCB Version
#define SIEBENUHR_MINI	// disable to build for the standart siebenuhr

static const int SIEBENURH_FIRMWARE_VERSION = 1; // just increase if EEPROM structure changed

static const int DEFAULT_SETUP_HOUR = 7;
static const int DEFAULT_SETUP_MINUTE = 42;

// number of segments per glyph (hahaha, must be seven, as it's called siebenuhr, stupid!)
static const int SEGMENT_COUNT = 7;
// leds per single segment
#ifdef SIEBENUHR_MINI
	static const int LEDS_PER_SEGMENT = 4;
#else
	static const int LEDS_PER_SEGMENT = 11;
#endif
// number of glyphs on this clock
static const int GLYPH_COUNT = 4;
// // what's the refresh rate per second?
static const int DISPLAY_FREQUENCY = 50;
// the interval between refreshs (in ms) given a specific frequency
static const int DISPLAY_REFRESH_INTERVAL = floor(1000 / DISPLAY_FREQUENCY);

static const int OPERATION_MODE_CLOCK_HOURS = 0;
static const int OPERATION_MODE_CLOCK_MINUTES = 1;
static const int OPERATION_MODE_MESSAGE = 2;
static const int OPERATION_MODE_PROGRESS_BAR_BOTTOM = 3;
static const int OPERATION_MODE_PROGRESS_BAR_COMPLETE = 4;
static const int OPERATION_MODE_PHOTO_SHOOTING = 5;
static const int OPERATION_MODE_DEMO = 6;
static const int OPERATION_MODE_TIME_SETUP = 7;

static const int DISPLAY_EFFECT_DAYLIGHT_WHEEL = 0;
static const int DISPLAY_EFFECT_SOLID_COLOR = 1;
static const int DISPLAY_EFFECT_RANDOM_COLOR = 2;

static const int EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE = 0;
static const int EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE = 1;
static const int EEPROM_ADDRESS_VERSION = 2;
static const int EEPROM_ADDRESS_H = 3;
static const int EEPROM_ADDRESS_S = 4;
static const int EEPROM_ADDRESS_V = 5;
static const int EEPROM_ADDRESS_BRIGHTNESS = 6;
static const int EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX = 7;
static const int EEPROM_ADDRESS_COLOR_WHEEL_ANGLE = 8;
static const int EEPROM_ADDRESS_TIMEZONE_ID = 9;
static const int EEPROM_ADDRESS_WIFI_ENABLED = 10;
// strings starting here....
static const int EEPROM_ADDRESS_WIFI_SSID = 20;
static const int EEPROM_ADDRESS_WIFI_PSWD = 60;
static const int EEPROM_ADDRESS_HA_MQTT_USERNAME = 100;
static const int EEPROM_ADDRESS_HA_MQTT_PASSWORD = 140;
static const int EEPROM_ADDRESS_HA_MQTT_IP = 200;

static const int EEPROM_ADDRESS_MAX_LENGTH = 40;

static const uint8_t EEPROM_ADDRESS_COUNT = 11;
static const uint8_t EEPROM_ADDRESSES[EEPROM_ADDRESS_COUNT] = { // exclude 'strings' as they need to be handled differently
	EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE,
	EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE,
	EEPROM_ADDRESS_VERSION,
	EEPROM_ADDRESS_H,
	EEPROM_ADDRESS_S,
	EEPROM_ADDRESS_V,
	EEPROM_ADDRESS_BRIGHTNESS,
	EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX,
	EEPROM_ADDRESS_COLOR_WHEEL_ANGLE,
	EEPROM_ADDRESS_TIMEZONE_ID,
	EEPROM_ADDRESS_WIFI_ENABLED
};

static const int BLENDIG_PERIOD = 500;

static const int RESET_BUTTON = 4;
static const int FUNCTION_LED = 2;

static const int SIEBENUHR_WIRING_PARALELL = 1;
static const int SIEBENUHR_WIRING_SERIAL = 2;
static const int SIEBENUHR_WIRING = SIEBENUHR_WIRING_SERIAL;
// static const int SIEBENUHR_WIRING = SIEBENUHR_WIRING_PARALELL;

// data pin consts....

#ifdef QUINLEDBOARD
static const int DATA_PIN_0 = 16; 	// LED1 - LED SERIAL
static const int DATA_PIN_1 = 15; 	// Q1 - KNOB-ENC-A
static const int DATA_PIN_2 = 12; 	// Q2 - KNOB-ENC-B
static const int DATA_PIN_3 = 2; 	// Q3 - KNOB-BUTTON
#else
static const int DATA_PIN_0 = 19; // LED SERIAL
static const int DATA_PIN_1 = 21; // KNOB-ENC-A
static const int DATA_PIN_2 = 22; // KNOB-ENC-B
static const int DATA_PIN_3 = 23; // KNOB-BUTTON
#endif

static const int ROTARY_ENCODER_A_PIN = DATA_PIN_1;
static const int ROTARY_ENCODER_B_PIN = DATA_PIN_2;
static const int ROTARY_ENCODER_BUTTON_PIN = DATA_PIN_3;

static const int BUTTON_LATENCY = 300; // latency in ms where consecutive button clicks will get ignored

static const CHSV NOTIFICATION_COLOR = CHSV(171, 255, 220);
static const CHSV DEFAULT_COLOR = CHSV(171, 255, 220);

void debugColor(CRGB color, int scale = 1);

inline int max(int a, int b)
{
	if (a > b) {
		return a;
	}
	return b;
}

inline int min(int a, int b)
{
	if (a < b) {
		return a;
	}
	return b;
}

inline String formatString(const char *format, ...)
{
	static char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	return String(buffer);
}

inline String HSVtoString(CHSV color)
{
	return formatString("%02X%02X%02X", color.h, color.s, color.v);
}

inline void debugColor(CRGB color, int scale)
{
	const int SPACE = 20;
	if (scale == 1 && (color.r >= SPACE or color.g >= SPACE or color.g >= SPACE))
	{
		int sr = floor(color.r / SPACE);
		int sg = floor(color.g / SPACE);
		int sb = floor(color.b / SPACE);
		scale = max(sr, max(sg, sb));
	}

	// red
	Serial.print("R [");
	color.r < 100 ? Serial.print(" ") : false;
	color.r < 10 ? Serial.print(" ") : false;
	Serial.print(color.r);
	Serial.print("]: ");
	int i = 0;
	int j = 0;
	for (; i < color.r; i++)
	{
		Serial.print("+");
		i = scale > 1 ? i + scale : i;
		j++;
	}
	for (; j <= SPACE; j++)
	{
		Serial.print(" ");
	}

	// green
	Serial.print("G [");
	color.g < 100 ? Serial.print(" ") : false;
	color.g < 10 ? Serial.print(" ") : false;
	Serial.print(color.g);
	Serial.print("]: ");
	i = 0;
	j = 0;
	for (; i < color.g; i++)
	{
		Serial.print("+");
		i = scale > 1 ? i + scale : i;
		j++;
	}
	for (; j <= SPACE; j++)
	{
		Serial.print(" ");
	}

	// blue
	Serial.print("B [");
	color.b < 100 ? Serial.print(" ") : false;
	color.b < 10 ? Serial.print(" ") : false;
	Serial.print(color.b);
	Serial.print("]: ");
	i = 0;
	j = 0;
	for (; i < color.b; i++)
	{
		Serial.print("+");
		i = scale > 1 ? i + scale : i;
		j++;
	}

	for (; j <= SPACE; j++)
	{
		Serial.print(" ");
	}

	Serial.println();
}

inline String reformatNotification(String str)
{
    for (int i = 0; i < str.length(); i++) {
        switch (str[i]) {
            case 'a':
                str[i] = 'A';
                break;
            case 'B':
                str[i] = 'b';
                break;
            case 'D':
                str[i] = 'd';
                break;
            case 'e':
                str[i] = 'E';
                break;
            case 'g':
                str[i] = 'G';
                break;
			case 'H':
                str[i] = 'h';
                break;	
			case 'i':
                str[i] = 'I';
                break;	
			case 'j':
                str[i] = 'J';
                break;	
			case 'l':
                str[i] = 'L';
                break;	
			case 'M':
                str[i] = 'm';
                break;	
			case 'O':
                str[i] = 'o';
                break;	
			case 'Q':
                str[i] = 'q';
                break;
			case 'R':
                str[i] = 'r';
                break;					
			case 'U':
                str[i] = 'u';
                break;						
            default:
                // Do nothing if the character doesn't match any case
                break;
        }
    }
    return str;
}


#endif
