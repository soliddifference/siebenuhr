/*  Segment.cpp -
	v0.1 tg 20180119 initial setup

  Class to handle one glyph of the 7clock

*/
#include <Arduino.h>
#include "Glyph.h"

Glyph::Glyph(int leds_per_segment) :
  _glyphNext(new int[SEGMENT_COUNT*leds_per_segment]),
  _glyphCurrent(new int[SEGMENT_COUNT*leds_per_segment]),
  _leds(new CRGB[SEGMENT_COUNT*leds_per_segment]),
  _leds_char(new CRGB[SEGMENT_COUNT*leds_per_segment]),
  _leds_effect(new CRGB[SEGMENT_COUNT*leds_per_segment]),
  LEDS_PER_SEGMENT(leds_per_segment),
  _effectMapperVertical(new CRGB[2*leds_per_segment+3]) {
  _lastUpdate = millis();

  // total leds per glyph
  _numLEDS = SEGMENT_COUNT * leds_per_segment;
  }

Glyph::~Glyph () {
  // destructor
}

void Glyph::attach(int data_pin_in, int glyph_id_in) {
  _glyphID = glyph_id_in;
  if(data_pin_in == 0) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_0>(_leds, _numLEDS);
  }
  else if(data_pin_in == 1) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_1>(_leds, _numLEDS);
  }
  else if(data_pin_in == 2) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_2>(_leds, _numLEDS);
  }
  else if(data_pin_in == 3) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_3>(_leds, _numLEDS);
  }
}

void Glyph::attach(int glyph_id_in) {
  _glyphID = glyph_id_in;
  _glyphOffset = (3-_glyphID)*SEGMENT_COUNT*LEDS_PER_SEGMENT;
}

void Glyph::setNextChar(char character, int fade_interval) {
    int character_ascii_code = character;
    _glyphChangeBlendingPeriodStarted = millis();
    _glyphChangeBlendingPeriod = fade_interval;
    for (int i=0; i<SEGMENT_COUNT; i++) {
      for(int j=0; j<LEDS_PER_SEGMENT; j++) {
        _glyphCurrent[i*LEDS_PER_SEGMENT+j] = _glyphNext[i*LEDS_PER_SEGMENT+j];
        _glyphNext[i*LEDS_PER_SEGMENT+j] = ASCII_TABLE[character_ascii_code][i];
      }
    }
}

void Glyph::setColorRGB(CRGB color, int fade_interval_ms) {
    _colorChangeBlendingPeriodStarted = millis();
    _colorChangeBlendingPeriod = fade_interval_ms;
    _colorBase = _colorCurrent;
    _colorNext = color;
}

void Glyph::updateBlendingToNextColor() {
  unsigned long now = millis();

  // in case we have enough time for blending, we prepare the next color here. 
  if(now - _colorChangeBlendingPeriodStarted  < _colorChangeBlendingPeriod) {
    int iterator = floor((now - _colorChangeBlendingPeriodStarted)/DISPLAY_REFRESH_INTERVAL);
    iterator = floor(255*(float)iterator/((float)_colorChangeBlendingPeriod/(float)DISPLAY_REFRESH_INTERVAL));
    _colorCurrent.r = lerp8by8(_colorBase.r, _colorNext.r, iterator );
    _colorCurrent.g = lerp8by8(_colorBase.g, _colorNext.g, iterator );
    _colorCurrent.b = lerp8by8(_colorBase.b, _colorNext.b, iterator );
  }
  // else covers the case, where we don't have time for blending, but the new color needs to be set immediatly.
  else {
    _colorCurrent.r = _colorNext.r;
    _colorCurrent.g = _colorNext.g;
    _colorCurrent.b = _colorNext.b;
  }
}

void Glyph::update() {
  unsigned long now = millis();
  if(now - _glyphChangeBlendingPeriodStarted  < _glyphChangeBlendingPeriod ) {
    int iterator = floor((now - _glyphChangeBlendingPeriodStarted)/DISPLAY_REFRESH_INTERVAL);
    iterator = floor(255*(float)iterator/((float)_glyphChangeBlendingPeriod/(float)DISPLAY_REFRESH_INTERVAL));
    for (int i = 0; i < _numLEDS; i++) {
      _leds_char[i].r = lerp8by8(_glyphCurrent[i]*_colorCurrent.r, _glyphNext[i]*_colorCurrent.r, iterator );
      _leds_char[i].g = lerp8by8(_glyphCurrent[i]*_colorCurrent.g, _glyphNext[i]*_colorCurrent.g, iterator );
      _leds_char[i].b = lerp8by8(_glyphCurrent[i]*_colorCurrent.b, _glyphNext[i]*_colorCurrent.b, iterator );
    }
  }
  else {
    for (int i = 0; i < _numLEDS; i++) {
      _leds_char[i].r = _glyphNext[i]*_colorCurrent.r;
      _leds_char[i].g = _glyphNext[i]*_colorCurrent.g;
      _leds_char[i].b = _glyphNext[i]*_colorCurrent.b;
    }
  }
  // mix in the special effects
  for (int i = 0; i < _numLEDS; i++) {
    if (_leds_char[i]) { // only mix in the effects if the the LED is not black
      _leds[i].r = _leds_char[i].r;// + _leds_effect[i].r;
      _leds[i].g = _leds_char[i].g;// + _leds_effect[i].g;
      _leds[i].b = _leds_char[i].b ;//+ _leds_effect[i].b;
    }
    else {
      _leds[i] = CRGB::Black;
    }
  }
}

