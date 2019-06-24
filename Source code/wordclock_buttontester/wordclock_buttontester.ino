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
  if((digitalRead(incrementPin) == 0) && (millis() - buttonTime > interval)){
    buttonTime = millis();
    delay(500);
    if((digitalRead(incrementPin) == 0) && (digitalRead(decrementPin) == 1)) {   
      Serial.println("Registered increment button press");
    }
  }

  if((digitalRead(incrementPin) == 1) && (millis() - buttonTime > interval)){
    buttonTime = millis();
    delay(500);
    if((digitalRead(incrementPin) == 1) && (digitalRead(decrementPin) == 0)) {
      Serial.println("Registered decrement button press");
    }
  }

  if((digitalRead(colorPin) == 0) && (millis() - buttonTime > interval)) {
    buttonTime = millis();
    Serial.println("Registered colour button press");
  }
}
