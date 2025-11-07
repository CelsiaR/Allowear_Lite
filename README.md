Allowear Lite — IoT-Based Maternal Health Monitoring System

Overview

Allowear Lite is a simulated IoT wearable prototype inspired by SaveMom’s Allowear.
It continuously measures and publishes vital parameters — body temperature, heart rate, HRV, stress level, motion (steps), and baby cry detection — using an ESP32 in a fully online environment.
The system publishes real-time data to a public MQTT broker (HiveMQ) and visualizes it through a Node-RED dashboard.

Tech Stack

| Component       | Platform              |
| --------------- | --------------------- |
| Microcontroller | ESP32 DevKit-C V4     |
| IDE / Build     | PlatformIO (VS Code)  |
| Simulation      | Wokwi ESP32 Simulator |
| Cloud Broker    | HiveMQ Public Broker  |
| Visualization   | Node-RED Dashboard    |
| Protocol        | MQTT (JSON data)      |

Repository Structure

Allowear_Lite/
├─ src/main.cpp                → ESP32 firmware
├─ platformio.ini              → Build configuration
├─ wokwi/diagram.json          → Circuit layout
├─ wokwi/wokwi.toml            → Firmware mapping
├─ node-red/flow.json          → Dashboard flow export
├─ docs/report.pdf             → Full documentation
├─ docs/fig_dashboard.png      → Screenshot of dashboard
├─ build/firmware.bin          → Compiled binary
├─ README.md                   → This file
└─ .gitignore                  → Ignore rules

MQTT Configuration

| Parameter      | Value                |
| -------------- | -------------------- |
| Broker         | `broker.hivemq.com`  |
| Port           | `1883`               |
| Topic          | `celsia/health/data` |
| Protocol       | MQTT v3.1            |
| Authentication | None (public)        |

Example JSON Payload
{
  "temp": 36.8,
  "heart": 74,
  "hrv": 45.2,
  "stress": 0.52,
  "steps": 2035,
  "audio": 0
}

Node-RED Dashboard

Import node-red/flow.json in Node-RED (Menu → Import → Clipboard).

Deploy the flow.

Open http://localhost:1880/ui

View live readings for:

Temperature (Gauge + °C symbol)

Heart Rate (Line Chart)

HRV (ms) (Chart)

Stress (0–1 Gauge)

Steps (Text)

Baby Cry Alert (Notification)

Circuit Overview (Wokwi)

| Sensor                       | ESP32 Pin                    |
| ---------------------------- | ---------------------------- |
| DHT22 (Temp)                 | GPIO 4                       |
| MPU6050 (Motion)             | SDA → GPIO 21, SCL → GPIO 22 |
| Sound Sensor / Potentiometer | GPIO 34                      |
| OLED Display (optional)      | SDA → GPIO 21, SCL → GPIO 22 |
| LED (Alert)                  | GPIO 2                       |
| Power                        | 3V3 & GND (common)           |


Power

3.3 V regulated supply

Current < 500 mA


How to Run Locally

Clone Repo:

git clone https://github.com/<your-username>/Allowear_Lite.git
cd Allowear_Lite


Build Firmware:

pio run


Simulate in Wokwi:

Open wokwi.toml → verify firmware path

Click Run Simulation

Start Node-RED:

node-red


View Dashboard:
Navigate to http://localhost:1880/ui


Results

Real-time data published to HiveMQ.

Node-RED gauges and charts show live sensor variations.

Baby-cry alert triggers visual notification.

HRV and stress graphs reflect dynamic physiological changes.


References

SaveMom Allowear Product : https://savemom.in/products/allowear#features

HiveMQ Public Broker : https://www.hivemq.com/demos/websocket-client/

Node-RED Dashboard : https://nodered.org/

Wokwi ESP32 Simulator : https://wokwi.com/

PlatformIO Docs : https://platformio.org/


Author:

Celsia
Email: celsia2092002@gmail.com