/* ClockSquared by Willem Pennings
 * A WS2812B-based word clock with birthday feature
 * Copyright (C) 2017-2019 Willem Pennings (willemm.nl)
 * The FastLED and RTClib library are required to compile this sketch!
 */

/* Libraries */
#include <Wire.h>
#include <Time.h>
#include "FastLED.h"
#include "RTClib.h"

RTC_DS3231 rtc;
FASTLED_USING_NAMESPACE

/* Does the LED strip start at the top left letter 'H' or the top right letter 'N'? */
const int startN           = 1;          // [-]
const int startH           = 0;          // [-]
                        
/* LED strip and grid definitions */
#define LED_TYPE             WS2812B     // [-] - LED strip model
#define COLOR_ORDER          GRB         // [-] - LED strip color order
const int numLeds          = 121;        // [-] - Number of LEDS
const int dataPin          = A0;         // [-] - LED data pin
const int clockPin         = 0;          // [-] - LED clock pin
CRGB leds[numLeds];  
                        
/* Input pin definitions */
const int buttonPins[3]    = {A1,A2,A3};  // [-] - Button pins

/* HSV colour variables */
int hue                   = 0;           // [-] - Red as standard hue value.
int sat                   = 0;           // [-] - 0 saturation for white colour as default.
int val                   = 255;         // [-] - Full brightness is standard value.

/* General variables */
int interval               = 1000;       // [ms] - General time interval variable
int brightness             = 128;        // [-] - LED strip global brightness on a scale of 0 to 255
long updateTime            = 0;          // [ms] - Time at which the last time update was done
bool index[20]             = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int i,j,k                  = 0;          // [-] - General purpose indexing variables

/* Colour variables */
int colours[7][3]          = {{  0,  0,255}, // white
                              {  0,255,255}, // red
                              { 30,255,255}, // orange
                              { 43,255,255}, // yellow
                              { 85,255,255}, // green
                              {170,255,255}, // blue
                              {213,255,255}}; // purple

int colourIndex            = 0;
bool birthdayMode          = false;

/* Startup animation data */
int ring1[ 1] = {60};
int ring2[ 8] = {48,49,50,59,72,71,70,61};
int ring3[16] = {36,37,38,39,40,47,51,58,62,69,73,80,81,82,83,84};
int ring4[24] = {24,25,26,27,28,29,30,35,41,46,52,57,63,68,74,79,85,90,91,92,93,94,95,96};
int ring5[32] = {12,13,14,15,16,17,18,19,20,23,31,34,42,45,53,56,64,67,75,78,86,89,97,100,101,102,103,104,105,106,107,108};
int ring6[40] = {0,1,2,3,4,5,6,7,8,9,10,11,32,33,54,55,76,77,98,99,120,119,118,117,116,115,114,113,112,111,110,109,88,87,66,65,44,43,22,21};

bool startupDone = false;

void setup() {
  // Clear display at startup and provide time for hardware initialization
  FastLED.clear();
  delay(1000);
  
  // Initialize communications
  Serial.begin(115200);
  Wire.begin();

  // Initialize pins
  for(i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT);
  }

  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, dataPin, COLOR_ORDER>(leds, numLeds).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Could not find RTC.");
    while(millis() < 10000) {
      leds[0] = CRGB::Red;
      FastLED.show();
    }
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power -- time inaccurate.");
  }
  
  // Startup animation
  leds[60] = CRGB::Red;
  for(i = 0; i < 25; i++) {
    leds[60].fadeToBlackBy(26);
    FastLED.show();
    delay(20);
  }
  leds[60] = CRGB::Green;
  for(i = 0; i < 25; i++) {
    leds[60].fadeToBlackBy(26);
    FastLED.show();
    delay(20);
  }
  leds[60] = CRGB::Blue;
  for(i = 0; i < 25; i++) {
    leds[60].fadeToBlackBy(26);
    FastLED.show();
    delay(20);
  }
  for(i = 0; i < 5; i++) {
    leds[60] = CRGB::White;
    for(j = 0; j < 25; j++) {
      leds[60].fadeToBlackBy(26);
      FastLED.show();
      delay(10);
    }
  }
  leds[60] = CRGB::White;
  for(j = 0; j < 50; j++) {
    leds[60].fadeToBlackBy(26);
    FastLED.show();
    delay(10);
  }
  
  FastLED.clear();

  // Display system time on screen
  displayTime(hue, sat, val);
  FastLED.show();
}

