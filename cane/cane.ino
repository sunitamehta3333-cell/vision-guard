/*
 * Vision Guard — Smart Cane firmware
 * ----------------------------------
 * Measures distance with an HC-SR04 ultrasonic sensor, drives a vibration
 * motor when something is closer than ALERT_CM, and streams live readings
 * to the Vision Guard app over a serial Bluetooth module.
 *
 * Bluetooth module:
 *   Uses an HC-05 (Bluetooth Classic) module, wired as a plain 3.3 V UART
 *   serial port. The Vision Guard app (app/BUILD_GUIDE.md) pairs with it
 *   through App Inventor's built-in BluetoothClient component.
 *
 * Serial output format, one line per reading:
 *   "87,ALERT"   distance in centimetres, comma, status
 *   "250,OK"
 *
 * Wiring — sensor and motor
 *   HC-SR04 VCC -> 5V      HC-SR04 Trig -> 9
 *   HC-SR04 GND -> GND     HC-SR04 Echo -> 8
 *   Motor driver IN -> 7   (motor takes its own power via the driver)
 *
 * Wiring — Bluetooth (HC-05)
 *   HC-05 VCC -> 5V        HC-05 TXD -> 10
 *   HC-05 GND -> GND       HC-05 RXD -> 11 VIA A VOLTAGE DIVIDER (see README)
 *
 * The voltage divider on RXD is not optional: the Arduino outputs 5 V and
 * the module's RXD pin expects 3.3 V. A direct connection destroys it.
 */

#include <SoftwareSerial.h>

SoftwareSerial BT(10, 11);   // HC-05: RX on 10, TX on 11

const int trigPin  = 9;
const int echoPin  = 8;
const int motorPin = 7;      // to motor driver module

const int ALERT_CM = 100;    // alert threshold, centimetres

long readings[3];
int  idx = 0;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motorPin, OUTPUT);
  Serial.begin(9600);
  BT.begin(9600);
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

  // False-alarm filter: three consecutive readings must agree.
  readings[idx] = d;
  idx = (idx + 1) % 3;

  bool tooClose = (readings[0] < ALERT_CM &&
                   readings[1] < ALERT_CM &&
                   readings[2] < ALERT_CM);

  if (tooClose) {
    digitalWrite(motorPin, HIGH);
    BT.print(d);
    BT.println(",ALERT");
  } else {
    digitalWrite(motorPin, LOW);
    BT.print(d);
    BT.println(",OK");
  }

  delay(200);   // five readings per second
}
