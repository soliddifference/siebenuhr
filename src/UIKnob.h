#ifndef _7U_KNOB_H
#define _7U_KNOB_H

#include "SiebenUhr.h"

#include "UIButton.h"

namespace siebenuhr {

class UIKnob : public UIButton {

public:
  UIKnob(uint8_t encoderPinA, uint8_t encoderPinB, uint8_t buttonPin);
  ~UIKnob() {};

  void registerCallbacks(void (*callbackFncButton)(void), void (*callbackFncKnob)(void));

  void update();
  void callbackEncoder(void);

  void setPosition(int16_t position);
  int16_t getPosition();

  int16_t encoderChanged();

private:
  uint8_t _nEncoderPinA;
  uint8_t _nEncoderPinB;

  bool _nEncoderChanged;

  // volatile int16_t mCurr_value = 0;
  volatile int16_t _nEncoderDeltaSinceLastUpdate;
  volatile uint32_t _nEncoderLastValueUpdateTimestamp;

  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

protected:
  volatile int16_t _nPrevEncoderCounter;
  volatile int16_t _nEncoderCounter;
};

}

#endif
