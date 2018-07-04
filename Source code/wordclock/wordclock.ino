/* ClockSquared by Willem Pennings - My design of the Word Clock.
 * Utilizes LED strips and now features individual control of letters.
 * Copyright (C) 2017 Willem Pennings (willemm.nl)
 * The FastLED library is required to compile this program!
 */

/* Libraries */
#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include "FastLED.h"

/* Does the LED strip start at the top left letter 'H' or the top right letter 'N'? */
#define START_N         1         // [-]
#define START_H         0         // [-]
                        
FASTLED_USING_NAMESPACE 
                        
/* LED strip and grid definitions */
#define NUM_LEDS        121       // [-] - The amount of lights on the strip
#define DATA_PIN        2         // [-] - The Arduino data output pin
#define CLOCK_PIN       3         // [-] - The Arduino clock output pin
#define LIGHT_PIN       4         // [-] - LED button transistor light switch pin
#define LED_TYPE        APA102    // [-] - The LED strip model
#define COLOR_ORDER     BGR       // [-] - The LED strip color order
CRGB leds[NUM_LEDS];  
                        
/* Input pin definitions */
#define INC_PIN         9         // [-] - The Arduino increment input pin. Has multiple uses
#define DEC_PIN         10        // [-] - The Arduino decrement input pin. Has multiple uses
#define COL_PIN         11        // [-] - The Arduino color input pin. Has multiple uses

/* Time variables */
time_t                  t;        // [s] - Absolute time in seconds

/* HSV colour variables */
int hsv_hue           = 0;        // [-] - Red as standard hue value.
int hsv_sat           = 0;        // [-] - 0 saturation for white colour as default.
int hsv_val           = 255;      // [-] - Full brightness is standard value.

/* General variables */
int fps               = 120;      // [frames/s] - The rendering speed (animations)
int interval          = 1000;     // [ms] - The interval which is mainly used for input debouncing
int brightness        = 128;      // [-] - LED strip brightness on a scale of 0 to 255
int brightvar         = 10;       // [-] - Brightness variable
int frequency_strobe  = 1;        // [Hz] - The frequency of the strobe function
int strobe_delay      = 30;       // [ms] - The strobe pulse time
long buttonTime       = 0;        // [ms] - Time at which the button was pressed
long updateTime       = 0;        // [ms] - Time at which the last time update was done
bool index[20]        = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int i,j               = 0;        // [-] - Help variables, for example in the for(){} functions

uint8_t gCurrentModeNumber = 0;   // [-] - The current mode number
uint8_t gHue = 0;                 // [-] - HSV hue value [0-255] used clock-wide for all animations

void setup() {
  /* Provide a small startup delay to let hardware initialize */
  FastLED.clear();
  delay(1000);
  
  /* Initialize communication protocols */
  Serial.begin(9600);
  Wire.begin();

  /* Initialize pins */
  pinMode(INC_PIN, INPUT);
  pinMode(DEC_PIN, INPUT);
  pinMode(COL_PIN, INPUT);
  
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, HIGH);

  /* Initialize LED strip */
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);

  /* Initialize RTC & set system time */
  setSyncProvider(RTC.get);
  if(timeStatus() != timeSet) {
    Serial.println("Unable to sync with DS3231. Top left LED will blink red for 10 seconds!");
    while(millis() < 10000) {
      leds[10] = CRGB::Red;
      FastLED.show();
    }
  } else {
    Serial.println("RTC has set the system time");
  }

  /* Blink all LEDs in RGB order to make sure colour order is correct and all LEDs work */
  for(i=0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
  delay(500);
  for(i=0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Green;
  }
  FastLED.show();
  delay(500);
  for(i=0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
  delay(500);
  FastLED.clear();

  /* Render the time, given that the startup mode is 1 (normal operation) */
  if(gCurrentModeNumber == 0) {
    displayTime(hsv_hue, hsv_sat, hsv_val);
  }
}

typedef void (*modeList[])();
modeList gModes = {normal, varbright, night, confetti, wave, birthday};

void loop() {
  gModes[gCurrentModeNumber]();
  FastLED.show();
  FastLED.delay(1000/fps);

  /* Increase HSV hue value every 20 milliseconds */
  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }

  /* Go to the next mode if both the increment and decrement pins are pressed */
  if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(1000);
    if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 0)) {
      nextPattern();
    }
  }
}

/* Apparently this makes sense */
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

