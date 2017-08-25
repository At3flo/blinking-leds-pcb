
#include <Arduino.h>
#include <FastLED.h>
#include <Button.h>

/* ========================================================================= */

#define REDPIN 6
#define BLUEPIN 5
#define GREENPIN 3

#define CFG_BTN_PIN 8

#define CFG_BTN_IS_PULLUP true
#define CFG_BTN_INVERT true
#define CFG_BTN_DEBOUNCE_DURATION 25 /* MS */

#define CFG_CHECK_DURATION 250 /* MS */

#define CFG_MIN_ACC 1
#define CFG_MAX_ACC 20
#define CFG_MIN_MOD 1
#define CFG_MAX_MOD 20

/* ========================================================================= */

CHSV acc(10, 10, 10);
CHSV speed(0, 0, 0);
CHSV mod(1, 1, 1);
CHSV led(0, 255, 255);

bool dir_delay = true;
uint8_t delay = 0;
uint8_t acc_delay = 0;
uint8_t speed_delay = 0;
uint8_t mod_delay = 0;

bool hdir = true;
bool vdir = true;
bool sdir = true;

uint8_t count = 0;

enum FeaturesEnum
{
  None = 0,
  ChangeHue = 1,
  ChangeSat = 2,
  ChangeVal = 4,
  FollowSin = 8,
  Randomize = 16
};

enum ModeEnum
{
    ManualColor = 0,
    ManualColorEditing = 1,
    Rainbow = 2,
    Crazy = 3
};

ModeEnum mode = ManualColor;
FeaturesEnum features = None;

Button btn(CFG_BTN_PIN, CFG_BTN_IS_PULLUP, CFG_BTN_INVERT, CFG_BTN_DEBOUNCE_DURATION);

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


void show(const CHSV& hsv, uint16_t duration = 0)
{
  CRGB rgb;

  hsv2rgb_rainbow(hsv, rgb);

  show(rgb, duration);
}

void check()
{
  show(CRGB::White, CFG_CHECK_DURATION);
  show(CRGB::Red, CFG_CHECK_DURATION);
  show(CRGB::Blue, CFG_CHECK_DURATION);
  show(CRGB::Green, CFG_CHECK_DURATION);
  show(CRGB::Black, CFG_CHECK_DURATION);
}


void cursor(const uint8_t acc, const uint8_t mod,
  uint8_t &speed, uint8_t &position, bool &dir, bool reverse)
{
  if ((count % mod) == 0)
  {
    uint8_t delta = 1;

    if ((features & FollowSin) != 0)
    {
      delta = sin((float)map(speed, 0, 255, 0, 360) * M_PI / 180.0) * acc;
      speed++;
    }

    if (reverse && (position + delta) < position)
    {
      dir = !dir;
    }

    if (dir)
    {
      position += delta;
    }
    else
    {
      position -= delta;
    }
  }
}

void updateDisplay()
{
  if ((features & ChangeDelay) != 0)
  {
    cursor(acc_delay, mod_delay speed_delay, delay, dir_delay, true);
  }

  show(led, 5);

  if ((features & ChangeHue) != 0)
  {
    cursor(acc.h, mod.h, speed.h, led.h, hdir, false);
  }

  if ((features & ChangeVal) != 0)
  {
    cursor(acc.v, mod.v, speed.v, led.v, vdir, true);
  }

  if ((features & ChangeSat) != 0)
  {
    cursor(acc.s, mod.s, speed.s, led.s, sdir, true);
  }

  count++;
}

void randomize()
{
  acc.h = random(CFG_MIN_ACC, CFG_MAX_ACC);
  acc.v = random(CFG_MIN_ACC, CFG_MAX_ACC);
  acc.s = random(CFG_MIN_ACC, CFG_MAX_ACC);
  mod.h = random(CFG_MIN_MOD, CFG_MAX_MOD);
  mod.v = random(CFG_MIN_MOD, CFG_MAX_MOD);
  mod.s = random(CFG_MIN_MOD, CFG_MAX_MOD);
}

void setMode(ModeEnum newMode)
{
  switch(newMode)
  {
      case ManualColorEditing:
        led.s = 255;
        led.v = 255;
        features = None;
        break;
      case ManualColor:
        if (mode != ManualColorEditing)
        {
          led.s = 255;
          led.v = 255;
        }
        features = ChangeVal | FollowSin;
        break;
      case Rainbow:
        led.s = 255;
        led.v = 255;
        features = ChangeHue | Randomize;
        break;
      case Crazy:
        features = ChangeHue | ChangeSat | ChangeVal | FollowSin | Randomize;
        break;
  };

  mode = newMode;
}


void setup()
{
  pinMode(REDPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);

  pinMode(CFG_BTN_PIN, INPUT_PULLUP);

  randomSeed(analogRead(0));

  check();

  setMode(ManualColor);
}

void loop()
{
  btn.read();

  if (btn.isPressed())
  {
    if (mode == ManualColorEditing || mode == ManualColor && btn.pressedFor(500))
    {
      setMode(ManualColorEditing);
      led.h++;

      show(CRGB::Black, 5);
      show(led, 45);
    }
    else
    {
      show(CRGB::Red, 5);
    }
  }
  else if (btn.wasReleased())
  {
    switch(mode)
    {
        case ManualColor:
          setMode(Rainbow);
          break;
        case ManualColorEditing:
          setMode(ManualColor);
          break;
        case Rainbow:
          setMode(Crazy);
          break;
        case Crazy:
          setMode(ManualColor);
          break;
    };
  }
  else
  {
    if ((mode & Randomize) != 0)
    {
      randomize();
    }

    updateDisplay();
  }
}
