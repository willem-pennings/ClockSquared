/* ClockSquared by Willem Pennings
 * A WS2812B-based word clock with birthday feature
 * Copyright (C) 2017-2019 Willem Pennings (willemm.nl)
 * The FastLED and RTClib library are required to compile this sketch!
 */

/* Libraries */
#include <Wire.h>
#include <EEPROM.h>
#include "FastLED.h"
#include "RTClib.h"

RTC_DS3231 rtc;
RTC_Millis softrtc;
FASTLED_USING_NAMESPACE

const bool mirrorDisplay    = true;
const bool boustrofedon     = true;
                        
/* LED strip and grid definitions */
#define LED_TYPE              WS2812B     // [-] - LED strip model
#define COLOR_ORDER           GRB         // [-] - LED strip color order
const uint8_t numLeds       = 121;        // [-] - Number of LEDS
const uint8_t dataPin       = A0;         // [-] - LED data pin

CRGB leds[numLeds];
CRGB leds_p[numLeds];

/* Input pin definitions */
const uint8_t buttonPins[3] = {A1,A2,A3};  // [-] - Button pins
const uint8_t buttonLedPin  = 12;          // Button LED power pin

/* HSV colour variables */
uint8_t hue                 = 0;           // [-] - Red as standard hue value.
uint8_t sat                 = 0;           // [-] - 0 saturation for white colour as default.
uint8_t val                 = 255;         // [-] - Full brightness is standard value.

/* General variables */
uint16_t interval           = 1000;       // [ms] - General time interval variable
uint8_t brightness          = 128;        // [-] - LED strip global brightness on a scale of 0 to 255
unsigned long updateTime    = 0;          // [ms] - Time at which the last time update was done
bool index[20]              = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t i,j,k               = 0;          // [-] - General purpose indexing variables
bool maintenanceMode        = false;      // [-] - Maintenance mode toggle

/* Colour variables */
const uint8_t colours[8][3] PROGMEM = {{  0,  0,255},  // white
                                       {  0,255,255},  // red
                                       { 30,255,255},  // orange
                                       { 43,255,255},  // yellow
                                       { 85,255,255},  // green
                                       {170,255,255},  // blue
                                       {213,255,255},  // magenta
                                       {248,255,224}}; // pink

uint8_t colourIndex        = 0;
bool birthdayMode          = false;
bool manualBirthdayMode    = false;
bool softRTC               = false;

/* Birthday LED positions */
const uint8_t birthdayPos[16] PROGMEM = {34,35,36,37,38,55,56,57,58,59,60,61,62,63,64,65};

// Bitmap font locations
const uint8_t posL[35] PROGMEM = {22,23,24,25,26,33,34,35,36,37,44,45,46,47,48,55,56,57,58,59,66,67,68,69,70,77,78,79,80,81,88,89,90,91,92};
const uint8_t posR[35] PROGMEM = {28,29,30,31,32,39,40,41,42,43,50,51,52,53,54,61,62,63,64,65,72,73,74,75,76,83,84,85,86,87,94,95,96,97,98};
                          
/* 5x7 bitmap number font */
const uint8_t font5x7[10][35] PROGMEM = {{0,1,1,1,0,1,0,0,0,1,1,0,0,1,1,1,0,1,0,1,1,1,0,0,1,1,0,0,0,1,0,1,1,1,0},  // 0
                                         {0,1,1,0,0,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,1,1,1,1},  // 1
                                         {0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1},  // 2
                                         {0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,1,1,1,0,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0},  // 3
                                         {0,0,0,1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,0},  // 4
                                         {1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0},  // 5
                                         {0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0},  // 6
                                         {1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0},  // 7
                                         {0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0},  // 8
                                         {0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0}}; // 9

// Amount of birthdays that can be stored in EEPROM
const uint8_t nBirthdays = 20;

// Debounce condition for switching between maintenance mode and regular mode
unsigned long switchTime = 0;

// Timer for AM PM indicator display
unsigned long APTime = 0;