/* Handles the changing of modes and start conditions when a mode is initialized */
void nextPattern() {
  FastLED.clear(true);
  gCurrentModeNumber = (gCurrentModeNumber + 1) % ARRAY_SIZE(gModes);
  
  if(gCurrentModeNumber == 0 || gCurrentModeNumber == 5) { // Normal or birthday
    FastLED.setBrightness(128);
    digitalWrite(LIGHT_PIN, HIGH);
    fps = 120;
    hsv_sat = 0;
    hsv_hue = 0;
    displayTime(hsv_hue, hsv_sat, hsv_val);
  }
  if(gCurrentModeNumber == 1) { // varbright
    FastLED.setBrightness(10);
    digitalWrite(LIGHT_PIN, LOW);
    fps = 120;
    hsv_sat = 0;
    hsv_hue = 0;
    brightvar = 10;
    displayTime(hsv_hue, hsv_sat, hsv_val);
  }
  if(gCurrentModeNumber == 2) { // Night
    FastLED.setBrightness(2);
    digitalWrite(LIGHT_PIN, LOW);
    fps = 120;
    hsv_sat = 255;
    hsv_hue = 0;
    displayTime(hsv_hue, hsv_sat, hsv_val);
  }
  if(gCurrentModeNumber == 3) { // Confetti
    FastLED.setBrightness(128);
    digitalWrite(LIGHT_PIN, HIGH);
    fps = 120;
  }
  if(gCurrentModeNumber == 4) { // Wave
    digitalWrite(LIGHT_PIN, HIGH);
    FastLED.setBrightness(128);
    fps = 120;
  }
}

void normal() {
  if(millis() - updateTime > 5*interval) {
    displayTime(hsv_hue, hsv_sat, hsv_val);
    updateTime = millis();
  }
  
  /* This code displays a rainbow effect on the birthday words 'Fijne verjaardag' */

  // Some random birthday examples; set any date or amount
  if(((month() == 8) && (day() == 5)) || ((month() == 9) && (day() == 18))) {
    if(START_N) {
      for(i = 38; i >= 34; i--) {
        leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
      }
      for(i = 65; i >= 55; i--) {
        leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
      }
      j+=2;
      if(j >= 255*5) {
        j = 0;
      }
    }
    if(START_H) {
      for(i = 42; i >= 38; i--) {
        leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
      }
      for(i = 65; i >= 55; i--) {
        leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
      }
      j+=2;
      if(j >= 255*5) {
        j = 0;
      }
    }
  }

  /* Wipe the LED matrix once a day to clear birthday messages after the birthday is over */
  if(hour() == 0 && minute() == 1 && second() == 1) {
    FastLED.clear();
  }
  
  /* If the colour button is pressed, set HSV values to change colours and increment hue value */
  if((digitalRead(COL_PIN) == 0) && (millis() - buttonTime > interval*0.05)) {
    buttonTime = millis();
    delay(50);
    if(digitalRead(COL_PIN) == 0) {
      hsv_sat = 255;
      hsv_hue = hsv_hue + 2;
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be incremented */
  if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1)) {   
      adjustTime(-((5 + (minute() % 5)) * 60 + second()) + 600);
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be decremented */
  if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0)) {
      adjustTime(-((5 + (minute() % 5)) * 60 + second()));
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }
}

void varbright() {
  if(millis() - updateTime > 5*interval) {
    displayTime(hsv_hue, hsv_sat, hsv_val);
    updateTime = millis();
  }
  
  /* This code displays a rainbow effect on the birthday words 'Fijne verjaardag' */

  // Some random birthday examples; set any date or amount
  if(((month() == 8) && (day() == 5)) || ((month() == 9) && (day() == 18))) {
    if(START_N) {
      for(i = 38; i >= 34; i--) {
        leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
      }
      for(i = 65; i >= 55; i--) {
        leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
      }
      j+=2;
      if(j >= 255*5) {
        j = 0;
      }
    }
    if(START_H) {
      for(i = 42; i >= 38; i--) {
        leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
      }
      for(i = 65; i >= 55; i--) {
        leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
      }
      j+=2;
      if(j >= 255*5) {
        j = 0;
      }
    }
  }

  /* Wipe the LED matrix once a day to clear birthday messages after the birthday is over */
  if(hour() == 0 && minute() == 1 && second() == 1) {
    FastLED.clear();
  }
  
  /* If the colour button is pressed, set brightness values to change LED brightness */
  if((digitalRead(COL_PIN) == 0) && (millis() - buttonTime > interval*0.05)) {
    buttonTime = millis();
    delay(50);
    if(digitalRead(COL_PIN) == 0) {
      if(brightvar == 128) {
        brightvar = 1;
      } else {
        brightvar++;
      }
      FastLED.setBrightness(brightvar);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be incremented */
  if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1)) {   
      adjustTime(-((5 + (minute() % 5)) * 60 + second()) + 600);
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be decremented */
  if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0)) {
      adjustTime(-((5 + (minute() % 5)) * 60 + second()));
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }
}

