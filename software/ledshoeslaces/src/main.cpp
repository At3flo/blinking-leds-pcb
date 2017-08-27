#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>

enum ButtonStateEnum : uint8_t
{
  Unknown,
  Up,
  Falling,
  Down,
  Rising
};

enum StateEnum : uint8_t
{
  Sleep,
  Solid,
  FadingInc,
  FadingDec,
  Rainbow
};

#define CFG_RED_PIN 1
#define CFG_BLUE_PIN 3
#define CFG_GREEN_PIN 4
#define CFG_BTN_PIN 0
#define STEP 10U

StateEnum state = FadingInc;

uint8_t hue = 0;
uint8_t value = 0;

ButtonStateEnum buttonState = Unknown;
uint32_t buttonChanged = 0;
uint32_t timeRef = 0;


//This runs each time the watch dog wakes us up from sleep
ISR(WDT_vect) {
  //Don't do anything. This is just here so that we wake up.
}

//Sets the watchdog timer to wake us up, but not reset
//0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms
//6=1sec, 7=2sec, 8=4sec, 9=8sec
//From: http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void setup_watchdog(int timerPrescaler) {

  if (timerPrescaler > 9 ) timerPrescaler = 9; //Limit incoming amount to legal settings

  byte bb = timerPrescaler & 7;
  if (timerPrescaler > 7) bb |= (1<<5); //Set the special 5th bit if necessary

  //This order of commands is important and cannot be combined
  MCUSR &= ~(1<<WDRF); //Clear the watch dog reset
  WDTCR |= (1<<WDCE) | (1<<WDE); //Set WD_change enable, set WD enable
  WDTCR = bb; //Set new watchdog timeout value
  WDTCR |= _BV(WDIE); //Set the interrupt enable, this will keep unit from resetting after each int
}

void setup()
{
    pinMode(CFG_RED_PIN, OUTPUT);
    pinMode(CFG_BLUE_PIN, OUTPUT);
    pinMode(CFG_GREEN_PIN, OUTPUT);
    pinMode(CFG_BTN_PIN, INPUT_PULLUP);

    ADCSRA &= ~_BV(ADEN);  // switch ADC OFF
    ACSR  |= _BV(ACD);     // switch Analog Compartaror OFF

    //Power down various bits of hardware to lower power usage
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); //Power down everything, wake up from WDT
    sleep_enable();

    timeRef = millis();
}

void display()
{
  float r, g, b;
  float h = ((float)hue) / 255.0f;
  float v = ((float)value) / 255.0f;
  float s = 1.0f;

  int i = int(h * 6);
  float f = h * 6.0f - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - f * s);
  float t = v * (1.0f - (1.0f - f) * s);

  switch(i % 6)
  {
      case 0: r = v, g = t, b = p; break;
      case 1: r = q, g = v, b = p; break;
      case 2: r = p, g = v, b = t; break;
      case 3: r = p, g = q, b = v; break;
      case 4: r = t, g = p, b = v; break;
      case 5: r = v, g = p, b = q; break;
  }

  analogWrite(CFG_BLUE_PIN, (uint8_t)(b * 255.0));
  analogWrite(CFG_GREEN_PIN, (uint8_t)(g * 255.0));
  analogWrite(CFG_RED_PIN, (uint8_t)(r * 255));
}

void sleep()
{
  ADCSRA &= ~(1<<ADEN); //Disable ADC, saves ~230uA
  setup_watchdog(6); //Setup watchdog to go off after 1sec
  sleep_mode(); //Go to sleep! Wake up 1sec later
  //Check for water
  ADCSRA |= (1<<ADEN); //Enable ADC
}

void button()
{
  uint32_t now = millis();

  if ((now - buttonChanged) < 25)
  {
    return;
  }

  uint8_t state = digitalRead(CFG_BTN_PIN);
  ButtonStateEnum origState = buttonState;

  switch (buttonState)
  {
    case Unknown:
      buttonState = state == HIGH ? Up : Down;
      break;
    case Up:
      if (state == LOW)
      {
        buttonState = Falling;
      }
      break;
    case Down:
      if (state == HIGH)
      {
        buttonState = Rising;
      }
      break;
    case Falling:
      buttonState = Down;
      break;
    case Rising:
      buttonState = Up;
      break;
  }

  if (buttonState != origState)
  {
    buttonChanged = now;
  }
}

void waitButton(ButtonStateEnum state)
{
  while (state != buttonState)
  {
    button();
  }
}


void loop()
{
  uint32_t now = millis();

  button();

  switch (state)
  {
    case Sleep:
      if (digitalRead(CFG_BTN_PIN) == LOW)
      {
        hue = 0;
        value = 255;
        state = Solid;
        display();
        waitButton(Up);
      }
      else
      {
        value = 0;
        buttonState = Unknown;
        display();
        sleep();
      }
      break;
    case Solid:
      if (buttonState == Falling)
      {
        timeRef = now;
      }
      else if (buttonState == Down)
      {
        if ((now - timeRef) > 500)
        {
          hue++;
        }
      }
      else if (buttonState == Rising)
      {
        if ((now - timeRef) <= 500)
        {
          state = FadingDec;
        }
        waitButton(Up);
      }
      break;
    case FadingInc:
      if (value >= (0xFF - STEP))
      {
        state = FadingDec;
        value = 255;
      }
      else
      {
        value += STEP;
      }
      if (buttonState == Rising)
      {
        state = Rainbow;
        waitButton(Up);
      }
      break;
    case FadingDec:
      if (value <= STEP)
      {
        state = FadingInc;
        value = 0;
      }
      else
      {
        value -= STEP;
      }
      if (buttonState == Rising)
      {
        state = Rainbow;
        waitButton(Up);
      }
      break;
    case Rainbow:
      hue++;
      if (buttonState == Rising)
      {
        state = Sleep;
        waitButton(Up);
      }
      break;
  }

  display();
  delay(5);
}
