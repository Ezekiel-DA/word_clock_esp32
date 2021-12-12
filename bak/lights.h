#pragma once

#include <Arduino.h>
#include <FastLED.h>

// TODO
// * It would likely make sense, and make this code simpler, to separate conceptual lights and physical light controllers

/**
 * Data structure for serialization of a light's settings
 */
struct LightDataBlock {
  uint8_t cycleColor : 1;
  uint8_t patternID : 7; // max 128 patterns, wich should be more than fine!
  byte hue;
  byte saturation;
};

// ----------------------------------------------------------------
// ILight interface to be used to refer to all lights
// ----------------------------------------------------------------
class ILight
{
protected:
  byte _selectedPatternID = 0;

public:
  bool _cycleColor = false;

  byte _hue = 0;
  byte _saturation = 0;
  byte _val = 0;

  byte getSelectedPattern();

  /**
   * Switch to the next pattern in the available patterns. To be implemented by derived class based on how they actually implement patterns!
   */
  virtual void nextPattern() = 0;

  /**
   * Perform some sort of update step for this light.
   * Returns true if the upate actually changed any state. Callers should do with that information what they will for performance reasons!
   * 
   * Derived classes should call their base class (which will call it's own, etc.) and check the returned value.
   */
  virtual bool update() = 0;

  /**
   * 
   */
  virtual void setup() = 0;

  /**
   * Pulse light briefly, to indicate it is in programming mode. This is a BLOCKING operation by design, to simplify having to deal with existing patterns.
   * Each subclass is in charge of implementing some way to do this and not interfere with anything.
   */
  virtual void pulse() = 0;

  void serialize(LightDataBlock* ioDataBlock);
  void deserialize(LightDataBlock* iDataBlock);
};

// ----------------------------------------------------------------
// Base light implementation with color cycle handling, if needed
// ----------------------------------------------------------------
template <bool colorSupport>
class Light : public ILight
{
private:
  uint16_t _lastColorCycleUpdate = 0;

public:
  virtual bool update();
  virtual void setup();
};

template <>
bool Light<true>::update();

template <>
void Light<true>::setup();

// ----------------------------------------------------------------
// Pattern light implementation
// ----------------------------------------------------------------
template <bool colorSupport>
class PatternLight : public Light<colorSupport>
{
protected:
  byte _patternStep = 0;
  byte _prevPatternID = 0;
  uint16_t _lastLightUpdate = 0;

public:
  virtual void nextPattern();

  virtual bool update();

  virtual void setup();
};

// ----------------------------------------------------------------
// A PatternLight on one or more WS2812B LED string (with color)
// Support up to three strips; strips can be of different lengths.
// See constructor for hackish details of multiple strip support.
//
// TODO: handling of multiple strips is pretty hackish and gross, but
// at this point I don't want to clean it up for MW3. Maybe for MW4!
//
// ----------------------------------------------------------------
template <int dataPin1>
class PatternLightLEDStrip : public PatternLight<true>
{
  int _numLEDs1;
  CRGB *_leds1 = nullptr;

  byte _maxBrightness = 255;

public:
  // Number of LEDs in the strip is optional if strips 2 and 3 are present. For any of strip 2 or 3 where the number of LEDs is not specified, two things will happen:
  // - the number of LEDs from strip 1 will be used
  // - the buffer for strip 1 will be reused
  PatternLightLEDStrip(int numLEDs1) : _numLEDs1(numLEDs1) {};

  void setMaxBrightness(byte maxBrightness);

  virtual void nextPattern();

  void setup();

  bool update();

  void pulse();
};

// ----------------------------------------------------------------
// A PatternLight on a PWM port (no color)
// ----------------------------------------------------------------
class PatternLightPWMPort : public PatternLight<false>
{
  int _pin;

public:
  PatternLightPWMPort(int pin);

  void setup();

  bool update();

  void pulse();
};

// ----------------------------------------------------------------
// A PatternLight on a digital port (no color, no PWM)
// BROKEN, DO NOT USE: need to implement a way to eliminate incompatible patterns, otherwise we'll cycle through a bunch of indistinguishable bs.
// ----------------------------------------------------------------
class PatternLightDigitalPort : public PatternLight<false>
{
  int _pin;

public:
  PatternLightDigitalPort(int pin);

  void setup();

  bool update();

  void pulse();
};


// ----------------------------------------------------------------
// A type of fairy lights that come with their own controllers
// We'll just use a data pin to simulate clicking the button.
// ----------------------------------------------------------------
class FairyLightsController : public ILight
{
  int _pin;
  byte _numPatternsAvailable;
  byte _offPatternID;
  byte _onPatternID;

  byte _prevPatternID = 0;

  byte patternDistance(byte currentPattern, byte desiredPattern);

  void clickFairyLights(byte numClicks);

public:
  FairyLightsController(int pin, byte numPatternsAvailable=9, byte offPatternID=0, byte onPatternID=8);

  virtual bool update();

  virtual void setup();

  // NB this probably needs to move to some base class shared with PatternLight but I can't be bothered right now
  virtual void nextPattern();

  void pulse();
};
