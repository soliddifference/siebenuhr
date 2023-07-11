/*  Controller.cpp
	v0.1 ys 20190422 refactoring

	Class to manage all in- and outout of siebenuhr
*/

#include <Arduino.h>
#include "APController.h"
#include "Controller.h"
#include "TimeZones.h"

#include <WiFi.h>

#include "HomeAssistant.h"

using namespace siebenuhr;

Controller* Controller::_pInstance = nullptr;
UIKnob* Controller::_pKnobEncoder = nullptr;

const ControllerMenu_t Controller::_sMenu[Controller::_nMenuMaxEntries] = {
		{CONTROLLER_MENU::CLOCK, "Display Clock", "CLCK"},
		{CONTROLLER_MENU::BRIGHTNESS, "Brightness", "Brit"},
		{CONTROLLER_MENU::HUE, "Hue", "COLr"},
		{CONTROLLER_MENU::SATURATION, "Saturation", "SAtU"},
		{CONTROLLER_MENU::EFFECT, "Effect", "EFCT"},
		{CONTROLLER_MENU::TIMEZONE, "Timezone", "ZOnE"},
		{CONTROLLER_MENU::SET_HOUR, "Set Hour", "HOUr"},
		{CONTROLLER_MENU::SET_MINUTE, "Set Minute", "MinU"}};

Controller* Controller::getInstance(){
	if (Controller::_pInstance == nullptr) {
		Controller::_pInstance = new Controller();
	}
	return Controller::_pInstance;
}

Controller::Controller() {
	_bDebugEnabled = false;
	_bEEPROMEnabled = false;
	_bWifiEnabled = false;
	_bNTPEnabled = false;

	_eState = CONTROLLER_STATE::INITIALIZING;
	_nLastErrorCode = 0L;
	_strLastErrorDesc = "undefined.";

	_cMessage = "7uhr";
	_pDisplay = nullptr;

	_nMenuCurPos = CONTROLLER_MENU::CLOCK;
	_nMenuLastPosChange = millis();

	_bFirstTimeSetup = false;
	_nSerialNumber = 0;

	_nSetupHour = 21;
	_nSetupMinute = 42;

	_pHomeAssistant = nullptr;
}

void Controller::initializeEEPROM(bool forceFirstTimeSetup) {
	// initialize eeprom (hopefully only once!)
	EEPROM.begin(512);
	_bEEPROMEnabled = true;

	_bDeferredSavingToEEPROMScheduled = false;
	memset(_nDeferredSavingToEEPROMAt, 0, sizeof(uint32_t)*EEPROM_ADDRESS_COUNT);
	memset(_nDeferredSavingToEEPROMValue, 0, sizeof(uint8_t)*EEPROM_ADDRESS_COUNT);

	uint8_t serialNumber_lowByte = readFromEEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE);
	uint8_t serialNumber_highByte = readFromEEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE);
	_nSerialNumber = (serialNumber_highByte<<8) | serialNumber_lowByte;

	if(forceFirstTimeSetup || _nSerialNumber == 65535 || _nSerialNumber == 0) {
		// according to the ARDUINO documentation, EEPROM.read() returns 255 if no
		// has been written to the EEPROM before. therefore the siebenuhrs EEPROM
		// needs to be initialized with the default values and the serial number
		_bFirstTimeSetup = true;

		debugMessage(formatString("Executing first time setup..."));

		uint64_t addr = ESP.getEfuseMac();
		uint8_t serial[8];
		std::memcpy(serial, &addr, sizeof(addr));

		serialNumber_lowByte = serial[0]+serial[1]+serial[2]+serial[3]; // magic 
		serialNumber_highByte = serial[4]+serial[5]+serial[6]+serial[7]; // pony
		_nSerialNumber = (serialNumber_highByte<<8) | serialNumber_lowByte;

		writeToEEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_LOW_BYTE, serialNumber_lowByte, 0);
		writeToEEPROM(EEPROM_ADDRESS_SERIAL_NUMBER_HIGH_BYTE, serialNumber_highByte, 0);

		// lets save all the default values to EEPROM
		writeToEEPROM(EEPROM_ADDRESS_H, DEFAULT_COLOR.h, 0);
		writeToEEPROM(EEPROM_ADDRESS_S, DEFAULT_COLOR.s, 0);
		writeToEEPROM(EEPROM_ADDRESS_V, DEFAULT_COLOR.v, 0);
		writeToEEPROM(EEPROM_ADDRESS_BRIGHTNESS, 80, 0);
		writeToEEPROM(EEPROM_ADDRESS_DISPLAY_EFFECT_INDEX, 0, 0);
		writeToEEPROM(EEPROM_ADDRESS_COLOR_WHEEL_ANGLE, 171, 0);
		writeToEEPROM(EEPROM_ADDRESS_TIMEZONE_ID, 11  /*Europe-Zurich*/, 0);
		writeToEEPROM(EEPROM_ADDRESS_WIFI_ENABLED, 0, 0);

		flushDeferredSavingToEEPROM();
	}

	if (true) {
		bool useWifi = (readFromEEPROM(EEPROM_ADDRESS_WIFI_ENABLED) == 1);
		int nTimeZoneID = readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_ID);

		debugMessage("\nEEPROM setup completed.");
		debugMessage("SerialNumber   : %d", _nSerialNumber);
		debugMessage("Timezone       : %d", nTimeZoneID);
		debugMessage("Timezone       : %s", __timezones[nTimeZoneID].name);
		debugMessage("Enable WIFI    : %s", useWifi ? "true" : "false");
	}
}

