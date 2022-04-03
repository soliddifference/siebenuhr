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

	bool setup(DisplayDriver* display, AsyncWiFiManager* WiFiManager);
	bool update();

	inline int getLastErrorCode() { return _nLastErrorCode; };
	inline char* getLastErrorDesc() { return (char*)_strLastErrorDesc.c_str(); };

	void setResetButton(int buttonResetPin);
	void setKnob(int knobPinA, int knobPinB, int buttonPin);

private:
	Controller();

	void handleUIResetButton();
	void handleUIKnob();

	CONTROLLER_STATE _eState;
	int _nLastErrorCode;
	String _strLastErrorDesc;

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