unsigned long animationStartTime;

void loop() {
  // Timekeeping (refresh every minute)
  if(second() == 0 && (millis() - updateTime > interval)) {
    displayTime(hue, sat, val);
    updateTime = millis();
  }

  // Wipe matrix at midnight to clear birthday messages
  if(hour() == 0 && minute() == 0 && second() == 0) {
    FastLED.clear();
    displayTime(hue, sat, val);
  }

  // Check buttons
  buttons();

  // Check for birthdays and display birthday greetings
  birthdays();

  // Refresh display
  FastLED.show();
}

// Colour changing code
void changeColour() {
  if(colourIndex == 6) {
    colourIndex = 0;
  } else {
    colourIndex++;
  }

  hue = colours[colourIndex][0];
  sat = colours[colourIndex][1];
  val = colours[colourIndex][2];

  displayTime(hue, sat, val);
}

// Change screen brightness
void changeBrightness() {
  static bool incrementing = false;
  
  if(brightness == 128) {
    incrementing = false;
  } else
  if(brightness == 0) {
    incrementing = true;
  }

  if(brightness < 32) {
    birthdayMode = false;
    FastLED.clear();
    displayTime(hue, sat, val);
  }
  
  if(incrementing) {
    brightness++;
    FastLED.setBrightness(brightness);
  } else {
    brightness--;
    FastLED.setBrightness(brightness);
  }
}

// Increment time to the next 5-minute indicator
void incrementTime() {
  DateTime now = rtc.now();
  int adjustment = -((5 + (minute() % 5)) * 60 + second()) + 600;
  rtc.adjust(now + adjustment);
  displayTime(hue, sat, val);
}

// Decrement time to the previous 5-minute indicator
void decrementTime() {
  DateTime now = rtc.now();
  int adjustment = -((5 + (minute() % 5)) * 60 + second());
  rtc.adjust(now + adjustment);
  displayTime(hue, sat, val);
}

void buttons() {
  bool readings[3];
  static bool lastReadings[3];
  static unsigned long buttonTime, cTime, iTime, dTime, bTime, vTime;

  static int debounceTime = 250;

  // Get readings
  for(i = 0; i < 3; i++) {
    // Pull-up circuit: invert reading
    readings[i] = !(digitalRead(buttonPins[i]));
  }

  // Check if button state has changed w.r.t. last loop
  for(i = 0; i < 3; i++) {
    if(readings[i] != lastReadings[i]) {
      // Register time at which buttons were pressed
      buttonTime = millis();
    }
  }

  // Debounce conditions
  if(millis() - buttonTime > debounceTime) {
    if(readings[0] == 1 && readings[1] == 0 && readings[2] == 0 && (millis() - cTime > 500)) {
      changeColour();
      cTime = millis();
    } else
    if(readings[0] == 0 && readings[1] == 1 && readings[2] == 0 && (millis() - iTime > 1000)) {
      incrementTime();
      iTime = millis();
    } else
    if(readings[0] == 0 && readings[1] == 0 && readings[2] == 1 && (millis() - dTime > 1000)) {
      decrementTime();
      dTime = millis();
    } else
    if(readings[0] == 1 && readings[1] == 1 && readings[2] == 0 && (millis() - bTime > 50)) {
      changeBrightness();
      bTime = millis();
    }
    if(readings[0] == 0 && readings[1] == 1 && readings[2] == 1 && (millis() - vTime > 1000)) {
      changeBirthday();
      vTime = millis();
    }
  }
  
  // Save current readings to new variable for comparison in next loop
  for(i = 0; i < 3; i++) {
    lastReadings[i] = readings[i];
  }
}

// Manual birthday mode controller
void changeBirthday() {
  if(brightness >= 32 || birthdayMode) {
    birthdayMode = !birthdayMode;
  }
  if(!birthdayMode) {
    FastLED.clear();
    displayTime(hue, sat, val);
  }
}