String Controller::readStringFromEEPROM(uint8_t EEPROM_address, int maxLength) {
	char data[maxLength]; // Max 100 Bytes
	int len = 0;
	unsigned char k;
	k = EEPROM.read(EEPROM_address);
	while (k != '\0' && len < maxLength) {
		k = EEPROM.read(EEPROM_address + len);
		data[len] = k;
		len++;
	}
	data[len] = '\0';
	return String(data);
}

void Controller::writeStringToEEPROM(uint8_t EEPROM_address, String data, int maxLength) {
	if (data.equals(readStringFromEEPROM(EEPROM_address, maxLength))) {
		return;
	}
	int _size = data.length();
	int i;
	for (i = 0; i < _size || i < maxLength; i++) {
		EEPROM.write(EEPROM_address + i, data[i]);
		delay(10);
	}
	EEPROM.write(EEPROM_address + _size, '\0'); // Add termination null character for String Data
	EEPROM.commit();
}

uint8_t Controller::readFromEEPROM(uint8_t EEPROM_address) {
  uint8_t return_value = EEPROM.read(EEPROM_address);
  return return_value;
}

void Controller::writeToEEPROM(uint8_t EEPROM_address, uint8_t value, uint32_t delay) {
	//Utility function to schedule a deferred writing to save a value to EEPROM.
	uint32_t now = millis();
	_nDeferredSavingToEEPROMAt[EEPROM_address] = now+delay;
	_nDeferredSavingToEEPROMValue[EEPROM_address] = value;
	_bDeferredSavingToEEPROMScheduled = true;
}