void setup() { 
  // Clear display at startup and provide time for hardware initialization
  delay(1000);
  FastLED.clear();
  
  // Initialize communications
  Serial.begin(115200);
  Wire.begin();

  // Initialize pins
  for(i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT);
  }
  
  pinMode(buttonLedPin, OUTPUT);
  digitalWrite(buttonLedPin, HIGH);

  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, dataPin, COLOR_ORDER>(leds_p, numLeds).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);

  if(rtc.lostPower()) {
    Serial.println(F("Could not find RTC or RTC lost power. Falling back to softrtc timekeeping..."));
    softrtc.begin(DateTime(2019, 1, 1, 0, 0, 0));
    rtc.adjust(DateTime(2019, 1, 1, 0, 0, 0));
    Serial.println(F("Datetime has been set to January 1, 2019, 12AM (midnight)."));
    softRTC = true;

    // Blink center LED three times in red to indicate RTC failure
    for(i = 0; i < 3; i++) {
      leds_p[60] = CRGB::Red;
      FastLED.show();
      delay(500);
      leds_p[60] = CRGB::Black;
      FastLED.show();
      delay(500);
    }
  }
  
  // Startup animation in RGB sequence to verify correctness of software colour order
  leds_p[60] = CRGB::Red;
  for(i = 0; i < 25; i++) {
    leds_p[60].fadeToBlackBy(26);
    FastLED.show();
    delay(20);
  }
  leds_p[60] = CRGB::Green;
  for(i = 0; i < 25; i++) {
    leds_p[60].fadeToBlackBy(26);
    FastLED.show();
    delay(20);
  }
  leds_p[60] = CRGB::Blue;
  for(i = 0; i < 25; i++) {
    leds_p[60].fadeToBlackBy(26);
    FastLED.show();
    delay(20);
  }
  leds_p[60] = CRGB::White;
  for(j = 0; j < 50; j++) {
    leds_p[60].fadeToBlackBy(26);
    FastLED.show();
    delay(10);
  }
  
  FastLED.clear();

  // Read previous screen colour from EEPROM
  if(EEPROM.read(2*nBirthdays) == 255) {
    colourIndex = 0;
  } else {
    colourIndex = EEPROM.read(2*nBirthdays);
  }
  hue = pgm_read_byte(&(colours[colourIndex][0]));
  sat = pgm_read_byte(&(colours[colourIndex][1]));
  val = pgm_read_byte(&(colours[colourIndex][2]));

  // Display system time on screen
  displayTime(hue, sat, val);

  // Check if the birthday message should be enabled
  checkBirthdays();
}

void loop() {
  if(!maintenanceMode) {
    // Timekeeping (refresh every minute)
    if(second() == 0 && (millis() - updateTime > interval)) {
      displayTime(hue, sat, val);
      updateTime = millis();
    }
  
    // Wipe birthday message at midnight and check for new birthdays
    if(hour() == 0 && minute() == 0 && second() == 0) {
      // Disable manual birthday mode if it was enabled
      if(manualBirthdayMode) {
        manualBirthdayMode = false;
      }
      
      if(birthdayMode) {
        for(i = 0; i < 16; i++) {
          leds[pgm_read_byte(&(birthdayPos[i]))] = CRGB::Black;
        }
      }

      // Check if there's a new birthday today
      checkBirthdays();
    }

    // Display birthday message if birthdayMode is enabled
    birthdays();
  
    // Check buttons
    buttons();
  } else {
    // Maintenance mode: set current date and birthdays
    setDateBirthdays();
  }

  // Check if AM/PM indicator should be shown (only when changing time)
  AMPMIndicator();

  // Refresh display
  printScreen(mirrorDisplay, boustrofedon);
}

// AM-PM indicator while changing time
void AMPMIndicator() {
  if(millis() - APTime < 2000) {
    if(hour() >= 12) {
      leds[3] = CRGB::Blue;
    } else {
      leds[3] = CRGB::Red;
    }
  } else {
    leds[3] = CRGB::Black;
  }
}

// Colour changing code
void changeColour() {
  if(colourIndex == 7) {
    colourIndex = 0;
  } else {
    colourIndex++;
  }

  hue = pgm_read_byte(&(colours[colourIndex][0]));
  sat = pgm_read_byte(&(colours[colourIndex][1]));
  val = pgm_read_byte(&(colours[colourIndex][2]));

  displayTime(hue, sat, val);

  // Write new colour value to EEPROM
  EEPROM.update(2*nBirthdays, colourIndex);
  
  Serial.println(F("Button 1 - Text colour changed."));
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
  
  if(incrementing) {
    brightness++;

    if(brightness == 32) {
      digitalWrite(buttonLedPin, HIGH);
      Serial.println(F("Button LEDs enabled."));
    }
  } else {
    brightness--;

    if(brightness == 32) {
      digitalWrite(buttonLedPin, LOW);
      Serial.println(F("Button LEDs disabled."));
    }
  }

  FastLED.setBrightness(brightness);
  
  Serial.print(F("Button 1 + 2 - brightness changed (")); Serial.print(brightness); Serial.println(F(")."));
}

