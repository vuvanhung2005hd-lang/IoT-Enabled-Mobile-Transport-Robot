# 🚗 IoT-Enabled Mobile Transport Robot

## 📖 Overview

This project is an IoT-enabled mobile transport robot controlled from a smartphone over Wi-Fi. It was developed for the **Student Scientific Research Competition** at Hanoi University of Industry (HaUI).

An ESP32 microcontroller runs a Wi-Fi access point and a web server, so a phone browser acts as the remote control with low command latency (about 100–200 ms). An HC-SR04 ultrasonic sensor, read through a hardware interrupt on its ECHO pin, provides automatic emergency braking when the robot drives forward: it stopped in 10 out of 10 approach trials in the tested indoor setup.

---

## ✨ Key Features

- **Real-Time Motion Control:** Four-wheel robot driven by an ESP32, an L298N motor driver and differential steering.
- **Obstacle Detection & Emergency Braking:** Interrupt-driven HC-SR04 measurement (ECHO pulse timed in an ISR, at most one measurement every 60 ms). While driving forward, the robot brakes and latches into STOP when the distance falls below 25 cm.
- **Wi-Fi Client-Server Control:** The ESP32 hosts a web interface for navigation and PWM speed control from any smartphone browser.
- **Embedded Web Interface:** Custom HTML5/CSS3 dashboard served directly by the ESP32 (press-and-hold direction buttons, speed slider, live distance readout), so no dedicated mobile app is needed.
- **Serial Logging:** Distance readings and braking events are printed over Serial (115200 baud) for debugging and performance evaluation.
<!-- TODO: the firmware you shared prints plain-text lines, not CSV with timestamps/PWM/state. If another build logs CSV, restore the original telemetry bullet for that build. -->

---

## 🧩 System Architecture

<img width="1280" height="797" alt="image" src="https://github.com/user-attachments/assets/2a233618-6595-46cb-b45e-3bf7d10e1d1c" />

---

## 🛠️ Hardware Architecture

The robot uses a four-wheel platform with differential steering. The left and right wheel pairs are driven by an L298N dual H-bridge.

| Component | Specification & Function |
|---|---|
| **Microcontroller** | ESP32 development board, dual-core Xtensa LX6 up to 240 MHz; handles Wi-Fi, web server requests and motion control |
| **Motor Driver** | L298N dual H-bridge for bidirectional DC motor control and PWM speed adjustment |
| **Actuators** | DC gear motors in a four-wheel configuration with differential steering |
| **Ultrasonic Sensor** | HC-SR04, nominal range 2–400 cm, 40 kHz |
| **Power Supply** | 7.4 V 2S Li-ion pack, about 60 minutes of continuous operation |
| **Voltage Regulator** | DC-DC buck converter for the logic supply |
| **Chassis** | Four-wheel acrylic (mica) platform, about 650 g total mass |

> **Hardware note:** the HC-SR04 ECHO output is 5 V, while ESP32 GPIOs are 3.3 V and not 5 V-tolerant. If the sensor is powered at 5 V, level-shift ECHO (for example with a 1 kΩ / 2 kΩ divider) before GPIO 18.
> <!-- TODO: confirm whether a divider is already in your build and document it. -->

---

## 📐 Pin Mapping (ESP32)

| ESP32 GPIO | Component Pin | Function |
|---|---|---|
| GPIO 33 | L298N ENA | Left motor PWM speed control |
| GPIO 32 | L298N ENB | Right motor PWM speed control |
| GPIO 14 | L298N IN1 | Left motor direction control |
| GPIO 27 | L298N IN2 | Left motor direction control |
| GPIO 26 | L298N IN3 | Right motor direction control |
| GPIO 25 | L298N IN4 | Right motor direction control |
| GPIO 4 | HC-SR04 TRIG | Ultrasonic trigger pulse output |
| GPIO 18 | HC-SR04 ECHO | Echo signal input |

```cpp
// L298N Motor Driver
const int ENA = 33;  // Left motor PWM
const int ENB = 32;  // Right motor PWM
const int IN1 = 14;  // Left motor direction
const int IN2 = 27;  // Left motor direction
const int IN3 = 26;  // Right motor direction
const int IN4 = 25;  // Right motor direction

// HC-SR04 Ultrasonic Sensor
const int TRIG_PIN = 4;   // Trigger output
const int ECHO_PIN = 18;  // Echo input
```