void Controller::flushDeferredSavingToEEPROM() {
	uint32_t now = millis();
	for(int EEPROM_address=0; EEPROM_address<EEPROM_ADDRESS_COUNT; EEPROM_address++) {
		if (_nDeferredSavingToEEPROMAt[EEPROM_address] && now >= _nDeferredSavingToEEPROMAt[EEPROM_address]) {
			// only save if value != the value already in the EEPROM (to extend the TTL)
			uint8_t oldValue = EEPROM.read(EEPROM_address);
			if (oldValue != _nDeferredSavingToEEPROMValue[EEPROM_address]) {
				EEPROM.write(EEPROM_address, _nDeferredSavingToEEPROMValue[EEPROM_address]);
				debugMessage(formatString("[EEPROM] addr(%02d) = %d", EEPROM_address, _nDeferredSavingToEEPROMValue[EEPROM_address]));
			}
			_nDeferredSavingToEEPROMAt[EEPROM_address] = 0;
		}

		// check, if some other EEPROM value is scheduled for saving
		_bDeferredSavingToEEPROMScheduled = false;
		for(int i=0; i< EEPROM_ADDRESS_COUNT; i++) {
			if(_bDeferredSavingToEEPROMScheduled == false) {
				_bDeferredSavingToEEPROMScheduled = _nDeferredSavingToEEPROMAt[i] > 0 ? true : false;
			}
		}
	}

	// as no more commits are scheduled, lets commit to EEPROM
	if(_bDeferredSavingToEEPROMScheduled == false) {
		EEPROM.commit();
		debugMessage("[EEPROM] commit!");
	}
}

bool Controller::initializeDebug(bool enabled, int baud, int waitMilliseconds) {
	_bDebugEnabled = enabled;
	if (enabled) {
        Serial.begin(baud);
		debugMessage("\n7Uhr debugging enabled.");
	}
	return true;
}

void Controller::debugMessage(const String &message) {
	if (_bDebugEnabled) {
		Serial.println(message.c_str());
	}
}

void Controller::debugMessage(const char *format, ...) {
	static char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	if (_bDebugEnabled) {
		Serial.println(buffer);
	}
}

void Controller::debugValue(const char *key, const int value) {
	debugMessage(formatString("%s: %d", key, value));
}

bool Controller::initializeDisplay(DisplayDriver* display) {
	_pDisplay = display;

	if (_pDisplay == nullptr) {
		_strLastErrorDesc = "Failed to setup display and glyphs.";
		return false;
	}

    _pDisplay->disableNotification();

	return true;
}

bool Controller::initializeWifi(bool enabled) {
	WiFi.setHostname("7uhr");

	if (enabled) {
		if (WiFi.status() == WL_CONNECTED) {
			return true;
		}

		WiFi.mode(WIFI_STA);

		String SSID = readStringFromEEPROM(EEPROM_ADDRESS_WIFI_SSID, EEPROM_ADDRESS_MAX_LENGTH);
		String PSWD = readStringFromEEPROM(EEPROM_ADDRESS_WIFI_PSWD, EEPROM_ADDRESS_MAX_LENGTH);

		if (SSID.length() != 0) {
			WiFi.begin(SSID.c_str(), PSWD.c_str());
			debugMessage("Connecting to WiFi (%s)..", SSID);
			
			int ConnectRetries = 0;
			while (WiFi.status() != WL_CONNECTED && ConnectRetries < 50) {
				ConnectRetries++;
				debugMessage(".. retry #%d", ConnectRetries);
				delay(200);
			}

			if (WiFi.status() == WL_CONNECTED) {
				APController::getInstance()->getNetworkInfo();
				_bWifiEnabled = true;
				return true;
			} 
		}	
	} 

	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	_bWifiEnabled = false;

	return true;
}

bool Controller::initializeNTP(bool enabled, int timezoneId) {
	if (_bWifiEnabled && enabled) {
		if (timezoneId == -1) {
			timezoneId = (int)readFromEEPROM(EEPROM_ADDRESS_TIMEZONE_ID);
		}
		String sTimezone = __timezones[timezoneId].name;
		debugMessage(formatString("Timezone (EEPROM) : %s", sTimezone.c_str()));

		setDebug(INFO);
		while(timeStatus()==timeNotSet) {
			updateNTP();
			debugMessage("Waiting for time sync...");
			delay(100);
		}
		waitForSync();
		_pDisplay->setNewDefaultTimezone(sTimezone);
		_bNTPEnabled = enabled;
	} 
	
	return true;
}

void Controller::setKnob(int knobPinA, int knobPinB, int buttonPin) {
	_pKnobEncoder = new UIKnob(knobPinA, knobPinB, buttonPin);
}