// Increment time to the next 5-minute indicator
void incrementTime() {
  if(!softRTC) {
    DateTime now = rtc.now();
    int adjustment = -((5 + (minute() % 5)) * 60 + second()) + 600;
    rtc.adjust(now + adjustment);
  } else {
    DateTime now = softrtc.now();
    int adjustment = -((5 + (minute() % 5)) * 60 + second()) + 600;
    softrtc.adjust(now + adjustment);
    rtc.adjust(now + adjustment);
  }

  displayTime(hue, sat, val);
  checkBirthdays();
  APTime = millis();
  Serial.println(F("Button 2 - time incremented."));
}

// Decrement time to the previous 5-minute indicator
void decrementTime() {
  if(!softRTC) {
    DateTime now = rtc.now();
    int adjustment = -((5 + (minute() % 5)) * 60 + second());
    rtc.adjust(now + adjustment);
  } else {
    DateTime now = softrtc.now();
    int adjustment = -((5 + (minute() % 5)) * 60 + second());
    softrtc.adjust(now + adjustment);
    rtc.adjust(now + adjustment);
  }

  displayTime(hue, sat, val);
  checkBirthdays();
  APTime = millis();
  Serial.println(F("Button 3 - time decremented."));
}

void buttons() {
  int readings[3];
  static bool lastReadings[3];
  static unsigned long buttonTime, cTime, iTime, dTime, bTime, vTime, sTime;

  static int debounceTime = 200;

  // Get and process readings
  if(analogRead(buttonPins[0]) < 100) {readings[0] = true;} else {readings[0] = false;}
  if(analogRead(buttonPins[1]) < 100) {readings[1] = true;} else {readings[1] = false;}
  if(analogRead(buttonPins[2]) < 100) {readings[2] = true;} else {readings[2] = false;}

  // Check if button state has changed w.r.t. last loop
  for(i = 0; i < 3; i++) {
    if(readings[i] != lastReadings[i]) {
      // Register time at which buttons were pressed
      buttonTime = millis();
    }
  }

  // Debounce conditions
  if(millis() - buttonTime > debounceTime) {
    // Button 1 press
    if(readings[0] == 1 && readings[1] == 0 && readings[2] == 0 && (millis() - cTime > 500)) {
      changeColour();
      cTime = millis();
    } else
    // Button 2 press
    if(readings[0] == 0 && readings[1] == 1 && readings[2] == 0 && (millis() - iTime > 500)) {
      incrementTime();
      iTime = millis();
    } else
    // Button 3 press
    if(readings[0] == 0 && readings[1] == 0 && readings[2] == 1 && (millis() - dTime > 500)) {
      decrementTime();
      dTime = millis();
    } else
    // Button 1 + 2 press
    if(readings[0] == 1 && readings[1] == 1 && readings[2] == 0 && (millis() - bTime > 50)) {
      changeBrightness();
      bTime = millis();
    }
    // Button 2 + 3 press
    if(readings[0] == 0 && readings[1] == 1 && readings[2] == 1 && (millis() - vTime > 1000)) {
      changeBirthday();
      vTime = millis();
    }
    // Button 1 + 2 + 3 press
    if(readings[0] == 1 && readings[1] == 1 && readings[2] == 1 && (millis() - sTime > 1000) && (millis() - switchTime > 1000)) {
      maintenanceMode = true;
      FastLED.clear();

      // Clear screen
      for(i = 0; i < numLeds; i++) {
        leds[i] = CRGB:: Black;
      }

      // Set screen colour to white
      hue = 0; sat = 0; val = 255;

      // Clear EEPROM that contains birthdays (max. 20)
      for(i = 0; i < (2*nBirthdays); i++) {
        EEPROM.update(i,0);
      }
      
      Serial.println(F("Initialized maintenance mode!"));
      sTime = millis();
      switchTime = millis();
    }
  }
  
  // Save current readings to new variable for comparison in next loop
  for(i = 0; i < 3; i++) {
    lastReadings[i] = readings[i];
  }
}

