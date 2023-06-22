#include <Arduino.h>
#include "UIKnob.h"

using namespace siebenuhr;

#define ROTARY_ENCODER_STEPS 4

 AiEsp32RotaryEncoder* UIKnob::_pRotaryEncoder = nullptr;

void IRAM_ATTR UIKnob::handleEncoderInterrupt() {
	_pRotaryEncoder->readEncoder_ISR();
}

void IRAM_ATTR UIKnob::handleButtonInterrupt() {
	_pRotaryEncoder->readButton_ISR();
}

UIKnob::UIKnob(uint8_t encoderPinA, uint8_t encoderPinB, uint8_t buttonPin) 
{
	_pRotaryEncoder = new AiEsp32RotaryEncoder(encoderPinA, encoderPinB, buttonPin, -1, ROTARY_ENCODER_STEPS);
    _pRotaryEncoder->begin();
    _pRotaryEncoder->setup(handleEncoderInterrupt, handleButtonInterrupt);
    _pRotaryEncoder->setBoundaries(0, 1000, false); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    _pRotaryEncoder->setAcceleration(250);
}

UIKnob::~UIKnob() {
  delete _pRotaryEncoder;
}

void UIKnob::update() {
	// todo
}

int16_t UIKnob::encoderChanged()
{
	if (_pRotaryEncoder->encoderChanged()) {
		_nEncoderCounter = _pRotaryEncoder->readEncoder();		
        Serial.println(_nEncoderCounter);
		return _nEncoderCounter;
    }
	return 0;
}

void UIKnob::setPosition(int16_t position) {
	_nEncoderCounter = position;
	_pRotaryEncoder->setEncoderValue(_nEncoderCounter);
}

int16_t UIKnob::getPosition() {
	return _nEncoderCounter;
}

bool UIKnob::getState() {
	bool isEncoderButtonDown = _pRotaryEncoder->isEncoderButtonDown();
	return isEncoderButtonDown;
}

bool UIKnob::isPressed(int countThreshold) {
	return getState();
}

bool UIKnob::isReleased(int countThreshold) {
	return !getState();
}
