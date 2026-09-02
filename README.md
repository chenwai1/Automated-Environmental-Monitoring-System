# 🌿 Automated Environmental Monitoring System

[![ESP32 Examples](https://img.shields.io/badge/ESP32-Official_Examples-70FF00?style=for-the-badge&logo=espressif&logoColor=black)](https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples)
[![Blynk Platform](https://img.shields.io/badge/IoT_Platform-Blynk_Website-00E676?style=for-the-badge&logo=blynk&logoColor=black)](https://blynk.io/)
[![Arduino Download](https://img.shields.io/badge/Software-Arduino_IDE_Download-FFD700?style=for-the-badge&logo=arduino&logoColor=black)](https://www.arduino.cc/en/software)

---

## 📌 $\color{#A6FF00}{\text{Project Overview}}$

```diff
+ Wireless Management: Modern agriculture and home gardening transformed into automated processes.
+ Environmental Balance: Delicate balance of temperature, humidity, and light essential for plants.
+ Proactive Protection: Bridge real-time data collection with smart automated plant protection.
```

---

## 🛠 $\color{#4EF037}{\text{Components Required}}$

```diff
+ [MCU]       ESP32          - Main IoT Microcontroller & Wireless Gateway
+ [Sensor]    DHT11          - Temperature & Humidity Real-time Monitoring
+ [Sensor]    LDR            - Ambient Sunlight & Luminosity Sensing
+ [Display]   LCD 1602 (I2C) - Local Status Telemetry & Reading Display
+ [Actuator]  Servo Motor    - Automated Smart Shading Mechanism
+ [Actuator]  Relay & Pump   - Scheduled & Threshold-based Water Irrigation
```

---

## 📱 $\color{#00E676}{\text{Application and IoT Connectivity}}$

```diff
+ Connected via Blynk IoT Platform
+ Real-time Sensor Telemetry Dashboard
+ Automatic / Manual Control for Spraying & Shading
```

> $\color{#00E676}{\text{Application}}$ **`Blynk`** $\color{#4EF037}{\text{is used to connect directly with the ESP32, enabling smooth remote control of the}}$ $\color{#00E676}{\text{water spraying}}$ $\color{#4EF037}{\text{and}}$ $\color{#009624}{\text{shading system}}$ $\color{#A6FF00}{\text{in real time.}}$

---

## 📚 $\color{#FFD700}{\text{Arduino Libraries Required}}$

```diff
! [Core]      Blynk                  by Blynk
! [Sensor]    DHT sensor library     by Adafruit
! [Actuator]  ESP32Servo             by Kevin Harrington, John K. Bennett
! [Display]   LiquidCrystal I2C      by Frank de Brabander
```

* $\color{#FFD700}{\text{Blynk:}}$ [GitHub Repository](https://github.com/blynkkk/blynk-library)
* $\color{#FFCA28}{\text{DHT Sensor:}}$ [GitHub Repository](https://github.com/adafruit/DHT-sensor-library)
* $\color{#FFB300}{\text{ESP32Servo:}}$ [GitHub Repository](https://github.com/madhephaestus/ESP32Servo)
* $\color{#FFA000}{\text{LiquidCrystal I2C:}}$ [GitHub Repository](https://github.com/johnrickman/LiquidCrystal_I2C)
