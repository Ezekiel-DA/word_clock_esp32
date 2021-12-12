#include "lights.h"
#include "quakeFlicker.h"
#include "pacifica.h"
#include "LED_functions.h"


byte ILight::getSelectedPattern() { return _selectedPatternID; };

void ILight::serialize(LightDataBlock* ioDataBlock)
{
  ioDataBlock->cycleColor = _cycleColor;
  ioDataBlock->patternID = _selectedPatternID;
  ioDataBlock->hue = _hue;
  ioDataBlock->saturation = _saturation;
};

void ILight::deserialize(LightDataBlock* iDataBlock)
{
  _cycleColor = iDataBlock->cycleColor;
  _selectedPatternID = iDataBlock->patternID;
  _hue = iDataBlock->hue;
  _saturation = iDataBlock->saturation;
}

template <bool colorSupport>
bool Light<colorSupport>::update() { return false; };

template <bool colorSupport>
void Light<colorSupport>::setup() {};

template <>
bool Light<true>::update()
{
  uint16_t now = millis();

  if ((uint8_t)(now - _lastColorCycleUpdate) > 20)
  { // don't cycle colors too fast when button is held
    _lastColorCycleUpdate = now;
    if (_cycleColor) {
      ++_hue;
      return true;
    }
  }

  return true;
};

template <>
void Light<true>::setup()
{
  _lastColorCycleUpdate = millis();
};

template <bool colorSupport>
void PatternLight<colorSupport>::nextPattern() {
  this->_selectedPatternID = ++(this->_selectedPatternID) % NUM_LIGHTSTYLES;
};

template <bool colorSupport>
bool PatternLight<colorSupport>::update()
{
  Light<colorSupport>::update(); // safe to ignore whether or not color cycling made any changes; we'll determine if we wanna skip updating on our own, for animation purposes
  uint16_t now = millis();

  if ((uint8_t)(now - _lastLightUpdate) < 16)
  { // no need to update faster than 60FPS
    return false;
  }

  if (this->_selectedPatternID < NUM_LIGHTSTYLES) // don't perform Quake style flicker if we're out of range of those; we'll do Pacifica instead
    this->_val = enhancedQuakeFlicker(this->_lastLightUpdate, this->_selectedPatternID, this->_prevPatternID, this->_patternStep);
  
  return true;
};

template <bool colorSupport>
void PatternLight<colorSupport>::setup()
{
  Light<colorSupport>::setup();
  _lastLightUpdate = millis();
  ;
};

template <int dataPin1>
void PatternLightLEDStrip<dataPin1>::setMaxBrightness(byte maxBrightness)
{
  _maxBrightness = maxBrightness;
};

template <int dataPin1>
void PatternLightLEDStrip<dataPin1>::nextPattern()
{
  this->_selectedPatternID = ++(this->_selectedPatternID) % (NUM_LIGHTSTYLES + 1); // add one to support Pacifica as an additional style, which is not handled by the quakeFlicker code
};

template <int dataPin1>
void PatternLightLEDStrip<dataPin1>::setup()
{
  _leds1 = new CRGB[_numLEDs1];
  FastLED.addLeds<WS2812B, dataPin1, GRB>(_leds1, _numLEDs1).setCorrection(TypicalLEDStrip);
  
  PatternLight::setup();
};

template <int dataPin1>
bool PatternLightLEDStrip<dataPin1>::update()
{
  if (PatternLight::update())
  {
    if (_selectedPatternID < NUM_LIGHTSTYLES)
    {
      byte scaledVal = scale8_video(_val, _maxBrightness);
      setAllLEDs(CHSV(_hue, _saturation, scaledVal), _leds1, _numLEDs1);
    }
    else
    {
      pacifica_loop(_leds1, _numLEDs1);
    }
  }

  return true;
};

template <int dataPin1>
void PatternLightLEDStrip<dataPin1>::pulse()
{
  for (byte i = 0; i < 4; ++i)
  {
    setAllLEDs(CRGB::White, _leds1, _numLEDs1);
    FastLED.show();
    delay(100);
    setAllLEDs(CRGB::Black, _leds1, _numLEDs1);
    FastLED.show();
    delay(100);
  }
};

PatternLightPWMPort::PatternLightPWMPort(int pin) : _pin(pin){};

void PatternLightPWMPort::setup()
{
  pinMode(_pin, OUTPUT);
  analogWrite(_pin, 0);

  PatternLight::setup();
};

bool PatternLightPWMPort::update()
{
  if (PatternLight::update())
  {
    analogWrite(_pin, _val);
  }

  return true;
};

void PatternLightPWMPort::pulse()
{
  for (byte i = 0; i < 4; ++i)
  {
    analogWrite(_pin, 255);
    delay(100);
    analogWrite(_pin, 0);
    delay(100);
  }
};

PatternLightDigitalPort::PatternLightDigitalPort(int pin) : _pin(pin){};

void PatternLightDigitalPort::setup()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);

  PatternLight::setup();
};

bool PatternLightDigitalPort::update()
{
  if (PatternLight::update())
  {
    digitalWrite(_pin, _val);
  }

  return true;
};

void PatternLightDigitalPort::pulse()
{
  for (byte i = 0; i < 4; ++i)
  {
    digitalWrite(_pin, HIGH);
    delay(100);
    digitalWrite(_pin, LOW);
    delay(100);
  }
};

byte FairyLightsController::patternDistance(byte currentPattern, byte desiredPattern)
{
  int distance = desiredPattern - currentPattern;
  return distance < 0 ? distance + _numPatternsAvailable : distance;
};

void FairyLightsController::clickFairyLights(byte numClicks) {
  for (byte i = 0; i < numClicks; ++i) {
    digitalWrite(_pin, LOW);
    delay(40);
    digitalWrite(_pin, HIGH);
    delay(40);
  }
}

FairyLightsController::FairyLightsController(int pin, byte numPatternsAvailable, byte offPatternID, byte onPatternID) : _pin(pin), _numPatternsAvailable(numPatternsAvailable), _offPatternID(offPatternID), _onPatternID(onPatternID) {};

bool FairyLightsController::update()
{
  if (_selectedPatternID != _prevPatternID)
  {
    byte distance = patternDistance(_prevPatternID, _selectedPatternID);
    //Serial.print("updating FL controller, distance between SelectedID (#"); Serial.print(_selectedPatternID); Serial.print(") and previousID (#"); Serial.print(_prevPatternID); Serial.print(") is "); Serial.println(distance);
    clickFairyLights(distance);
    _prevPatternID = _selectedPatternID;
    return true;
  }
  return false;
};

void FairyLightsController::setup()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, HIGH);
};

// NB this probably needs to move to some base class shared with PatternLight but I can't be bothered right now
void FairyLightsController::nextPattern() {
  this->_selectedPatternID = ++(this->_selectedPatternID) % _numPatternsAvailable;
};

void FairyLightsController::pulse()
{
  clickFairyLights(patternDistance(_selectedPatternID, _onPatternID));
  for (byte i = 0; i < 2; ++i)
  {
    delay(500);
    clickFairyLights(patternDistance(_onPatternID, _offPatternID));
    delay(300);
    clickFairyLights(patternDistance(_offPatternID, _onPatternID));
  }
  clickFairyLights(patternDistance(_onPatternID, _selectedPatternID));
};
