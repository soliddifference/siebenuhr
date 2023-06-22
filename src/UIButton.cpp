#include <Arduino.h>
#include "UIButton.h"

using namespace siebenuhr;

UIButton::UIButton(uint8_t buttonPin)
{
	_bCurrentState = false;
	_nButtonPin = buttonPin;
	_nButtonStateChangeTime = millis();
	_nUpdateCounter = 999;

	pinMode(FUNCTION_LED, OUTPUT);
	pinMode(_nButtonPin, INPUT);
}

void UIButton::registerCallbacks(void (*callbackFncButton)(void))
{
	attachInterrupt(digitalPinToInterrupt(this->_nButtonPin), callbackFncButton, CHANGE);
}

void UIButton::update()
{
	_nUpdateCounter++;
	if (isPressed()) {
		digitalWrite(FUNCTION_LED, HIGH);
	} else {
		digitalWrite(FUNCTION_LED, LOW);
	}
}

void IRAM_ATTR UIButton::callbackButton()
{
	_bCurrentState = digitalRead(_nButtonPin); // LOW = pressed & HIGH = not pressed, because internal pull-up resistor is enabled
	_nUpdateCounter = 0;
	_nButtonStateChangeTime = millis();
}

bool UIButton::getState()
{
	return _bCurrentState;
}

bool UIButton::isPressed(int countThreshold)
{
	return (getState() && _nUpdateCounter <= countThreshold);
}

bool UIButton::isReleased(int countThreshold)
{
	return (!getState() && _nUpdateCounter <= countThreshold);
}

int UIButton::getTimeSinceStateChange()
{
	return millis() - _nButtonStateChangeTime;
}
