/* wordclock_buttontester.ino
 * A simple piece of code to test the buttons. Feedback is provided through serial output.
 * By Willem Pennings
 */

long buttonTime = 0;
int interval = 1000;

int incrementPin = A2;
int decrementPin = A3;
int colorPin = A1;

int a, b, c;

void setup() {
  Serial.begin(115200);

  pinMode(incrementPin, INPUT);
  pinMode(decrementPin, INPUT);
  pinMode(colorPin, INPUT);
}

void loop() {
  a = analogRead(colorPin);
  b = analogRead(incrementPin);
  c = analogRead(decrementPin);

  Serial.print(a); Serial.print("\t");
  Serial.print(b); Serial.print("\t");
  Serial.println(c);
}