void night() {
  if(millis() - updateTime > 5*interval) {
    displayTime(hsv_hue, hsv_sat, hsv_val);
    updateTime = millis();
  }

  /* This code displays a rainbow effect on the birthday words 'Fijne verjaardag' */
  if(((month() == 8) && (day() == 5)) || ((month() == 9) && (day() == 18))) {
    if(START_N) {
      for(i = 38; i >= 34; i--) {
        leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
      }
      for(i = 65; i >= 55; i--) {
        leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
      }
      j+=2;
      if(j >= 255*5) {
        j = 0;
      }
    }
    if(START_H) {
      for(i = 42; i >= 38; i--) {
        leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
      }
      for(i = 65; i >= 55; i--) {
        leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
      }
      j+=2;
      if(j >= 255*5) {
        j = 0;
      }
    }
  }

  /* Wipe the LED matrix once a day to clear birthday messages after the birthday is over */
  if(hour() == 0 && minute() == 1 && second() == 1) {
    FastLED.clear();
  }

  /* Check if the time should be incremented */
  if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1)) {
      adjustTime(-((5 + (minute() % 5)) * 60 + second()) + 600);
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be decremented */
  if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0)) {
      adjustTime(-((5 + (minute() % 5)) * 60 + second()));
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }
}

/* Randomly coloured speckles that blink in and fade smoothly */
void confetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);

  if((digitalRead(COL_PIN) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if(digitalRead(COL_PIN) == 0) {
      if(brightness == 255) {
        brightness = 128;
      } else {
        brightness = brightness + 127;
      }
      FastLED.setBrightness(brightness);
    }
  }
}

/* Cycles through the colour spectrum using a periodic formula */
void wave() {
  for(i = 0; i < 121; i++) {
    leds[i] = CHSV(cubicwave8(gHue), 255, 255);
  }
}

void birthday() {
  if(millis() - updateTime > 5*interval) {
    displayTime(hsv_hue, hsv_sat, hsv_val);
    updateTime = millis();
  }
  
  /* This code displays a rainbow effect on the birthday words 'Fijne verjaardag' */
  if(START_N) {
    for(i = 38; i >= 34; i--) {
      leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
    }
    for(i = 65; i >= 55; i--) {
      leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
    }
    j+=2;
    if(j >= 255*5) {
      j = 0;
    }
  }
  if(START_H) {
    for(i = 42; i >= 38; i--) {
      leds[i] = CHSV((((i * 256 / 5) - j) & 255), 255, 255);
    }
    for(i = 65; i >= 55; i--) {
      leds[i] = CHSV((((i * 256 / 10) - j) & 255), 255, 255);
    }
    j+=2;
    if(j >= 255*5) {
      j = 0;
    }
  }

  /* If the colour button is pressed, set HSV values to change colours and increment hue value */
  if((digitalRead(COL_PIN) == 0) && (millis() - buttonTime > interval*0.05)) {
    buttonTime = millis();
    delay(50);
    if(digitalRead(COL_PIN) == 0) {
      hsv_sat = 255;
      hsv_hue = hsv_hue + 2;
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be incremented */
  if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 0) && (digitalRead(DEC_PIN) == 1)) {   
      adjustTime(-((5 + (minute() % 5)) * 60 + second()) + 600);
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }

  /* Check if the time should be decremented */
  if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    delay(250);
    if((digitalRead(INC_PIN) == 1) && (digitalRead(DEC_PIN) == 0)) {
      adjustTime(-((5 + (minute() % 5)) * 60 + second()));
      t = now();
      RTC.set(t);
      displayTime(hsv_hue, hsv_sat, hsv_val);
    }
  }
}

/* This is the time function that takes the current time, translates
 * it to hours and minutes, and then translates that to a series of
 * LEDs to light up.
 */
