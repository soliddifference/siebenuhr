/*  Segment.cpp -
	v0.1 tg 20180119 initial setup

  Class to handle one glyph of the 7clock

*/
#include <Arduino.h>
#include "Glyph.h"

Glyph::Glyph(int leds_per_segment) :
      glyph_next(new int[SEGMENT_COUNT*leds_per_segment]),
      glyph_current(new int[SEGMENT_COUNT*leds_per_segment]),
      _leds(new CRGB[SEGMENT_COUNT*leds_per_segment]),
      _leds_char(new CRGB[SEGMENT_COUNT*leds_per_segment]),
      _leds_effect(new CRGB[SEGMENT_COUNT*leds_per_segment]),
      LEDS_PER_SEGMENT(leds_per_segment),
      _effect_mapper_vertical(new CRGB[2*leds_per_segment+3]) {
   //constructor
   last_update = millis();
   // total leds per glyph
   NUM_LEDS = SEGMENT_COUNT * leds_per_segment;
}

Glyph::~Glyph () {
  // destructor
}

void Glyph::attach(int data_pin_in, int glyph_id_in) {
  _glyph_id = glyph_id_in;
  if(data_pin_in == 0) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_0>(_leds, NUM_LEDS);
  }
  else if(data_pin_in == 1) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_1>(_leds, NUM_LEDS);
  }
  else if(data_pin_in == 2) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_2>(_leds, NUM_LEDS);
  }
  else if(data_pin_in == 3) {
    FastLED.addLeds<NEOPIXEL, DATA_PIN_3>(_leds, NUM_LEDS);
  }
}

void Glyph::attach(int glyph_id_in) {
  _glyph_id = glyph_id_in;
  _glyph_offset = (3-_glyph_id)*SEGMENT_COUNT*LEDS_PER_SEGMENT;
}

void Glyph::set_color(int r, int g, int b) {
  color_current.r = r;
  color_current.g = g;
  color_current.b = b;
}

void Glyph::set_color(const struct CRGB& color) {
  color_current = color;
}

void Glyph::set_color(const struct CHSV& color) {
  color_current = color;
}

// void Glyph::set_next_digit(int digit, int fade_interval) {
//     glyph_change_blending_period_started = millis();
//     glyph_change_blending_period = fade_interval;
//     for (int i=0; i<SEGMENT_COUNT; i++) {
//       for(int j=0; j<LEDS_PER_SEGMENT; j++) {
//         glyph_current[i*LEDS_PER_SEGMENT+j] = glyph_next[i*LEDS_PER_SEGMENT+j];
//         glyph_next[i*LEDS_PER_SEGMENT+j] = DIGIT[digit][i];
//       }
//     }
// }

void Glyph::set_next_char(char character, int fade_interval) {
    int character_ascii_code = character;
    glyph_change_blending_period_started = millis();
    glyph_change_blending_period = fade_interval;
    for (int i=0; i<SEGMENT_COUNT; i++) {
      for(int j=0; j<LEDS_PER_SEGMENT; j++) {
        glyph_current[i*LEDS_PER_SEGMENT+j] = glyph_next[i*LEDS_PER_SEGMENT+j];
        glyph_next[i*LEDS_PER_SEGMENT+j] = ASCII_TABLE[character_ascii_code][i];
      }
    }
}

void Glyph::set_next_color(CRGB color, int fade_interval_ms) {
    color_change_blending_period_started = millis();
    color_change_blending_period = fade_interval_ms;
    //Serial.println(fade_interval_ms);
    color_base = color_current;
    color_next = color;
}

void Glyph::set_next_color(CHSV color, int fade_interval_ms) {

    color_change_blending_period_started = millis();
    color_change_blending_period = fade_interval_ms;
    color_base = color_current;
    color_next = color;
}