// Manual birthday mode controller
void changeBirthday() {
  if(!manualBirthdayMode) {
    manualBirthdayMode = !manualBirthdayMode;
    Serial.println(F("Button 2 + 3 - Birthday mode turned on."));
  } else {
    manualBirthdayMode = !manualBirthdayMode;
    for(i = 0; i < 16; i++) {
      leds[pgm_read_byte(&(birthdayPos[i]))] = CRGB::Black;
    }
    displayTime(hue, sat, val);
    Serial.println(F("Button 2 + 3 - Birthday mode turned off."));
  }
}

// Check if birthday message should be activated based on automatic datekeeping
void checkBirthdays() {
  // Reset birthday boolean
  birthdayMode = false;

  for(i = 0; i < nBirthdays; i++) {
    if(day() == EEPROM.read(i*2) && month() == EEPROM.read(i*2+1)) {
      // If there's any birthday today, enable birthday mode
      birthdayMode = true;
    }
  }
}

// Birthday message code
void birthdays() {
  if((birthdayMode || manualBirthdayMode) && brightness >= 32) {
    static long int waitTime = 0;

    if(millis() - waitTime > 8) {
      for(i = 0; i < 16; i++) {
        leds[pgm_read_byte(&(birthdayPos[i]))] = CHSV(17*i - k, 255, 255);
      }
      k++;
      if(k == 255) {
        k = 0;
      }
      waitTime = millis();
    }
  } else {
    for(i = 0; i < 16; i++) {
      leds[pgm_read_byte(&(birthdayPos[i]))] = CRGB::Black;
    }
  }
}

void displayDigits(uint8_t num) {
  // Print two 5x7 bitmap numbers on the clock screen

  uint8_t numL = (num - (num % 10)) / 10;
  uint8_t numR = num % 10;

  for(i = 0; i < 35; i++) {
    if(pgm_read_byte(&(font5x7[numL][i])) == 1) {
      leds[pgm_read_byte(&(posL[i]))] = CHSV(hue, sat, val);
    }
  }

  for(i = 0; i < 35; i++) {
    if(pgm_read_byte(&(font5x7[numR][i])) == 1) {
      leds[pgm_read_byte(&(posR[i]))] = CHSV(hue, sat, val);
    }
  }
}

