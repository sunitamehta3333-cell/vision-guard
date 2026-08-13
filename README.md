# Vision Guard

An integrated assistive mobility kit for visually impaired users: a smart cane and sensor glasses that together cover the full hazard envelope — from the ground to above head height.

The white cane is reliable, cheap, and has barely changed in seventy years, because it is genuinely good at what it does. But it senses only what is on the ground. Overhead hazards — signboards, branches, wall-mounted units, vehicle mirrors — are exactly where head injuries happen. Vision Guard fills that gap without replacing the cane.

---

## Coverage

| Zone | Handled by |
|---|---|
| Ground, kerbs, steps, drains | The cane shaft (mechanical) |
| Waist to chest | Cane-mounted ultrasonic sensor |
| Head level | Sensor glasses |

---

## How it works

An **HC-SR04 ultrasonic sensor** measures distance by emitting a 40 kHz pulse and timing how long the echo takes to return:

```
distance = (echo time × speed of sound) ÷ 2
```

An **Arduino** compares that distance to a threshold and drives a **vibration motor** through a driver module. A three-reading confirmation filter suppresses false alerts from passers-by.

An **HC-05 Bluetooth module** streams live distance readings to an Android app built in MIT App Inventor.

---

## Design principles

- **The cane works without power.** If the battery dies, the user still has a full-length cane. Safety never depends on a charge.
- **Alert less, not more.** A device that beeps at every passer-by gets switched off, and a switched-off device protects nobody. The threshold is deliberately short and filtered.
- **The app is for the family, not the user.** A blind person is not looking at a screen. The app lets a relative confirm the device is working, and provides an SOS.

---

## Bill of materials

### Smart cane

| Component | Qty | Approx. cost (₹) |
|---|---|---|
| Arduino UNO | 1 | 500 |
| HC-SR04 ultrasonic sensor | 1 | 80 |
| DC vibration motor | 1 | 30 |
| Motor driver module | 1 | 60 |
| HC-05 Bluetooth module | 1 | 300 |
| Battery + holder | 1 | 150 |
| On/off switch | 1 | 20 |
| Enclosure, wires, connectors | — | 110 |
| Cane / walking stick | 1 | 100 – 400 |

### Sensor glasses

| Component | Qty | Approx. cost (₹) |
|---|---|---|
| Arduino Nano | 1 | 350 |
| HC-SR04 ultrasonic sensor | 1 | 100 |
| Active buzzer | 1 | 20 |
| Rechargeable battery + pocket unit | 1 | 200 |
| Spectacle frame, wiring, misc. | — | 130 |

**Parts list total ≈ ₹2,300. Our actual first build cost ≈ ₹4,000** — the difference is spares, connectors, and components damaged while learning. Any real build costs more than its parts list.

---

## Wiring

### Cane — sensor and motor

| HC-SR04 pin | Arduino pin |
|---|---|
| VCC | 5V |
| GND | GND |
| Trig | 9 |
| Echo | 8 |

| Motor driver | Arduino pin |
|---|---|
| Signal / IN | 7 |
| VCC | 5V |
| GND | GND |

The motor is **not** driven directly from an Arduino pin — it draws more current than a pin can supply. The driver module switches the motor's own power on a low-power signal from the Arduino.

### Cane — Bluetooth

| HC-05 pin | Arduino pin |
|---|---|
| VCC | 5V |
| GND | GND |
| TXD | 10 |
| RXD | 11 **via a voltage divider** |

**The voltage divider is not optional.** The Arduino outputs 5 V; the HC-05 RXD pin expects 3.3 V. Direct connection works briefly and then destroys the module.

```
Arduino Pin 11 ──[ 1 kΩ ]──┬── HC-05 RXD
                            │
                         [ 2 kΩ ]
                            │
                           GND
```

Pins 10 and 11 are used instead of 0 and 1 so that code can still be uploaded while the module is connected.

---

## Firmware

`cane/cane.ino`

```cpp
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
```

**Serial output format:** `87,ALERT` or `250,OK` — distance in centimetres, comma, status.

**Why the filter matters.** A single ultrasonic reading is noisy, and the HC-SR04's wide cone picks up people walking past. Requiring three consecutive readings below the threshold removes most false alerts, because a passer-by produces one or two anomalous readings rather than three.

---

## Android app

Built in **MIT App Inventor** (`ai2.appinventor.mit.edu`). Source: `app/VisionGuard.aia`

Features:
- Live distance readout from the cane
- Clear / obstacle status panel
- SOS button — sends an SMS with location to a saved contact
- Connection status

Pairing is done in Android Bluetooth settings (passcode `1234`), not inside the app. The app then connects to the already-paired HC-05.

---

## Repository structure

```
vision-guard/
├── cane/
│   └── cane.ino              Arduino sketch for the smart cane
├── glasses/
│   └── glasses.ino           Arduino sketch for the sensor glasses
├── app/
│   └── VisionGuard.aia       MIT App Inventor project file
├── docs/
│   ├── wiring-diagram.png
│   └── build-photos/
└── README.md
```

---

## Known limitations

- Ultrasonic sensors detect thin objects poorly — a single wire or a narrow pole may be missed
- Accuracy degrades in heavy rain
- The cane-mounted sensor moves as the cane is swept, adding noise to readings
- The device supplements, and does not replace, formal Orientation & Mobility training

---

## Planned improvements

- Downward-facing sensor for staircases and drop-offs
- Time-of-flight sensor to improve detection of thin objects
- Adjustable cane length with a marked scale, so one cane fits any user
- Replace the Arduino UNO with a Nano to reduce enclosure size

---

## Status

School project prototype. **Not yet tested with visually impaired users** — that is the next step, and it will likely reveal problems that sighted testing cannot.

---

## Acknowledgements

Cane sizing follows established Orientation & Mobility practice: cane length measured from floor to sternum, or to chin for faster walkers.

---

## License

MIT License — free to use, modify and build upon. If you build one, we would be glad to hear about it.