void Glyph::update_blending_to_next_color() {
  unsigned long now = millis();
  // Serial.print("now: ");
  // Serial.println(now);
  // Serial.print("bps: ");
  // Serial.println(color_change_blending_period_started);
  // Serial.print("ccp: ");
  // Serial.println(color_change_blending_period);

  if(now - color_change_blending_period_started  < color_change_blending_period) {
    int itterator = floor((now - color_change_blending_period_started)/DISPLAY_REFRESH_INTERVAL);
    itterator = floor(255*(float)itterator/((float)color_change_blending_period/(float)DISPLAY_REFRESH_INTERVAL));
    color_current.r = lerp8by8(color_base.r, color_next.r, itterator );
    color_current.g = lerp8by8(color_base.g, color_next.g, itterator );
    color_current.b = lerp8by8(color_base.b, color_next.b, itterator );
  }
}

void Glyph::update_daylight_color() {
  //int huhu = hour();
}

void Glyph::update() {
  unsigned long now = millis();
  if(now - glyph_change_blending_period_started  < glyph_change_blending_period ) {
    int itterator = floor((now - glyph_change_blending_period_started)/DISPLAY_REFRESH_INTERVAL);
    itterator = floor(255*(float)itterator/((float)glyph_change_blending_period/(float)DISPLAY_REFRESH_INTERVAL));
    for (int i = 0; i < NUM_LEDS; i++) {
      _leds_char[i].r = lerp8by8(glyph_current[i]*color_current.r, glyph_next[i]*color_current.r, itterator );
      _leds_char[i].g = lerp8by8(glyph_current[i]*color_current.g, glyph_next[i]*color_current.g, itterator );
      _leds_char[i].b = lerp8by8(glyph_current[i]*color_current.b, glyph_next[i]*color_current.b, itterator );
      // if (i==6*LEDS_PER_SEGMENT+2 && _glyph_id == 1 ) {
      //   debug_color(_leds_char[i]);
      // }
    }
  }
  else {
    for (int i = 0; i < NUM_LEDS; i++) {
      _leds_char[i].r = glyph_next[i]*color_current.r;
      _leds_char[i].g = glyph_next[i]*color_current.g;
      _leds_char[i].b = glyph_next[i]*color_current.b;
    }
  }
  // mix in the special effects
  for (int i = 0; i < NUM_LEDS; i++) {
    if (_leds_char[i]) { // only mix in the effects if the the LED is not black
      _leds[i].r = _leds_char[i].r;// + _leds_effect[i].r;
      _leds[i].g = _leds_char[i].g;// + _leds_effect[i].g;
      _leds[i].b = _leds_char[i].b ;//+ _leds_effect[i].b;
    }
    else {
      _leds[i] = CRGB::Black;
    }
      // _leds[i].r = _leds_char[i].r;
      // _leds[i].g = _leds_char[i].g;
      // _leds[i].b = _leds_char[i].b;

      // if(i==13) {
      //   Serial.print(_leds_char[i].r);
      //   Serial.print(" ");
      //   Serial.print(_leds_char[i].g);
      //   Serial.print(" ");
      //   Serial.print(_leds_char[i].b);
      //   Serial.print(" -- ");
      //   Serial.print(_leds_effect[i].r);
      //   Serial.print(" ");
      //   Serial.print(_leds_effect[i].g);
      //   Serial.print(" ");
      //   Serial.println(_leds_effect[i].b);
      // }

  }
}

void Glyph::update_progress_bar()
{
  uint8_t _gHue = 0;
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( _leds, NUM_LEDS, 20);
  //int pos = beatsin16(3, 0, (LEDS_PER_SEGMENT - 1)) + LEDS_PER_SEGMENT * 5;
  int display_pos = beatsin16(15, 0, LEDS_PER_SEGMENT*4-1);
  if ((3-_glyph_id) == floor(display_pos/LEDS_PER_SEGMENT)) {
    int pos;
    pos = display_pos - ((3-_glyph_id) * LEDS_PER_SEGMENT) + (LEDS_PER_SEGMENT * 5);
    _leds[pos] += CHSV( _gHue, 255, 192);
    pos = display_pos - ((3-_glyph_id) * LEDS_PER_SEGMENT) + (LEDS_PER_SEGMENT * 1);
    _leds[pos] += CHSV( _gHue, 255, 192);
    pos = display_pos - ((3-_glyph_id) * LEDS_PER_SEGMENT) + (LEDS_PER_SEGMENT * 4);
    _leds[pos] += CHSV( _gHue, 255, 192);
  }
}

