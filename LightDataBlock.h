#pragma once

/**
 * Data structure for serialization of a light's settings
 */
struct LightDataBlock {
  uint8_t cycleColor : 1;
  uint8_t patternID : 7; // max 128 patterns, wich should be more than fine!
  uint8_t hue;
  uint8_t saturation;
};