void setDateBirthdays() {
  static long int db, db1, db2, db3, db4;

  int readings[3];
  static bool lastReadings[3];
  
  static int debounceTime = 200;
  
  static uint8_t dayVal = 1;
  static uint8_t monVal = 1;

  static bool settingDay = true;
  static bool settingBirthday = false;

  static uint8_t nBirthday = 0;

  // Get and process readings
  if(analogRead(buttonPins[0]) < 100) {readings[0] = true;} else {readings[0] = false;}
  if(analogRead(buttonPins[1]) < 100) {readings[1] = true;} else {readings[1] = false;}
  if(analogRead(buttonPins[2]) < 100) {readings[2] = true;} else {readings[2] = false;}

  // Check if button state has changed w.r.t. last loop
  for(i = 0; i < 3; i++) {
    if(readings[i] != lastReadings[i]) {
      // Register time at which buttons were pressed
      db = millis();
    }
  }

  // Button logic
  if(millis() - db > debounceTime) {
    // Button 1 press
    if(readings[0] == 1 && readings[1] == 0 && readings[2] == 0 && (millis() - db1 > 500)) {
      if(!settingDay) {
        if(!settingBirthday) {
          // Write date value to clock
          rtc.adjust(DateTime(year(), monVal, dayVal, hour(), minute(), second()));
          
          // Reset date indicators
          dayVal = 1;
          monVal = 1;

          // Now start saving birthdays to EEPROM
          settingBirthday = true;

          // Confirm new date by printing it to console
          DateTime now = rtc.now();
          Serial.print("New date written to RTC: "); Serial.print(year()); Serial.print("-"); Serial.print(month()); Serial.print("-"); Serial.print(day()); Serial.print(" "); Serial.print(hour()); Serial.print(":"); Serial.print(minute()); Serial.print(":"); Serial.print(second()); Serial.println(".");
        } else {
          // Write birthday pair (day + month) to EEPROM
          EEPROM.update(nBirthday*2, dayVal);
          EEPROM.update(nBirthday*2+1, monVal);

          // Confirm birthday by printing it to console
          Serial.print("New birthday saved (number "); Serial.print(nBirthday); Serial.print("). Date: "); Serial.print(dayVal); Serial.print("-"); Serial.print(monVal); Serial.println(".");

          // Reset date indicators
          dayVal = 1;
          monVal = 1;

          // Increase the birthday counter if there is sufficient room
          if(nBirthday < 20) {
            nBirthday++;
          } else {
            // Otherwise, quit maintenance mode and resume normal operation
            for(i = 0; i < numLeds; i++) {
              leds[i] = CRGB::Black;
            }

            // Retrieve old hue, sat and val from EEPROM
            hue = pgm_read_byte(&(colours[colourIndex][0]));
            sat = pgm_read_byte(&(colours[colourIndex][1]));
            val = pgm_read_byte(&(colours[colourIndex][2]));
            
            displayTime(hue, sat, val);

            // Reset day and month indicators
            dayVal = 1;
            monVal = 1;
            
            maintenanceMode = false;
            checkBirthdays();
          }
        }
      }
      
      // Flip the current setting
      settingDay = !settingDay;
      
      db1 = millis();
    } else
    // Button 2 press
    if(readings[0] == 0 && readings[1] == 1 && readings[2] == 0 && (millis() - db2 > 500)) {
      if(settingDay) {
        if(dayVal < 31) {
          dayVal++;
        } else {
          dayVal = 1;
        }
      } else {
        if(monVal < 12) {
          monVal++;
        } else {
          monVal = 1;
        }
      }
      db2 = millis();
    } else
    // Button 3 press
    if(readings[0] == 0 && readings[1] == 0 && readings[2] == 1 && (millis() - db3 > 500)) {
      if(settingDay) {
        if(dayVal > 1) {
          dayVal--;
        } else {
          dayVal = 31;
        }
      } else {
        if(monVal > 1) {
          monVal--;
        } else {
          monVal = 12;
        }
      }
      db3 = millis();
    }
    // Button 1 + 2 + 3 press
    if(readings[0] == 1 && readings[1] == 1 && readings[2] == 1 && (millis() - db4 > 500) && (millis() - switchTime > 1000)) {
      // Quit maintenance mode and resume normal operation
      for(i = 0; i < numLeds; i++) {
        leds[i] = CRGB::Black;
      }

      // Retrieve old hue, sat and val from EEPROM
      hue = pgm_read_byte(&(colours[colourIndex][0]));
      sat = pgm_read_byte(&(colours[colourIndex][1]));
      val = pgm_read_byte(&(colours[colourIndex][2]));
      
      displayTime(hue, sat, val);

      // Reset day and month indicators
      dayVal = 1;
      monVal = 1;

      Serial.println("Maintenance mode disabled!");
      
      maintenanceMode = false;
      checkBirthdays();
      
      db4 = millis();
      switchTime = millis();
    }
  }

  // Clear original led index
  if(maintenanceMode) {
    for(i = 0; i < numLeds; i++) {
      leds[i] = CRGB::Black;
    }

    if(settingDay) {
      displayDigits(dayVal);
    } else {
      displayDigits(monVal);
    }
  }

  // Save current readings to new variable for comparison in next loop
  for(i = 0; i < 3; i++) {
    lastReadings[i] = readings[i];
  }
}

/* This is the time function that takes the current time, translates
 * it to hours and minutes, and then translates that to a series of
 * LEDs to light up.
 */
