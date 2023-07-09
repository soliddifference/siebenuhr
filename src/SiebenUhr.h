#ifndef SEVENCLOCK_H
#define SEVENCLOCK_H

#define FASTLED_INTERNAL
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>
#include <SPIFFS.h>
#include <EEPROM.h>

#define QUINLEDBOARD

static const int DEFAULT_SETUP_HOUR = 7;
static const int DEFAULT_SETUP_MINUTE = 42;

// number of segments per glyph (hahaha, must be seven, as it's called siebenuhr, stupid!)
static const int SEGMENT_COUNT = 7;
// leds per single segment
// static const int LEDS_PER_SEGMENT = 6;
static const int LEDS_PER_SEGMENT = 11;
// number of glyphs on this clock
static const int GLYPH_COUNT = 4;
// static const int GLYPH_COUNT = 1;
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
static const int EEPROM_ADDRESS_H = 2;
static const int EEPROM_ADDRESS_S = 3;
static const int EEPROM_ADDRESS_V = 4;
static const int EEPROM_ADDRESS_BRIGHTNESS = 5;
static const int EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX = 6;
static const int EEPROM_ADDRESS_COLOR_WHEEL_ANGLE = 7;
static const int EEPROM_ADDRESS_TIMEZONE_ID = 8;
static const int EEPROM_ADDRESS_WIFI_ENABLED = 9;
static const int EEPROM_ADDRESS_WIFI_SSID = 20;
static const int EEPROM_ADDRESS_WIFI_PSWD = 60;
static const int EEPROM_ADDRESS_HA_MQTT_USERNAME = 100;
static const int EEPROM_ADDRESS_HA_MQTT_PASSWORD = 140;
static const int EEPROM_ADDRESS_HA_MQTT_IP = 200;


static const int EEPROM_ADDRESS_MAX_LENGTH = 40;

static const uint8_t EEPROM_ADDRESS_COUNT = 12;
static const uint8_t EEPROM_ADDRESSES[EEPROM_ADDRESS_COUNT] = {
	EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE,
	EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE,
	EEPROM_ADDRESS_H,
	EEPROM_ADDRESS_S,
	EEPROM_ADDRESS_V,
	EEPROM_ADDRESS_BRIGHTNESS,
	EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX,
	EEPROM_ADDRESS_COLOR_WHEEL_ANGLE,
	EEPROM_ADDRESS_TIMEZONE_ID,
	EEPROM_ADDRESS_WIFI_ENABLED,
	EEPROM_ADDRESS_WIFI_SSID, 
	EEPROM_ADDRESS_WIFI_PSWD};

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
#elif
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
	if (a > b)
	{
		return a;
	}
	return b;
}

inline int min(int a, int b)
{
	if (a < b)
	{
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

inline void debug_listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
	Serial.printf("Listing directory: %s\r\n", dirname);

	File root = fs.open(dirname);
	if (!root)
	{
		Serial.println("- failed to open directory");
		return;
	}
	if (!root.isDirectory())
	{
		Serial.println(" - not a directory");
		return;
	}

	File file = root.openNextFile();
	while (file)
	{
		if (file.isDirectory())
		{
			Serial.print("  DIR : ");
			Serial.println(file.name());
			if (levels)
			{
				debug_listDir(fs, file.name(), levels - 1);
			}
		}
		else
		{
			Serial.print("  FILE: ");
			Serial.print(file.name());
			Serial.print("\t\tSIZE: ");
			Serial.println(file.size());
		}
		file = root.openNextFile();
	}
}

inline String EEPROMReadString(char eeprom_address, int maxLength)
{
	char data[maxLength]; // Max 100 Bytes
	int len = 0;
	unsigned char k;
	k = EEPROM.read(eeprom_address);
	while (k != '\0' && len < maxLength) // Read until null character
	{
		k = EEPROM.read(eeprom_address + len);
		data[len] = k;
		len++;
	}
	data[len] = '\0';
	return String(data);
}

inline void EEPROMWriteString(char eeprom_address, String data, int maxLength)
{
	if (data.equals(EEPROMReadString(eeprom_address, maxLength))) {
		return;
	}
	int _size = data.length();
	int i;
	for (i = 0; i < _size || i < maxLength; i++) {
		EEPROM.write(eeprom_address + i, data[i]);
		delay(10);
	}
	EEPROM.write(eeprom_address + _size, '\0'); // Add termination null character for String Data
	EEPROM.commit();
}

#endif