// Birthday message code
void birthdays() {
  if(birthdayMode) {
    static long int waitTime = 0;
    static int posH[16] = {42, 41, 40, 39, 38, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55};
    static int posN[16] = {34, 35, 36, 37, 38, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65};

    if(millis() - waitTime > 8) {
      for(i=0;i<16;i++) {
        if(startH) {
          leds[posH[i]] = CHSV(17*i - j, 255, 255);
        } else {
          leds[posN[i]] = CHSV(17*i - j, 255, 255);
        }
      }
      j++;
      if(j == 255) {
        j = 0;
      }
      waitTime = millis();
    }
  }
}

/* This is the time function that takes the current time, translates
 * it to hours and minutes, and then translates that to a series of
 * LEDs to light up.
 */
void displayTime(int hue, int sat, int val) {
  if(startN) {
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
  if(startH) {
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
    if(startN) {
      for(i = 18; i <= 21; i++) {
          leds[i] = CHSV(hue, sat, val); // vijf
      }
    }
    if(startH) {
      for(i = 11; i <= 14; i++) {
          leds[i] = CHSV(hue, sat, val); // vijf
      }
    }
  }
  if(index[1] == 1) {
    if(startN) {
      for(i = 0; i <= 3; i++) {
        leds[i] = CHSV(hue, sat, val); // tien
      }
    }
    if(startH) {
      for(i = 7; i <= 10; i++) {
        leds[i] = CHSV(hue, sat, val); // tien
      }
    }
  }
  if(index[2] == 1) {
    if(startN) {
      for(i = 12; i <= 16; i++) {
        leds[i] = CHSV(hue, sat, val); // kwart
      }
    }
    if(startH) {
      for(i = 16; i <= 20; i++) {
        leds[i] = CHSV(hue, sat, val); // kwart
      }
    }
  }
  if(index[3] == 1) {
    if(startN) {
      for(i = 40; i <= 43; i++) {
        leds[i] = CHSV(hue, sat, val); // half
      }
    }
    if(startH) {
      for(i = 33; i <= 36; i++) {
        leds[i] = CHSV(hue, sat, val); // half
      }
    }
  }
  if(index[4] == 1) {
    if(startN) {
      for(i = 29; i <= 32; i++) {
        leds[i] = CHSV(hue, sat, val); // voor
      }
    }
    if(startH) {
      for(i = 22; i <= 25; i++) {
        leds[i] = CHSV(hue, sat, val); // voor
      }
    }
  }
  if(index[5] == 1) {
    if(startN) {
      for(i = 23; i <= 26; i++) {
        leds[i] = CHSV(hue, sat, val); // over
      }
    }
    if(startH) {
      for(i = 28; i <= 31; i++) {
        leds[i] = CHSV(hue, sat, val); // over
      }
    }
  }
  if(index[6] == 1) {
    if(startN) {
      for(i = 74; i <= 76; i++) {
        leds[i] = CHSV(hue, sat, val); // een
      }
    }
    if(startH) {
      for(i = 66; i <= 68; i++) {
        leds[i] = CHSV(hue, sat, val); // een
      }
    }
  }
  if(index[7] == 1) {
    if(startN) {
      for(i = 50; i <= 53; i++) {
        leds[i] = CHSV(hue, sat, val); // twee
      }
    }
    if(startH) {
      for(i = 45; i <= 48; i++) {
        leds[i] = CHSV(hue, sat, val); // twee
      }
    }
  }
  if(index[8] == 1) {
    if(startN) {
      for(i = 95; i <= 98; i++) {
        leds[i] = CHSV(hue, sat, val); // drie
      }
    }
    if(startH) {
      for(i = 88; i <= 91; i++) {
        leds[i] = CHSV(hue, sat, val); // drie
      }
    }
  }
  if(index[9] == 1) {
    if(startN) {
      for(i = 106; i <= 109; i++) {
        leds[i] = CHSV(hue, sat, val); // vier
      }
    }
    if(startH) {
      for(i = 99; i <= 102; i++) {
        leds[i] = CHSV(hue, sat, val); // vier
      }
    }
  }
  if(index[10] == 1) {
    if(startN) {
      for(i = 91; i <= 94; i++) {
        leds[i] = CHSV(hue, sat, val); // vijf
      }
    }
    if(startH) {
      for(i = 92; i <= 95; i++) {
        leds[i] = CHSV(hue, sat, val); // vijf
      }
    }
  }
  if(index[11] == 1) {
    if(startN) {
      for(i = 118; i <= 120; i++) {
        leds[i] = CHSV(hue, sat, val); // zes
      }
    }
    if(startH) {
      for(i = 110; i <= 112; i++) {
        leds[i] = CHSV(hue, sat, val); // zes
      }
    }
  }
  if(index[12] == 1) {
    if(startN) {
      for(i = 78; i <= 82; i++) {
        leds[i] = CHSV(hue, sat, val); // zeven
      }
    }
    if(startH) {
      for(i = 82; i <= 86; i++) {
        leds[i] = CHSV(hue, sat, val); // zeven
      }
    }
  }
  if(index[13] == 1) {
    if(startN) {
      for(i = 114; i <= 117; i++) {
        leds[i] = CHSV(hue, sat, val); // acht
      }
    }
    if(startH) {
      for(i = 113; i <= 116; i++) {
        leds[i] = CHSV(hue, sat, val); // acht
      }
    }
  }
  if(index[14] == 1) {
    if(startN) {
      for(i = 44; i <= 48; i++) {
        leds[i] = CHSV(hue, sat, val); // negen
      }
    }
    if(startH) {
      for(i = 50; i <= 54; i++) {
        leds[i] = CHSV(hue, sat, val); // negen
      }
    }
  }
  if(index[15] == 1) {
    if(startN) {
      for(i = 69; i <= 72; i++) {
        leds[i] = CHSV(hue, sat, val); // tien
      }
    }
    if(startH) {
      for(i = 70; i <= 73; i++) {
        leds[i] = CHSV(hue, sat, val); // tien
      }
    }
  }
  if(index[16] == 1) {
    if(startN) {
      for(i = 88; i <= 90; i++) {
        leds[i] = CHSV(hue, sat, val); // elf
      }
    }
    if(startH) {
      for(i = 96; i <= 98; i++) {
        leds[i] = CHSV(hue, sat, val); // elf
      }
    }
  }
  if(index[17] == 1) {
    if(startN) {
      for(i = 99; i <= 104; i++) {
        leds[i] = CHSV(hue, sat, val); // twaalf
      }
    }
    if(startH) {
      for(i = 104; i <= 109; i++) {
        leds[i] = CHSV(hue, sat, val); // twaalf
      }
    }
  }
  if(index[18] == 1) {
    if(startN) {
      for(i = 110; i <= 112; i++) {
        leds[i] = CHSV(hue, sat, val); // uur
      }
    }
    if(startH) {
      for(i = 118; i <= 120; i++) {
        leds[i] = CHSV(hue, sat, val); // uur
      }
    }
  }
  if(index[19] == 1) {
    if(startN) {
      for(i = 8; i <= 10; i++) {
        leds[i] = CHSV(hue, sat, val); // het
      }
    }
    if(startH) {
      for(i = 0; i <= 2; i++) {
        leds[i] = CHSV(hue, sat, val); // het
      }
    }
    if(startN) {
      for(i = 5; i <= 6; i++) {
        leds[i] = CHSV(hue, sat, val); // is
      }
    }
    if(startH) {
      for(i = 4; i <= 5; i++) {
        leds[i] = CHSV(hue, sat, val); // is
      }
    }
  }
  FastLED.show();

  /* Send current time through serial interface */
  Serial.print("Clock screen was refreshed. System date/time is ");
  Serial.print(year());   Serial.print("-");
  Serial.print(month());  Serial.print("-");
  Serial.print(day());    Serial.print(", ");
  Serial.print(hour());   Serial.print(":");
  Serial.print(minute()); Serial.print(":");
  Serial.print(second()); Serial.println(".");

  /* Reset the index array */
  for(i = 0; i <= 19; i++) {
    index[i] = 0;
  }

  // Register update time
  updateTime = millis();
}

// RTClib function extensions
int year() {
  DateTime now = rtc.now();
  return now.year();
}

int month() {
  DateTime now = rtc.now();
  return now.month();
}

int day() {
  DateTime now = rtc.now();
  return now.day();
}

int hour() {
  DateTime now = rtc.now();
  return now.hour();
}

int minute() {
  DateTime now = rtc.now();
  return now.minute();
}

int second() {
  DateTime now = rtc.now();
  return now.second();
}

int hourFormat12() {
  DateTime now = rtc.now();
  if(now.hour() > 12) {
    return now.hour() - 12;
  } else 
  if(now.hour() == 0) {
    return 12;
  } else {
    return now.hour();
  }
}