void Glyph::updateProgressBar() {
  uint8_t _gHue = 0;
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( _leds, _numLEDS, 20);
  //int pos = beatsin16(3, 0, (LEDS_PER_SEGMENT - 1)) + LEDS_PER_SEGMENT * 5;
  int display_pos = beatsin16(15, 0, LEDS_PER_SEGMENT*4-1);
  if ((3-_glyphID) == floor(display_pos/LEDS_PER_SEGMENT)) {
    int pos;
    pos = display_pos - ((3-_glyphID) * LEDS_PER_SEGMENT) + (LEDS_PER_SEGMENT * 5);
    _leds[pos] += CHSV( _gHue, 255, 192);
    pos = display_pos - ((3-_glyphID) * LEDS_PER_SEGMENT) + (LEDS_PER_SEGMENT * 1);
    _leds[pos] += CHSV( _gHue, 255, 192);
    pos = display_pos - ((3-_glyphID) * LEDS_PER_SEGMENT) + (LEDS_PER_SEGMENT * 4);
    _leds[pos] += CHSV( _gHue, 255, 192);
  }
}

void Glyph::updateProgressBarComplete() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( _leds, _numLEDS, 20);
  int display_pos = beatsin16(20, 0, LEDS_PER_SEGMENT*4-1+4*2);
  if ((3-_glyphID) == floor(display_pos/(LEDS_PER_SEGMENT+2))) {
    int glyph_pos = display_pos - ((3-_glyphID) * (LEDS_PER_SEGMENT+2));
    if(glyph_pos == LEDS_PER_SEGMENT+1) {
      fill_solid(_leds+LEDS_PER_SEGMENT*6,LEDS_PER_SEGMENT, _colorCurrent);
      fill_solid(_leds+LEDS_PER_SEGMENT*2,LEDS_PER_SEGMENT, _colorCurrent);
    }
    else if(glyph_pos == 0) {
      fill_solid(_leds+LEDS_PER_SEGMENT*0,LEDS_PER_SEGMENT, _colorCurrent);
      fill_solid(_leds+LEDS_PER_SEGMENT*3,LEDS_PER_SEGMENT, _colorCurrent);
    }
    else if (glyph_pos > 0 && glyph_pos < (LEDS_PER_SEGMENT + 1)) {
      int pos = glyph_pos + (LEDS_PER_SEGMENT * 5) - 1;
      _leds[pos] += _colorCurrent;
      pos = glyph_pos + (LEDS_PER_SEGMENT * 1) - 1;
      _leds[pos] += _colorCurrent;
      pos = glyph_pos + (LEDS_PER_SEGMENT * 4) - 1;
      _leds[pos] += _colorCurrent;
    }
  }
}

void Glyph::sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  // FIXME _gHue needs to be coming from DisplayDriver.
  int horizontal_effect_size = 2*LEDS_PER_SEGMENT + 3;
  fadeToBlackBy( _effectMapperVertical, horizontal_effect_size, 30);
  int pos = beatsin16(4, 0, horizontal_effect_size - 1);
  _effectMapperVertical[pos] = _colorCurrent;
  _effectMapperVertical[pos].nscale8(30);
  mapHorizontalEffect();
}

void Glyph::mapHorizontalEffect() {
  for (int pos=0; pos<2*LEDS_PER_SEGMENT + 3; pos++) {
    if (pos==0) { // fill segement 5 aka bottom
      fill_solid(_leds_effect+LEDS_PER_SEGMENT*5,LEDS_PER_SEGMENT, _effectMapperVertical[pos]);
    }
    else if (pos<=LEDS_PER_SEGMENT) { // fill 1 pixel each of segements 0 and 6
      _leds_effect[0 * LEDS_PER_SEGMENT + pos - 1] = _effectMapperVertical[pos];
      _leds_effect[6 * LEDS_PER_SEGMENT + pos - 1] = _effectMapperVertical[pos];
    }
    else if (pos==LEDS_PER_SEGMENT+1) { // fill segement 1 aka middle
      fill_solid(_leds_effect+LEDS_PER_SEGMENT*1,LEDS_PER_SEGMENT, _effectMapperVertical[pos]);
    }
    else if (pos<=(2 * LEDS_PER_SEGMENT) + 1) { // fill 1 pixel each of segements 0 and 6
      int local_pos = pos - (LEDS_PER_SEGMENT + 1);
      _leds_effect[2 * LEDS_PER_SEGMENT + local_pos - 1] = _effectMapperVertical[pos];
      _leds_effect[3 * LEDS_PER_SEGMENT + local_pos - 1] = _effectMapperVertical[pos];
    }
    else { //fill segement 4 aka top
      fill_solid(_leds_effect+LEDS_PER_SEGMENT*4,LEDS_PER_SEGMENT, _effectMapperVertical[pos]);
    }
  }
}