void displayTime(int hue, int sat, int val) {
  for(i = 0; i <= 33; i++) {
    leds[i] = CRGB::Black;
  }
  for(i = 39; i <= 54; i++) {
    leds[i] = CRGB::Black;
  }
  for(i = 66; i < numLeds; i++) {
    leds[i] = CRGB::Black;
  }
  
  // Minute indicator control
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

  // Hour indicator control
  if(minute() < 20) {
    switch(hourFormat12()) {
      case  1: index[6]  = 1; break; // 'een'
      case  2: index[7]  = 1; break; // 'twee'
      case  3: index[8]  = 1; break; // 'drie'
      case  4: index[9]  = 1; break; // 'vier'
      case  5: index[10] = 1; break; // 'vijf'
      case  6: index[11] = 1; break; // 'zes'
      case  7: index[12] = 1; break; // 'zeven'
      case  8: index[13] = 1; break; // 'acht'
      case  9: index[14] = 1; break; // 'negen'
      case 10: index[15] = 1; break; // 'tien'
      case 11: index[16] = 1; break; // 'elf'
      case 12: index[17] = 1; break; // 'twaalf'
    }

    if(minute() < 5) {
      index[18] = 1; // 'uur'
    }
  } else {
    switch(hourFormat12()) {
      case  1: index[7]  = 1; break; // 'twee'
      case  2: index[8]  = 1; break; // 'drie'
      case  3: index[9]  = 1; break; // 'vier'
      case  4: index[10] = 1; break; // 'vijf'
      case  5: index[11] = 1; break; // 'zes'
      case  6: index[12] = 1; break; // 'zeven'
      case  7: index[13] = 1; break; // 'acht'
      case  8: index[14] = 1; break; // 'negen'
      case  9: index[15] = 1; break; // 'tien'
      case 10: index[16] = 1; break; // 'elf'
      case 11: index[17] = 1; break; // 'twaalf'
      case 12: index[6]  = 1; break; // 'een'
    }
  }

  // Check all indexes and activate the corresponding LEDs
  for(i = 0; i <= 2; i++) {leds[i] = CHSV(hue, sat, val);} // het
  for(i = 4; i <= 5; i++) {leds[i] = CHSV(hue, sat, val);} // is
  if(index[ 0] == 1) {for(i =  18; i <=  21; i++) {leds[i] = CHSV(hue, sat, val);}} // vijf
  if(index[ 1] == 1) {for(i =   7; i <=  10; i++) {leds[i] = CHSV(hue, sat, val);}} // tien
  if(index[ 2] == 1) {for(i =  12; i <=  16; i++) {leds[i] = CHSV(hue, sat, val);}} // kwart
  if(index[ 3] == 1) {for(i =  40; i <=  43; i++) {leds[i] = CHSV(hue, sat, val);}} // kwart
  if(index[ 4] == 1) {for(i =  22; i <=  25; i++) {leds[i] = CHSV(hue, sat, val);}} // voor
  if(index[ 5] == 1) {for(i =  28; i <=  31; i++) {leds[i] = CHSV(hue, sat, val);}} // over
  if(index[ 6] == 1) {for(i =  66; i <=  68; i++) {leds[i] = CHSV(hue, sat, val);}} // een
  if(index[ 7] == 1) {for(i =  45; i <=  48; i++) {leds[i] = CHSV(hue, sat, val);}} // twee
  if(index[ 8] == 1) {for(i =  88; i <=  91; i++) {leds[i] = CHSV(hue, sat, val);}} // drie
  if(index[ 9] == 1) {for(i = 106; i <= 109; i++) {leds[i] = CHSV(hue, sat, val);}} // vier
  if(index[10] == 1) {for(i =  92; i <=  95; i++) {leds[i] = CHSV(hue, sat, val);}} // vijf
  if(index[11] == 1) {for(i = 110; i <= 112; i++) {leds[i] = CHSV(hue, sat, val);}} // zes
  if(index[12] == 1) {for(i =  78; i <=  82; i++) {leds[i] = CHSV(hue, sat, val);}} // zeven
  if(index[13] == 1) {for(i = 113; i <= 116; i++) {leds[i] = CHSV(hue, sat, val);}} // acht
  if(index[14] == 1) {for(i =  50; i <=  54; i++) {leds[i] = CHSV(hue, sat, val);}} // negen
  if(index[15] == 1) {for(i =  70; i <=  73; i++) {leds[i] = CHSV(hue, sat, val);}} // tien
  if(index[16] == 1) {for(i =  96; i <=  98; i++) {leds[i] = CHSV(hue, sat, val);}} // elf
  if(index[17] == 1) {for(i =  99; i <= 104; i++) {leds[i] = CHSV(hue, sat, val);}} // twaalf
  if(index[18] == 1) {for(i = 118; i <= 120; i++) {leds[i] = CHSV(hue, sat, val);}} // uur

  // Write RGB values to LEDs
  printScreen(mirrorDisplay, boustrofedon);

  /* Send current time through serial interface */
  Serial.print(F("Clock screen was refreshed. System date/time is "));
  Serial.print(year());   Serial.print(F("-"));
  Serial.print(month());  Serial.print(F("-"));
  Serial.print(day());    Serial.print(F(", "));
  Serial.print(hour());   Serial.print(F(":"));
  Serial.print(minute()); Serial.print(F(":"));
  Serial.print(second()); Serial.print(F(". "));
  Serial.print(F("Brightness is ")); Serial.print(brightness); Serial.println(F("."));

  /* Reset the index array */
  for(i = 0; i <= 19; i++) {
    index[i] = 0;
  }

  // Register update time
  updateTime = millis();
}

