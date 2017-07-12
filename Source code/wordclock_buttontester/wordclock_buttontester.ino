/* wordclock_buttontester.ino
 * A simple piece of code to test the buttons. Feedback is provided through serial output.
 * By Willem Pennings
 */

long buttonTime = 0;
int interval = 1000;

int incrementPin = 9;
int decrementPin = 10;
int colorPin = 11;

void setup() {
  Serial.begin(9600);

  pinMode(incrementPin, INPUT);
  pinMode(decrementPin, INPUT);
  pinMode(colorPin, INPUT);
}

void loop() {
  /* This block of code changes the system time when the buttons are pressed */
  if((digitalRead(incrementPin) == 0) && (digitalRead(decrementPin) == 1) && (millis() - buttonTime > interval)){
    buttonTime = millis();
    delay(500);
    if((digitalRead(incrementPin) == 0) && (digitalRead(decrementPin) == 1)) {   
      Serial.println("Time has been put 5 minutes forward");
    }
  }

  if((digitalRead(incrementPin) == 1) && (digitalRead(decrementPin) == 0) && (millis() - buttonTime > interval)){
    buttonTime = millis();
    delay(500);
    if((digitalRead(incrementPin) == 1) && (digitalRead(decrementPin) == 0)) {
      Serial.println("Time has been put 5 minutes backwards");
    }
  }

  if((digitalRead(colorPin) == 0) && (millis() - buttonTime > interval*0.5)) {
    buttonTime = millis();
    Serial.println("Color incremented!");
  }
}
