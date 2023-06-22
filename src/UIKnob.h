#ifndef _7U_KNOB_H
#define _7U_KNOB_H

#include "SiebenUhr.h"

#include <AiEsp32RotaryEncoder.h>

namespace siebenuhr {

class UIKnob {

public:
  UIKnob(uint8_t encoderPinA, uint8_t encoderPinB, uint8_t buttonPin);
  ~UIKnob();

  void update();

  void setPosition(int16_t position);
  int16_t getPosition();
  int16_t encoderChanged();

  bool getState();
  bool isPressed(int countThreshold=1);
  bool isReleased(int countThreshold=1);

  static void IRAM_ATTR handleEncoderInterrupt();
  static void IRAM_ATTR handleButtonInterrupt();

private:
  volatile int16_t _nEncoderCounter;
  static AiEsp32RotaryEncoder *_pRotaryEncoder;
};

}

#endif
