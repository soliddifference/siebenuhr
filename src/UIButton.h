#ifndef _7U_BUTTON_H
#define _7U_BUTTON_H

#include <Arduino.h>
#include "SiebenUhr.h"

namespace siebenuhr {

class UIButton {

public:
  UIButton(uint8_t buttonPin);
  ~UIButton() {};

  void registerCallbacks(void (*callbackFncButton)(void));

  void update();
  void callbackButton(void);

  bool getState();
  bool isPressed(int countThreshold=1);
  bool isReleased(int countThreshold=1);
  int getTimeSinceStateChange();

private:
  uint8_t _nButtonPin;
  bool _bCurrentState;
  int _nUpdateCounter;
  int _nButtonStateChangeTime;
};

}

#endif