CHSV Controller::getColor() {
	CHSV color;
	color.h = readFromEEPROM(EEPROM_ADDRESS_H);
	color.s = readFromEEPROM(EEPROM_ADDRESS_S);
	color.v = readFromEEPROM(EEPROM_ADDRESS_V);
	debugMessage("Get EEEPROM Color: %d %d %d", color.h, color.s, color.v);
	return color;
}

void Controller::begin() {
	// config display
	_pDisplay->setNotification(_cMessage, 3000);
	_eState = CONTROLLER_STATE::RUNNING;

	if (_bNTPEnabled) {
		setMenu(CONTROLLER_MENU::CLOCK);
	    _pDisplay->setOperationMode(OPERATION_MODE_CLOCK_HOURS);
	    //_pDisplay->setOperationMode(OPERATION_MODE_CLOCK_MINUTES);
		_pDisplay->setColor(getColor(), 0);
	} else {
		setMenu(CONTROLLER_MENU::SET_HOUR);
	    _pDisplay->setOperationMode(OPERATION_MODE_TIME_SETUP);
		_eState = CONTROLLER_STATE::SETUP_TIME;
	}

	IPAddress _mqttIP;    
    String _mqttUsername; 
	String _mqttPassword; 

	_mqttIP.fromString(readStringFromEEPROM(EEPROM_ADDRESS_HA_MQTT_IP, 20));
	_mqttUsername = readStringFromEEPROM(EEPROM_ADDRESS_HA_MQTT_USERNAME, EEPROM_ADDRESS_MAX_LENGTH);
	_mqttPassword = readStringFromEEPROM(EEPROM_ADDRESS_HA_MQTT_PASSWORD, EEPROM_ADDRESS_MAX_LENGTH);

	_pHomeAssistant = new HomeAssistant(_mqttIP, _mqttUsername, _mqttPassword);
	if (!_pHomeAssistant->setup()) {
		delete _pHomeAssistant;
		_pHomeAssistant = nullptr;
		debugMessage(formatString("WARNING: MQTT client could NOT connect to IP %s", _mqttIP.toString()));
	}
	else {
		debugMessage(formatString("MQTT client successfully connected to IP %s", _mqttIP.toString()));
		_pHomeAssistant->update();
	}
}

bool Controller::update() {
	if (_pKnobEncoder != nullptr) {
		_pKnobEncoder->update();
		handleMenu();

		if (_pKnobEncoder->getButtonPressTime() >= 5000) {
			debugMessage("start AP / WIFI setup...");
			_eState == CONTROLLER_STATE::SETUP_WIFI;
			APController::getInstance()->begin();

			initializeWifi(true);
			initializeNTP(true, APController::getInstance()->getSelectedTimeZone()); // needed, as EEPROM is probably not yet saved
		}
	}

	if (_eState == CONTROLLER_STATE::SETUP_TIME) {
		MessageExt msg;
		CHSV highlight = CHSV(140, 255, 220);
		CHSV lowlight = CHSV(171, 255, 220);
		msg.message[0] = (int)floor(_nSetupHour / 10) + '0';
		msg.message[1] = _nSetupHour % 10 + '0';
		msg.message[2] = (int)floor(_nSetupMinute / 10) + '0';
		msg.message[3] = _nSetupMinute % 10 + '0';
		if (_nMenuCurPos == CONTROLLER_MENU::SET_HOUR) {
			msg.color[0] = msg.color[1] = highlight;
			msg.color[2] = msg.color[3] = lowlight;
		} else {
			msg.color[0] = msg.color[1] = lowlight;
			msg.color[2] = msg.color[3] = highlight;
		}
		_pDisplay->setMessageExt(msg);
	}

	_pDisplay->update(_bWifiEnabled, _bNTPEnabled);

	if (_pHomeAssistant != nullptr) {
		_pHomeAssistant->update();
	}

	// save values to EEPROM (if scheduled)
	if(_bDeferredSavingToEEPROMScheduled) {
		flushDeferredSavingToEEPROM();
	}

	return true;
}

