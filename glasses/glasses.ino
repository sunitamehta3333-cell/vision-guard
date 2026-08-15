/*
 * Vision Guard — Sensor Glasses firmware
 * --------------------------------------
 * Covers the head-level zone the cane cannot reach: signboards, branches,
 * wall-mounted units, vehicle mirrors. An HC-SR04 mounted on a spectacle
 * frame looks forward; an active buzzer warns the user when an overhead or
 * head-height obstacle is within ALERT_CM.
 *
 * The glasses are deliberately self-contained: no Bluetooth, no app. They
 * do one job — warn of head-level hazards — and keep doing it even if the
 * phone or the cane is off. Simplicity here is a safety feature.
 *
 * Wiring (Arduino Nano)
 *   HC-SR04 VCC  -> 5V       HC-SR04 Trig -> 9
 *   HC-SR04 GND  -> GND      HC-SR04 Echo -> 8
 *   Active buzzer +  -> 7    (buzzer - -> GND)
 *
 * An ACTIVE buzzer makes its own tone from a steady HIGH — no PWM needed.
 * If you use a passive buzzer instead, replace digitalWrite(buzzerPin, HIGH)
 * with tone(buzzerPin, 2000) and digitalWrite(..., LOW) with noTone(buzzerPin).
 */

const int trigPin   = 9;
const int echoPin   = 8;
const int buzzerPin = 7;

// Head-level hazards should warn a little earlier than the cane's waist
// threshold, so the user can stop before walking into them.
const int ALERT_CM = 80;

long readings[3];
int  idx = 0;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  Serial.begin(9600);   // for bench testing over USB
}

long readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 400;        // nothing in range
  return duration * 0.034 / 2;          // centimetres
}

void loop() {
  long d = readDistance();

  // Same three-reading confirmation filter as the cane: a passer-by
  // produces one or two odd readings, not three in a row.
  readings[idx] = d;
  idx = (idx + 1) % 3;

  bool tooClose = (readings[0] < ALERT_CM &&
                   readings[1] < ALERT_CM &&
                   readings[2] < ALERT_CM);

  if (tooClose) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  Serial.print(d);
  Serial.println(tooClose ? ",ALERT" : ",OK");

  delay(200);   // five readings per second
}