void displayTime(int hsv_hue, int hsv_sat, int hsv_val) {
  if(START_N) {
    for(i = 0; i <= 33; i++) {
      leds[i] = CRGB::Black;
    }
    for(i = 39; i <= 54; i++) {
      leds[i] = CRGB::Black;
    }
    for(i = 66; i <= 120; i++) {
      leds[i] = CRGB::Black;
    }
  }
  if(START_H) {
    for(i = 0; i <= 37; i++) {
      leds[i] = CRGB::Black;
    }
    for(i = 43; i <= 54; i++) {
      leds[i] = CRGB::Black;
    }
    for(i = 66; i <= 120; i++) {
      leds[i] = CRGB::Black;
    }
  }
  index[19] = 1; // het is
  
  /* Minutes */
  if((minute() >= 5) && (minute() <= 9)) {
    index[0] = 1; // vijf
    index[5] = 1; // over
  }
  if((minute() >= 10) && (minute() <= 14)) {
    index[1] = 1; // tien
    index[5] = 1; // over
  }
  if((minute() >= 15) && (minute() <= 19)) {
    index[2] = 1; // kwart
    index[5] = 1; // over
  }
  if((minute() >= 20) && (minute() <= 24)) {
    index[1] = 1; // tien
    index[4] = 1; // voor
    index[3] = 1; // half
  }
  if((minute() >= 25) && (minute() <= 29)) {
    index[0] = 1; // vijf
    index[4] = 1; // voor
    index[3] = 1; // half
  }
  if((minute() >= 30) && (minute() <= 34)) {
    index[3] = 1; // half
  }
  if((minute() >= 35) && (minute() <= 39)) {
    index[0] = 1; // vijf
    index[5] = 1; // over
    index[3] = 1; // half
  }
  if((minute() >= 40) && (minute() <= 44)) {
    index[1] = 1; // tien
    index[5] = 1; // over
    index[3] = 1; // half
  }
  if((minute() >= 45) && (minute() <= 49)) {
    index[2] = 1; // kwart
    index[4] = 1; // voor
  }
  if((minute() >= 50) && (minute() <= 54)) {
    index[1] = 1; // tien
    index[4] = 1; // voor
  }
  if((minute() >= 55) && (minute() <= 59)) {
    index[0] = 1; // vijf
    index[4] = 1; // voor
  }

  /* "It's <hour> o'clock" */
  if((minute() < 5) && (hourFormat12() == 1)) {
    index[6]  = 1; // een
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 2)) {
    index[7]  = 1; // twee
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 3)) {
    index[8]  = 1; // drie
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 4)) {
    index[9]  = 1; // vier
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 5)) {
    index[10] = 1; // vijf
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 6)) {
    index[11] = 1; // zes
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 7)) {
    index[12] = 1; // zeven
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 8)) {
    index[13] = 1; // acht
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 9)) {
    index[14] = 1; // negen
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 10)) {
    index[15] = 1; // tien
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 11)) {
    index[16] = 1; // elf
    index[18] = 1; // uur
  }
  if((minute() < 5) && (hourFormat12() == 12)) {
    index[17] = 1; // twaalf
    index[18] = 1; // uur
  }

  /* Choose the hour when minute() < 20 */
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 1)) {
    index[6]  = 1; // een
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 2)) {
    index[7]  = 1; // twee
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 3)) {
    index[8]  = 1; // drie
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 4)) {
    index[9]  = 1; // vier
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 5)) {
    index[10] = 1; // vijf
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 6)) {
    index[11] = 1; // zes
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 7)) {
    index[12] = 1; // zeven
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 8)) {
    index[13] = 1; // acht
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 9)) {
    index[14] = 1; // negen
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 10)) {
    index[15] = 1; // tien
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 11)) {
    index[16] = 1; // elf
  }
  if((minute() >= 5) && (minute() <= 19) && (hourFormat12() == 12)) {
    index[17] = 1; // twaalf
  }

  /* Choose the hour when minute() >= 20 */
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 1)) {
    index[7] = 1; // twee
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 2)) {
    index[8]  = 1; // drie
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 3)) {
    index[9]  = 1; // vier
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 4)) {
    index[10] = 1; // vijf
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 5)) {
    index[11] = 1; // zes
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 6)) {
    index[12] = 1; // zeven
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 7)) {
    index[13] = 1; // acht
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 8)) {
    index[14] = 1; // negen
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 9)) {
    index[15] = 1; // tien
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 10)) {
    index[16] = 1; // elf
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 11)) {
    index[17] = 1; // twaalf
  }
  if((minute() >= 20) && (minute() <= 59) && (hourFormat12() == 12)) {
    index[6]  = 1; // een
  }

  /* Check the index number and activative the corresponding LEDs.
   * The function also checks whether the hardware starts at top-left 'H' or top-right 'N'.
   * The index array is reset after the time has been displayed, allowing a new iteration.
   */
  if(index[0] == 1) {
    if(START_N) {
      for(i = 18; i <= 21; i++) {
          leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // vijf
      }
    }
    if(START_H) {
      for(i = 11; i <= 14; i++) {
          leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // vijf
      }
    }
  }
  if(index[1] == 1) {
    if(START_N) {
      for(i = 0; i <= 3; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // tien
      }
    }
    if(START_H) {
      for(i = 7; i <= 10; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // tien
      }
    }
  }
  if(index[2] == 1) {
    if(START_N) {
      for(i = 12; i <= 16; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // kwart
      }
    }
    if(START_H) {
      for(i = 16; i <= 20; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // kwart
      }
    }
  }
  if(index[3] == 1) {
    if(START_N) {
      for(i = 40; i <= 43; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // half
      }
    }
    if(START_H) {
      for(i = 33; i <= 36; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // half
      }
    }
  }
  if(index[4] == 1) {
    if(START_N) {
      for(i = 29; i <= 32; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // voor
      }
    }
    if(START_H) {
      for(i = 22; i <= 25; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // voor
      }
    }
  }
  if(index[5] == 1) {
    if(START_N) {
      for(i = 23; i <= 26; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // over
      }
    }
    if(START_H) {
      for(i = 28; i <= 31; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // over
      }
    }
  }
  if(index[6] == 1) {
    if(START_N) {
      for(i = 74; i <= 76; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // een
      }
    }
    if(START_H) {
      for(i = 66; i <= 68; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // een
      }
    }
  }
  if(index[7] == 1) {
    if(START_N) {
      for(i = 50; i <= 53; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // twee
      }
    }
    if(START_H) {
      for(i = 45; i <= 48; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // twee
      }
    }
  }
  if(index[8] == 1) {
    if(START_N) {
      for(i = 95; i <= 98; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // drie
      }
    }
    if(START_H) {
      for(i = 88; i <= 91; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // drie
      }
    }
  }
  if(index[9] == 1) {
    if(START_N) {
      for(i = 106; i <= 109; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // vier
      }
    }
    if(START_H) {
      for(i = 99; i <= 102; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // vier
      }
    }
  }
  if(index[10] == 1) {
    if(START_N) {
      for(i = 91; i <= 94; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // vijf
      }
    }
    if(START_H) {
      for(i = 92; i <= 95; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // vijf
      }
    }
  }
  if(index[11] == 1) {
    if(START_N) {
      for(i = 118; i <= 120; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // zes
      }
    }
    if(START_H) {
      for(i = 110; i <= 112; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // zes
      }
    }
  }
  if(index[12] == 1) {
    if(START_N) {
      for(i = 78; i <= 82; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // zeven
      }
    }
    if(START_H) {
      for(i = 82; i <= 86; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // zeven
      }
    }
  }
  if(index[13] == 1) {
    if(START_N) {
      for(i = 114; i <= 117; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // acht
      }
    }
    if(START_H) {
      for(i = 113; i <= 116; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // acht
      }
    }
  }
  if(index[14] == 1) {
    if(START_N) {
      for(i = 44; i <= 48; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // negen
      }
    }
    if(START_H) {
      for(i = 50; i <= 54; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // negen
      }
    }
  }
  if(index[15] == 1) {
    if(START_N) {
      for(i = 69; i <= 72; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // tien
      }
    }
    if(START_H) {
      for(i = 70; i <= 73; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // tien
      }
    }
  }
  if(index[16] == 1) {
    if(START_N) {
      for(i = 88; i <= 90; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // elf
      }
    }
    if(START_H) {
      for(i = 96; i <= 98; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // elf
      }
    }
  }
  if(index[17] == 1) {
    if(START_N) {
      for(i = 99; i <= 104; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // twaalf
      }
    }
    if(START_H) {
      for(i = 104; i <= 109; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // twaalf
      }
    }
  }
  if(index[18] == 1) {
    if(START_N) {
      for(i = 110; i <= 112; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // uur
      }
    }
    if(START_H) {
      for(i = 118; i <= 120; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // uur
      }
    }
  }
  if(index[19] == 1) {
    if(START_N) {
      for(i = 8; i <= 10; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // het
      }
    }
    if(START_H) {
      for(i = 0; i <= 2; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // het
      }
    }
    if(START_N) {
      for(i = 5; i <= 6; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // is
      }
    }
    if(START_H) {
      for(i = 4; i <= 5; i++) {
        leds[i] = CHSV(hsv_hue, hsv_sat, hsv_val); // is
      }
    }
  }
  FastLED.show();

  /* Send current time through serial interface */
  Serial.print("Time has been updated. The displayed time is now ");
  Serial.print(hour());   Serial.print(":");
  Serial.print(minute()); Serial.print(":");
  Serial.print(second()); Serial.println(".");

  /* Reset the index array */
  for(i = 0; i <= 19; i++) {
    index[i] = 0;
  }
}

