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
	BRIGHTNESS,
	HUE,
	SATURATION,
	EFFECT,
	TIMEZONE,
	SET_HOUR,
	SET_MINUTE
};

struct ControllerMenu_t {
  CONTROLLER_MENU uid;
  String name;
  String message;
};

class Controller {
public:
	static const int _nMenuMaxEntries = 7;
	static const ControllerMenu_t _sMenu[_nMenuMaxEntries];

	static Controller* getInstance();

	bool initializeDebug(bool enabled, int baud=115200, int waitMilliseconds=3000);
	bool initializeWifi(bool enabled, AsyncWiFiManager* WiFiManager);
	bool initializeNTP(bool enabled);
	bool initializeDisplay(DisplayDriver* display);

	bool update();

	inline int getLastErrorCode() { return _nLastErrorCode; };
	inline char* getLastErrorDesc() { return (char*)_strLastErrorDesc.c_str(); };

	void setResetButton(int buttonResetPin);
	void setKnob(int knobPinA, int knobPinB, int buttonPin);

	void debugMessage(const char *msg);

private:
	Controller();

	void handleUIResetButton();
	void handleUIKnob();

	CONTROLLER_STATE _eState;
	int _nLastErrorCode;
	String _strLastErrorDesc;
	bool _bDebugEnabled;
	bool _bWifiEnabled;
	bool _bNTPEnabled;

	// hardware interfaces
	static UIButton* _pResetButton;
	static UIKnob* _pKnob;
	UIButton *_pButton;

	// hardware components
	DisplayDriver* _pDisplay;
	AsyncWiFiManager* _pWiFiManager;

	// MENU
	int _nMenuCurPos;
	int _nMenuMaxPos;

	String _cMessage;
	static Controller* _pInstance;
};

}

#endif
