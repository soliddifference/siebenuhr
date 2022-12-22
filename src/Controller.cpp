/*  Controller.cpp
	v0.1 ys 20190422 refactoring

	Class to manage all in- and outout of siebenuhr
*/

#include <Arduino.h>
#include "Controller.h"

using namespace siebenuhr;

Controller* Controller::_pInstance = nullptr;
UIButton* Controller::_pResetButton = nullptr;
UIKnob* Controller::_pKnob = nullptr;

const ControllerMenu_t Controller::_sMenu[Controller::_nMenuMaxEntries] = {
		{CONTROLLER_MENU::BRIGHTNESS, "Brightness", "Brit"},
		{CONTROLLER_MENU::HUE, "Hue", "COLO"},
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

	_nMenuCurPos = 0;
	_nMenuMaxPos = 10;

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
		_strLastErrorDesc = "Failed to setup wifi component.";
		return false;
	}

	_pDisplay->set_notification(_cMessage);

	_eState = CONTROLLER_STATE::initialized;
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

void Controller::debugMessage(const char* message) {
	if (_bDebugEnabled) {
		Serial.println(message);
	}
}

void Controller::debugMessage(const String &message) {
	if (_bDebugEnabled) {
		Serial.println(message.c_str());
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
	}
	return true;
}

bool Controller::initializeNTP(bool enabled) {
	_bNTPEnabled = enabled;
	// todo:
	return true;
}

void Controller::setResetButton(int buttonResetPin) {
	_pResetButton = new UIButton(buttonResetPin);
	_pResetButton->registerCallbacks([]{_pResetButton->callbackButton();});
}

void Controller::setKnob(int knobPinA, int knobPinB, int buttonPin) {
	_pKnob = new UIKnob(knobPinA, knobPinB, buttonPin);
	_pKnob->registerCallbacks([]{_pKnob->callbackButton();}, []{_pKnob->callbackEncoder();});
}

bool Controller::update() {
	if (_eState != CONTROLLER_STATE::initialized) {
		return false;
	}

	if (_pResetButton != nullptr) {
		_pResetButton->update();
		handleUIResetButton();
	}

	if (_pKnob != nullptr) {
		_pKnob->update();
		handleUIKnob();
	}

	_pDisplay->update();

	// save values to EEPROM (if scheduled)
	if(_bDeferredSavingToEEPROMScheduled) {
		saveToEEPROM();
	}

	return true;
}