void Glyph::update_progress_bar_complete()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( _leds, NUM_LEDS, 20);
  int display_pos = beatsin16(20, 0, LEDS_PER_SEGMENT*4-1+4*2);
  if ((3-_glyph_id) == floor(display_pos/(LEDS_PER_SEGMENT+2))) {
    int glyph_pos = display_pos - ((3-_glyph_id) * (LEDS_PER_SEGMENT+2));
    // Serial.print (_glyph_id);
    // Serial.print (" ");
    // Serial.print (display_pos);
    // Serial.print (" ");
    // Serial.println (glyph_pos);
    if(glyph_pos == LEDS_PER_SEGMENT+1) {
      fill_solid(_leds+LEDS_PER_SEGMENT*6,LEDS_PER_SEGMENT, color_current);
      fill_solid(_leds+LEDS_PER_SEGMENT*2,LEDS_PER_SEGMENT, color_current);
    }
    else if(glyph_pos == 0) {
      fill_solid(_leds+LEDS_PER_SEGMENT*0,LEDS_PER_SEGMENT, color_current);
      fill_solid(_leds+LEDS_PER_SEGMENT*3,LEDS_PER_SEGMENT, color_current);
    }
    else if (glyph_pos > 0 && glyph_pos < (LEDS_PER_SEGMENT + 1)) {
      int pos = glyph_pos + (LEDS_PER_SEGMENT * 5) - 1;
      _leds[pos] += color_current;
      pos = glyph_pos + (LEDS_PER_SEGMENT * 1) - 1;
      _leds[pos] += color_current;
      pos = glyph_pos + (LEDS_PER_SEGMENT * 4) - 1;
      _leds[pos] += color_current;
    }
  }
}

void Glyph::sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  // FIXME _gHue needs to be coming from DisplayDriver.
  int horizontal_effect_size = 2*LEDS_PER_SEGMENT + 3;
  fadeToBlackBy( _effect_mapper_vertical, horizontal_effect_size, 30);
  int pos = beatsin16(4, 0, horizontal_effect_size - 1);
  _effect_mapper_vertical[pos] = color_current;
  _effect_mapper_vertical[pos].nscale8(30);
  //_effect_mapper_vertical[pos] -= color_current;
  //.maximizeBrightness(30);
  //_effect_mapper_vertical[pos] = color_current + CHSV( 0, 0, 20);
  map_horizontal_effect();
}

void Glyph::map_horizontal_effect() {
  for (int pos=0; pos<2*LEDS_PER_SEGMENT + 3; pos++) {
    if (pos==0) { // fill segement 5 aka bottom
      fill_solid(_leds_effect+LEDS_PER_SEGMENT*5,LEDS_PER_SEGMENT, _effect_mapper_vertical[pos]);
    }
    else if (pos<=LEDS_PER_SEGMENT) { // fill 1 pixel each of segements 0 and 6
      _leds_effect[0 * LEDS_PER_SEGMENT + pos - 1] = _effect_mapper_vertical[pos];
      _leds_effect[6 * LEDS_PER_SEGMENT + pos - 1] = _effect_mapper_vertical[pos];
    }
    else if (pos==LEDS_PER_SEGMENT+1) { // fill segement 1 aka middle
      fill_solid(_leds_effect+LEDS_PER_SEGMENT*1,LEDS_PER_SEGMENT, _effect_mapper_vertical[pos]);
    }
    else if (pos<=(2 * LEDS_PER_SEGMENT) + 1) { // fill 1 pixel each of segements 0 and 6
      int local_pos = pos - (LEDS_PER_SEGMENT + 1);
      _leds_effect[2 * LEDS_PER_SEGMENT + local_pos - 1] = _effect_mapper_vertical[pos];
      _leds_effect[3 * LEDS_PER_SEGMENT + local_pos - 1] = _effect_mapper_vertical[pos];
    }
    else { //fill segement 4 aka top
      fill_solid(_leds_effect+LEDS_PER_SEGMENT*4,LEDS_PER_SEGMENT, _effect_mapper_vertical[pos]);
    }
  }
}
