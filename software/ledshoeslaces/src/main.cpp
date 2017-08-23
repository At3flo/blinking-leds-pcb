
#include <Arduino.h>
#include <FastLED.h>

#define REDPIN 6
#define BLUEPIN 5
#define GREENPIN 3

/**
 * This utility method provide display on leds.
 */
void show(const CRGB& rgb, uint16_t duration = 0)
{
  analogWrite(REDPIN, rgb.r);
  analogWrite(GREENPIN, rgb.g);
  analogWrite(BLUEPIN, rgb.b);

  if (duration >= 0)
  {
    delay(duration);
  }
}

void check()
{
  show(CRGB::Red, 500);
  show(CRGB::Blue, 500);
  show(CRGB::Green, 500);
  show(CRGB::Black, 500);
}

void setup()
{
  pinMode(REDPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);

  check();
}

uint8_t hue = 0;
uint8_t value = 0;

void loop()
{
  show(CHSV(hue, 255, value), 5);
  hue++;
  value+=10;
}