void Controller::setMenu(CONTROLLER_MENU menu) {
	_nMenuCurPos = menu;
	_nMenuLastPosChange = millis();

	debugMessage("MENU: %s (uid: %d)", _sMenu[_nMenuCurPos].name.c_str(), _sMenu[_nMenuCurPos].uid);
	
	switch (_nMenuCurPos) {			
	case CONTROLLER_MENU::SET_HOUR:
		_pKnobEncoder->setEncoderBoundaries(0, 23, DEFAULT_SETUP_HOUR, true);
		break;
	case CONTROLLER_MENU::SET_MINUTE:
		_pKnobEncoder->setEncoderBoundaries(0, 59, DEFAULT_SETUP_MINUTE, true);
		break;
	case CONTROLLER_MENU::CLOCK:
		_pKnobEncoder->setEncoderBoundaries(5, 255, _pDisplay->getBrightness());
		break;
	case CONTROLLER_MENU::HUE: 
		CHSV current_color = _pDisplay->getColor();
		_pKnobEncoder->setEncoderBoundaries(0, 255, current_color.hue, true);
		break;
	}
}

void Controller::handleMenu() {
	if(_pKnobEncoder->isButtonReleased()) {
		switch (_nMenuCurPos) {			
		case CONTROLLER_MENU::SET_HOUR:
			setMenu(CONTROLLER_MENU::SET_MINUTE);
			break;
		case CONTROLLER_MENU::SET_MINUTE:
			// show time as normal
			_pDisplay->setColor(getColor(), 5000);
		    _pDisplay->setOperationMode(OPERATION_MODE_CLOCK_HOURS);
			setTime(_nSetupHour, _nSetupMinute, 0, 1, 1, 2000);
			_eState = CONTROLLER_STATE::RUNNING;
			setMenu(CONTROLLER_MENU::CLOCK);
			break;
		case CONTROLLER_MENU::CLOCK:
			setMenu(CONTROLLER_MENU::HUE);
			break;
		case CONTROLLER_MENU::HUE:
			setMenu(CONTROLLER_MENU::CLOCK);
			break;
		}
    }

	// menu timeout, going back to clock display
	if (_nMenuCurPos == CONTROLLER_MENU::HUE && (millis() - _nMenuLastPosChange) > 10000) {
		setMenu(CONTROLLER_MENU::CLOCK);
		debugMessage("MENU Timeout! -> %s", _sMenu[_nMenuCurPos].name.c_str());
	}

	if (_pKnobEncoder->hasPositionChanged()) {
		long pos = _pKnobEncoder->getPosition(); 
		long delta = _pKnobEncoder->getPositionDiff();
		_nMenuLastPosChange = millis();

		switch(_sMenu[_nMenuCurPos].uid) {
			case CONTROLLER_MENU::SET_HOUR: {
				_nSetupHour = pos;
				debugMessage("manual time setup (HOUR) %d:%d", _nSetupHour, _nSetupMinute);
				break;
			}

			case CONTROLLER_MENU::SET_MINUTE: {
				_nSetupMinute = pos;
				debugMessage("manual time setup (MINUTE) %d:%d", _nSetupHour, _nSetupMinute);
				break;
			}

			case CONTROLLER_MENU::HUE: {
				CHSV color = CHSV( pos, 255, 220);
				debugMessage(formatString("Hue: %d", color.hue));
				_pDisplay->setColor(color, 0 ); // FIXME: We should store this value to EEPROM
				// _pDisplay->scheduleRedraw();
				break;
			}

			case CONTROLLER_MENU::CLOCK: {
				_pDisplay->setBrightness(pos);
				debugMessage(formatString("Brightness: %d", _pDisplay->getBrightness()));
				break;
			}
		}
	}
}
