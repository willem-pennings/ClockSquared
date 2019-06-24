#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 121

#define DATA_PIN 2
#define CLOCK_PIN 3

CRGB leds[NUM_LEDS];

void setup() { 
  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN,GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(40);
}

void loop() { 
  for(int i=0; i<NUM_LEDS; i++){
    leds[i] = CHSV(0, 0, 255);
    FastLED.show();
    delay(100);
  }
  delay(5000);
  FastLED.clear();
  for(int i=0; i<NUM_LEDS; i++){
    FastLED.clear();
    leds[i] = CHSV(0, 0, 255);
    FastLED.show();
    delay(1000);
  }
}