---

## 📊 Experimental Results

The robot was evaluated indoors. Results below are observed values under the tested conditions only.

| Metric | Result |
|---|---|
| **Command Execution Success Rate** | 98% across forward, backward, turning and stop commands |
| **Emergency Braking** | Stopped in 10/10 trials at a maximum speed of about 0.8 m/s |
| **Average Stopping Distance** | 16.4 cm |
| **Operational Communication Range** | Up to 10 m |
| **Command Response Time** | About 100–200 ms |
| **Continuous Operating Time** | About 60 minutes with the tested battery |

<!-- TODO: add (1) number of command trials behind the 98%, (2) how stopping distance was measured (distance travelled after the 25 cm trigger, or remaining gap to the obstacle; also confirm the 10/10 test ran with SAFE_DISTANCE = 25), (3) how response time was measured, (4) test surface, obstacle material/angle and battery level, (5) battery capacity (mAh). -->

### Emergency Braking Test

The robot approached an obstacle at its maximum speed in 10 trials and stopped every time, with an average stopping distance of 16.4 cm.

*Note: 10 trials is a small sample. These results reflect the tested indoor conditions and do not guarantee collision avoidance in all scenarios (for example angled or sound-absorbing obstacles, which ultrasonic sensors detect poorly).*

### Braking Behaviour & Limitations

- Braking is active only while driving **forward**; reversing and turning are not monitored.
- After braking, the robot latches into STOP and needs a new FORWARD command. If the obstacle is still within 25 cm it brakes again immediately, so back away first.
- A measurement is triggered at most every 60 ms (about 4.8 cm of travel at 0.8 m/s); echoes longer than 25 ms are treated as "no obstacle".
- Direction buttons are press-and-hold and rely on the release event to stop. There is no timeout if the Wi-Fi connection is lost mid-press.
- If the sensor stops responding, the last measured distance is kept and braking will not trigger.

---

## 🚀 Getting Started

### Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software)
- ESP32 Arduino core **3.x** (via Boards Manager); the firmware uses the 3.x LEDC API (`ledcAttach`, `ledcWrite(pin, duty)`) and will not compile on 2.x
- Libraries bundled with the ESP32 core: `WiFi.h`, `WebServer.h`
- An ESP32 development board and a Wi-Fi-enabled smartphone

### Installation & Flashing

1. **Clone the repository**

   ```bash
   git clone <repository-url>
   ```

2. **Open the project:** open `Code_control.ino` in the Arduino IDE.

3. **Configure the board:** select your ESP32 board model and the correct COM port.

4. **Upload the firmware:** connect the ESP32 and click **Upload**.

5. **Connect to the robot:** power it on and join its Wi-Fi access point.

   | Setting | Value |
   |---|---|
   | **SSID** | `My_car` |
   | **Password** | `12345678` |

   > The default credentials are for demonstration. Change them before using the robot in a shared environment; the web interface has no authentication.

6. **Open the control interface:** browse to `http://192.168.4.1` on your phone.

---

## 🔮 Future Improvements

- **Real-Time Task Scheduling:** use FreeRTOS tasks to separate Wi-Fi handling, sensing and motor control.
- **Machine Learning for Collision Prediction:** add structured CSV logging (timestamp, distance, PWM speed, state) and analyse it in Python for data-driven collision risk estimation.
- **Enhanced Obstacle Detection:** add more ultrasonic sensors or LiDAR for wider coverage.
- **Control Performance Optimization:** measure end-to-end command latency and tune the control loop.
- **Fail-safe Behaviour:** stop the motors when no command arrives within a timeout, and extend obstacle checks to reversing and turning.
- **Security:** add authentication and a configurable password for the control interface.

---

## 👥 Authors

**Vu Van Hung** — Embedded Firmware & System Integration

**Team Members**
- Bui Xuan Sang
- Phan Van Lam
- Vo Huu Hoang Anh
- Tran Thai Duong

*Developed as a student scientific research project at Hanoi University of Industry (HaUI).*

---

## 🏆 Awards

**3rd Prize — University-Level Student Scientific Research Competition**
Hanoi University of Industry (HaUI)

<p align="center">
  <img width="720" alt="IoT-Enabled Mobile Transport Robot" src="https://github.com/user-attachments/assets/492ff71c-307d-47a4-a345-696ab6d98818" />
</p>
