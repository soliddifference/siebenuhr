/*  RotaryEncoderDriver.cpp -
	v0.1 tg 20180724 initial setup
  v0.2 ys 20190425 refactoring

  Class to handle the rotary encoder on the siebenuhr

*/
#include <Arduino.h>
#include "UIKnob.h"

using namespace siebenuhr;

UIKnob::UIKnob(uint8_t encoderPinA, uint8_t encoderPinB, uint8_t buttonPin) : UIButton(buttonPin) {
  _nEncoderPinA = encoderPinA;
  _nEncoderPinB = encoderPinB;
  _nEncoderChanged = false;
  _nEncoderDeltaSinceLastUpdate = 0;
  _nEncoderLastValueUpdateTimestamp = 0;
  _nEncoderCounter = 0;
  _nPrevEncoderCounter = 0;

  // setup pins
  pinMode(_nEncoderPinA, INPUT_PULLUP);
  pinMode(_nEncoderPinB, INPUT_PULLUP);
}

void UIKnob::registerCallbacks(void (*callbackFncButton)(void), void (*callbackFncKnob)(void)) {
  UIButton::registerCallbacks(callbackFncButton);
  attachInterrupt(digitalPinToInterrupt(this->_nEncoderPinA), callbackFncKnob, FALLING);
}

void IRAM_ATTR UIKnob::callbackEncoder() {
  noInterrupts();                              //disable interrupts
  uint8_t currValueA  = digitalRead(_nEncoderPinA);
  uint8_t currValueB  = digitalRead(_nEncoderPinB);
  if(currValueA == currValueB) {
    _nEncoderCounter++;
  } else {
    _nEncoderCounter--;
  }

  uint32_t now = millis();
  if(_nEncoderLastValueUpdateTimestamp + 100 < now) {
    _nEncoderLastValueUpdateTimestamp = now;
    _nEncoderChanged = true;
  }

  interrupts();
}

int16_t UIKnob::encoderChanged() {
  if(_nEncoderChanged) {
    _nEncoderChanged = false;
    _nEncoderDeltaSinceLastUpdate = _nPrevEncoderCounter - _nEncoderCounter;
    _nPrevEncoderCounter = _nEncoderCounter;
    return _nEncoderDeltaSinceLastUpdate;
  }
  return 0;
}

void UIKnob::setPosition(int16_t position) {
  _nEncoderCounter = position;
}

int16_t UIKnob::getPosition() {
  return _nEncoderCounter;
}

void UIKnob::update() {
  UIButton::update();
}
