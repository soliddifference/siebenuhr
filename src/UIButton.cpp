/*  Button.cpp
  v0.1 tg 20180724 initial setup
  v0.2 ys 20190422 refactoring

  Class to handle the (reset) button on the siebenuhr
*/

#include <Arduino.h>
#include "UIButton.h"

using namespace siebenuhr;

UIButton::UIButton(uint8_t buttonPin) {
  _bCurrentState = false;
  _nButtonPin = buttonPin;
  _nButtonStateChangeTime = millis();
  _nUpdateCounter = 999;

  pinMode(FUNCTION_LED, OUTPUT);
  pinMode(_nButtonPin, INPUT);
}

void UIButton::registerCallbacks(void (*callbackFncButton)(void)) {
  attachInterrupt(digitalPinToInterrupt(this->_nButtonPin), callbackFncButton, CHANGE);
}

void UIButton::update() {
  _nUpdateCounter++;
  if (isPressed()) {
    digitalWrite(FUNCTION_LED, HIGH);
  } else {
    digitalWrite(FUNCTION_LED, LOW);
  }
  // Serial.print(_sTag);
  // Serial.print("Button Counter: ");
  // Serial.println(_nUpdateCounter);
}

void IRAM_ATTR UIButton::callbackButton() {
  noInterrupts();
  _bCurrentState = digitalRead(_nButtonPin); //LOW = pressed & HIGH = not pressed, because internal pull-up resistor is enabled
  _nUpdateCounter = 0;
  _nButtonStateChangeTime = millis();
  //
  // Serial.print(_sTag);
  // Serial.println(_bCurrentState?"button: TRUE":"button: FALSE");
  //
  interrupts();
}

bool UIButton::getState() {
  return _bCurrentState;
}

bool UIButton::isPressed(int countThreshold) {
  return (getState() && _nUpdateCounter <= countThreshold) ;
}

bool UIButton::isReleased(int countThreshold) {
  return (!getState() && _nUpdateCounter <= countThreshold) ;
}

int UIButton::getTimeSinceStateChange() {
  return millis() - _nButtonStateChangeTime;
}
