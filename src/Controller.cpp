/*  Controller.cpp
	v0.1 ys 20190422 refactoring

	Class to manage all in- and outout of siebenuhr
*/

#include <Arduino.h>
#include "Controller.h"

#include <WiFi.h>

using namespace siebenuhr;

Controller* Controller::_pInstance = nullptr;
UIButton* Controller::_pResetButton = nullptr;
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

	_eState = CONTROLLER_STATE::undefined;
	_nLastErrorCode = 0L;
	_strLastErrorDesc = "undefined.";

	_cMessage = "7uhr";
	_pDisplay = nullptr;
	_pWiFiManager = nullptr;

	_nMenuCurPos = CONTROLLER_MENU::CLOCK;
	_nMenuLastPosChange = millis();

	_bFirstTimeSetup = false;
	_nSerialNumber = 0;
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
		writeToEEPROM(EEPROM_ADDRESS_TIMEZONE_HOUR, 1, 0);

		saveToEEPROM();
	}

	debugValue("SerialNumber", _nSerialNumber);
}

uint8_t Controller::readFromEEPROM(uint8_t EEPROM_address) {
  uint8_t return_value = EEPROM.read(EEPROM_address);
  return return_value;
}

void Controller::saveToEEPROM() {
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

void Controller::writeToEEPROM(const int EEPROM_address, uint8_t value, uint32_t delay) {
	//Utility function to schedule a deferred writing to save a value to EEPROM.
	uint32_t now = millis();
	_nDeferredSavingToEEPROMAt[EEPROM_address] = now+delay;
	_nDeferredSavingToEEPROMValue[EEPROM_address] = value;
	_bDeferredSavingToEEPROMScheduled = true;
}

bool Controller::initializeDisplay(DisplayDriver* display) {
	_pDisplay = display;

	if (_pDisplay == nullptr) {
		_strLastErrorDesc = "Failed to setup display and glyphs.";
		return false;
	}

	// set welcome message
	_pDisplay->setNotification(_cMessage, 3000);

	_eState = CONTROLLER_STATE::initialized;
	setMenu(CONTROLLER_MENU::CLOCK);

	debugMessage("Controller::initializeDisplay");

	return true;
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

bool Controller::initializeWifi(bool enabled, AsyncWiFiManager* WiFiManager) {
	_bWifiEnabled = enabled;
	if (enabled) {
		_pWiFiManager = WiFiManager;

		if (_pWiFiManager == nullptr) {
			_strLastErrorDesc = "Failed to setup wifi component.";
			return false;
		}

		debugMessage(formatString("SSID              : %s", WiFi.SSID()));
		debugMessage(formatString("IP address (DHCP) : %s", WiFi.localIP()));
		debugMessage(formatString("MAC address is    : %s", WiFi.macAddress()));
	}
	return true;
}

bool Controller::initializeNTP(bool enabled) {
	_bNTPEnabled = enabled;
	if (_bWifiEnabled && _bNTPEnabled) {
		String sTimezone = EEPROMReadString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH);
		debugMessage(formatString("Timezone (EEPROM) : %s", sTimezone.c_str()));

		setDebug(INFO);
		while(timeStatus()==timeNotSet) {
			updateNTP();
			debugMessage("Waiting for time sync...");
			delay(100);
		}
		waitForSync();
		_pDisplay->setNewDefaultTimezone(sTimezone);
	} else {
		// todo: manual setup?
	}
	
	return true;
}

void Controller::setResetButton(int buttonResetPin) {
	_pResetButton = new UIButton(buttonResetPin);
	_pResetButton->registerCallbacks([]{_pResetButton->callbackButton();});
}

void Controller::setKnob(int knobPinA, int knobPinB, int buttonPin) {
	_pKnobEncoder = new UIKnob(knobPinA, knobPinB, buttonPin);
}

bool Controller::update() {
	if (_eState != CONTROLLER_STATE::initialized) {
		return false;
	}

	if (_pResetButton != nullptr) {
		_pResetButton->update();
		handleResetButton();
	}

	if (_pKnobEncoder != nullptr) {
		_pKnobEncoder->update();
		handleMenu();
	}

	_pDisplay->update(_bWifiEnabled, _bNTPEnabled);

	// save values to EEPROM (if scheduled)
	if(_bDeferredSavingToEEPROMScheduled) {
		saveToEEPROM();
	}

	return true;
}

void Controller::handleResetButton() {
	if (_pResetButton->getState()) {
	    int countdown_int = (5 - (int)(_pResetButton->getTimeSinceStateChange()/1000.f));
	    if (countdown_int >= 0) {
			_cMessage = formatString("rst%d", countdown_int);
			debugMessage(_cMessage);
			_pDisplay->setNotification(_cMessage);
	    } else {
			_cMessage = "boom";
			debugMessage("Restaring siebenuhr after WIFI-reset.");
			_pDisplay->setNotification(_cMessage);
			// code in wifi manager is broken. Just called for debug messages....
			_pWiFiManager->resetSettings();
			// here comes the actual reset. Only works after WiFi.begin(); (!)
			WiFi.disconnect(true, true);
			// also reset the timezone stored in the EEPROM
			EEPROMWriteString(EEPROM_ADDRESS_TIMEZONE_OLSON_STRING, "", EEPROM_ADDRESS_TIMEZONE_OLSON_STRING_LENGTH-1);
			delay(3000);
			ESP.restart();
			delay(3000);
	    }
	} 
}

void Controller::setMenu(CONTROLLER_MENU menu) {
	_nMenuCurPos = menu;
	_nMenuLastPosChange = millis();

	debugMessage("MENU: %s (uid: %d)", _sMenu[_nMenuCurPos].name.c_str(), _sMenu[_nMenuCurPos].uid);
	
	if (_nMenuCurPos == CONTROLLER_MENU::HUE) {
		CHSV current_color = _pDisplay->getColor();
		_pKnobEncoder->setEncoderBoundaries(0, 255, current_color.hue);
	} else if (_nMenuCurPos == CONTROLLER_MENU::CLOCK) {
		_pKnobEncoder->setEncoderBoundaries(0, 255, _pDisplay->getBrightness());
	}
}

void Controller::handleMenu() {
	if(_pKnobEncoder->isButtonReleased()) {
		if (_nMenuCurPos ==  CONTROLLER_MENU::CLOCK) {
			setMenu(CONTROLLER_MENU::HUE);
		} else {
			setMenu(CONTROLLER_MENU::CLOCK);
		}

		_pDisplay->setNotification(_sMenu[_nMenuCurPos].message, 2000);
    }

	// menu timeout, going back to clock display
	if (_nMenuCurPos != CONTROLLER_MENU::CLOCK && (millis() - _nMenuLastPosChange) > 10000) {
		setMenu(CONTROLLER_MENU::CLOCK);
		debugMessage("MENU Timeout! -> %s", _sMenu[_nMenuCurPos].name.c_str());
	}

	if (_pKnobEncoder->hasPositionChanged()) {
		long pos = _pKnobEncoder->getPosition(); 
		_nMenuLastPosChange = millis();

		switch(_sMenu[_nMenuCurPos].uid) {
			case CONTROLLER_MENU::HUE: {
				CHSV current_color = _pDisplay->getColor();
				current_color.hue = pos;
				debugMessage(formatString("Hue: %d", current_color.hue));
				_pDisplay->setColor(current_color, true /* SAFE TO EEPROM*/);
				_pDisplay->scheduleRedraw(0);
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
