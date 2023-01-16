#ifndef _7U_CONTROLLER_H
#define _7U_CONTROLLER_H

#include "SiebenUhr.h"

#include <DisplayDriver.h>
#include <ESPAsyncWiFiManager.h>

#include "UIButton.h"
#include "UIKnob.h"

namespace siebenuhr {

enum CONTROLLER_STATE {
	undefined,
	initialized,
	wifi_setup,
};

enum CONTROLLER_MENU {
	CLOCK = 0,
	BRIGHTNESS,
	HUE,
	SATURATION,
	EFFECT,
	TIMEZONE,
	SET_HOUR,
	SET_MINUTE,
	MENU_END
};

struct ControllerMenu_t {
  CONTROLLER_MENU uid;
  String name;
  String message;
};

class Controller {
public:
	static const int _nMenuMaxEntries = CONTROLLER_MENU::MENU_END;
	static const ControllerMenu_t _sMenu[_nMenuMaxEntries];

	static Controller* getInstance();

	// update and run
	bool initializeDebug(bool enabled, int baud=115200, int waitMilliseconds=3000);
	void initializeEEPROM(bool forceFirstTimeSetup=false);
	bool initializeWifi(bool enabled, AsyncWiFiManager* WiFiManager);
	bool initializeNTP(bool enabled);
	bool initializeDisplay(DisplayDriver* display);

	bool update();

	// error handling
	inline int getLastErrorCode() { return _nLastErrorCode; };
	inline char* getLastErrorDesc() { return (char*)_strLastErrorDesc.c_str(); };

	// configuration
	inline bool getFirstTimeSetup() { return _bFirstTimeSetup; };
	inline uint16_t getSerialNumber() { return _nSerialNumber; };

	void setResetButton(int buttonResetPin);
	void setKnob(int knobPinA, int knobPinB, int buttonPin);

	// EEPROM
	uint8_t readFromEEPROM(uint8_t EEPROM_address);
	void writeToEEPROM(const int EEPROM_address, uint8_t value, uint32_t delay=10000);

	// debugging and logging
	// void debugMessage(const char *msg);
	void debugMessage(const String &s);
	void debugMessage(const char *format, ...);
	void debugValue(const char *key, const int value);

private:
	Controller();

	void saveToEEPROM();

	void handleUIResetButton();
	void handleUIKnob();

	CONTROLLER_STATE _eState;
	int _nLastErrorCode;
	String _strLastErrorDesc;

	// config
	bool _bDebugEnabled;
	bool _bEEPROMEnabled;
	bool _bWifiEnabled;
	bool _bNTPEnabled;

	bool _bFirstTimeSetup;
	uint16_t _nSerialNumber;

	// EEPROM
	bool _bDeferredSavingToEEPROMScheduled;
	uint32_t _nDeferredSavingToEEPROMAt[EEPROM_ADDRESS_COUNT];
	uint8_t  _nDeferredSavingToEEPROMValue[EEPROM_ADDRESS_COUNT];

	// hardware interfaces
	static UIButton* _pResetButton;
	static UIKnob* _pKnob;
	UIButton *_pButton;

	// hardware components
	DisplayDriver* _pDisplay;
	AsyncWiFiManager* _pWiFiManager;

	// MENU
	uint8_t _nMenuCurPos;
	uint32_t _nMenuLastPosChange;

	String _cMessage;
	static Controller* _pInstance;
};

}

#endif