void printScreen(bool mirror, bool boustrofedon) {
  if(mirror) {
    // We need to mirror the screen if the LED with index 0 is not located at the left side of the screen
    // Set all output LEDs to black first
    for(i = 0; i < numLeds; i++) {
      leds_p[i] = CRGB::Black;
    }

    if(boustrofedon) {
      // If the LEDs are arranged in boustrofedon, we need not mirror every row, but just the even ones (and 0)
      for(i = 0; i < 11; i++) {
        if(i % 2 == 0) {
          // Mirror the even ones
          for(j = 0; j < 11; j++) {
            leds_p[i*11+j] = leds[i*11+10-j];
          }
        } else {
          // Copy the uneven ones as is
          for(j = 0; j < 11; j++) {
            leds_p[i*11+j] = leds[i*11+j];
          }
        }
      }
    } else {
      // If the LEDs are not arranged in boustrofedon, we need to mirror every row
      for(i = 0; i < 11; i++) {
        for(j = 0; j < 11; j++) {
            leds_p[i*11+j] = leds[i*11+10-j];
        }
      }
    }
  } else {
    if(boustrofedon) {
      // Boustrofedon rearrangement without mirroring
      for(i = 0; i < 11; i++) {
        if(i % 2 == 0) {
          // Copy the even ones as is
          for(j = 0; j < 11; j++) {
            leds_p[i*11+j] = leds[i*11+j];
          }
        } else {
          // Mirror the uneven ones
          for(j = 0; j < 11; j++) {
            leds_p[i*11+j] = leds[i*11+10-j];
          }
        }
      }
    } else {
      // If the LEDs are arranged in standard matrix form, we need not rearrange anything
      for(i = 0; i < numLeds; i++) {
        leds_p[i] = leds[i];
      }
    }
  }

  // Print output to screen
  FastLED.show();
}

// RTClib function extensions
int year() {
  if(!softRTC) {
    DateTime now = rtc.now();
    return now.year();
  }
  else {
    DateTime now = softrtc.now();
    return now.year();
  }
}

int month() {
  if(!softRTC) {
    DateTime now = rtc.now();
    return now.month();
  } else {
    DateTime now = softrtc.now();
    return now.month();
  }
}

int day() {
  if(!softRTC) {
    DateTime now = rtc.now();
    return now.day();
  } else {
    DateTime now = softrtc.now();
    return now.day();
  }
}

int hour() {
  if(!softRTC) {
    DateTime now = rtc.now();
    return now.hour();
  } else {
    DateTime now = softrtc.now();
    return now.hour();
  }
}

int minute() {
  if(!softRTC) {
    DateTime now = rtc.now();
    return now.minute();
  } else {
    DateTime now = softrtc.now();
    return now.minute();
  }
}

int second() {
  if(!softRTC) {
    DateTime now = rtc.now();
    return now.second();
  } else {
    DateTime now = softrtc.now();
    return now.second();
  }
}

int hourFormat12() {
  if(!softRTC) {
    DateTime now = rtc.now();
    
    if(now.hour() > 12) {
      return now.hour() - 12;
    } else 
    if(now.hour() == 0) {
      return 12;
    } else {
      return now.hour();
    }
  } else {
    DateTime now = softrtc.now();

    if(now.hour() > 12) {
      return now.hour() - 12;
    } else 
    if(now.hour() == 0) {
      return 12;
    } else {
      return now.hour();
    }
  }
}