void Controller::handleUIResetButton() {
	if (_pResetButton->getState()) {
	    int countdown_int = (5 - (int)(_pResetButton->getTimeSinceStateChange()/1000.f));
	    if (countdown_int >= 0) {
			_cMessage = formatString("rst%d", countdown_int);
			debugMessage(_cMessage);
			_pDisplay->set_notification(_cMessage);
	    } else {
			_cMessage = "boom";
			debugMessage("Restaring siebenuhr after WIFI-reset.");
			_pDisplay->set_notification(_cMessage);
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
	} else {
	    _pDisplay->disable_notification();
	}
}

void Controller::handleUIKnob() {
	if(_pKnob->isPressed()) {
		_nMenuCurPos = (_nMenuCurPos+1) == _nMenuMaxEntries ? 0 : _nMenuCurPos+1;
		debugMessage(formatString("MENU: %s", _sMenu[_nMenuCurPos].name.c_str()));
		_pDisplay->set_notification(_sMenu[_nMenuCurPos].message, 3000);
    }

  	int8_t encoderDelta = _pKnob->encoderChanged();
  	if (encoderDelta == 0) return;

	switch(_sMenu[_nMenuCurPos].uid) {
		case CONTROLLER_MENU::BRIGHTNESS: {
			int brightness_index = _pDisplay->get_brightness_index();
			brightness_index += encoderDelta;
			_pDisplay->save_and_set_new_default_brightness_index(brightness_index);
			brightness_index = _pDisplay->get_brightness_index();
			debugMessage(formatString("MENU: %s - Brightness: %d", _sMenu[_nMenuCurPos].name.c_str(), brightness_index));
			break;
		}

		case CONTROLLER_MENU::HUE: {
			uint8_t current_display_effect = _pDisplay->get_display_effect();
			switch(current_display_effect) {
				case DISPLAY_EFFECT_DAYLIGHT_WHEEL: {
					uint8_t color_wheel_angle = _pDisplay->get_color_wheel_angle();
					color_wheel_angle += encoderDelta*2;
					_pDisplay->save_new_color_wheel_angle(color_wheel_angle);
					_pDisplay->schedule_redraw();
					_pDisplay->schedule_redraw_with_special_blending_period(0);
					break;
				}
				case DISPLAY_EFFECT_SOLID_COLOR: {
					CHSV current_color = _pDisplay->get_solid_color();
					current_color.hue += encoderDelta*2;
					_pDisplay->save_and_set_new_default_solid_color(current_color);
					_pDisplay->schedule_redraw();
					_pDisplay->schedule_redraw_with_special_blending_period(0);
					break;
				}
			}
			break;
		}

		case CONTROLLER_MENU::SATURATION: {
          CHSV current_color = _pDisplay->get_solid_color();
          int32_t saturation = current_color.saturation;

		  saturation += encoderDelta*3;
          if (saturation >= 255) saturation = 255;
          if (saturation <= 0) saturation = 0;
          current_color.saturation = saturation;

          _pDisplay->save_and_set_new_default_solid_color(current_color);
          _pDisplay->schedule_redraw();
          _pDisplay->schedule_redraw_with_special_blending_period(0);
          break;
        }

		case CONTROLLER_MENU::EFFECT: {
			if(encoderDelta>0) {
				_pDisplay->adjust_and_save_new_display_effect(true);
			} else if(encoderDelta<0) {
				_pDisplay->adjust_and_save_new_display_effect(false);
			}
			debugMessage(formatString("current effect: %d", _pDisplay->get_display_effect()));
			_pDisplay->set_notification(_pDisplay->get_display_effect_short(), 3000);
			break;
		}

		case CONTROLLER_MENU::TIMEZONE: {
			int8_t timeZone = _pDisplay->get_timezone_hour();
			debugMessage(formatString("current TZ: %d", timeZone));
			// FIXME: before shipping to north korea et al I'll have to implenment half-hour timezones as well
			if(encoderDelta>0) timeZone++;
			else if (encoderDelta<0) timeZone--;
			debugMessage(formatString("new TZ: %d", timeZone));
			_pDisplay->set_timezone_hour(timeZone);

			char notifcation[4] { 0, 0, 0, 0 };
			if(timeZone>=0) {
				notifcation[0] = '-';
				notifcation[1] = '+';
			}
			else if(timeZone<0) {
				notifcation[0] = ' ';
				notifcation[1] = '-';
			}

			int digit2 = (int) floor(abs(timeZone)/10);
			if(digit2==0) {
				notifcation[2] = ' ';
			} else {
				notifcation[2] = (int) floor(abs(timeZone)/10) + '0';
			}
			// TODO: Fix this. was : notifcation[3] = abs(timeZone)%10 + '0';
			notifcation[3] = '0';
			_pDisplay->set_notification(notifcation, 3000);
			break;
		}

        case CONTROLLER_MENU::SET_HOUR: {
			uint32_t ts = now();
			int8_t hour_delta = encoderDelta;
			ts += hour_delta*3600;
			setTime(ts);
			_pDisplay->schedule_redraw();
			_pDisplay->schedule_redraw_with_special_blending_period(0);
			break;
        }

        case CONTROLLER_MENU::SET_MINUTE: {
			uint32_t ts = now();
			int8_t min_delta = encoderDelta;
			ts += min_delta*60;
			setTime(ts);
			_pDisplay->schedule_redraw();
			_pDisplay->schedule_redraw_with_special_blending_period(0);
			break;
        }

		default: {
			// by default, turning the knob defines the brightness of the clock
			int brightness_index = _pDisplay->get_brightness_index();
			brightness_index += encoderDelta;
			_pDisplay->save_and_set_new_default_brightness_index(brightness_index);
			brightness_index = _pDisplay->get_brightness_index();
			debugMessage(formatString("Brightness: %d", brightness_index));
			break;
		}
    }
}
