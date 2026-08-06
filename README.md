# AI-Powered Sustainable Low-Power IoT System for Crop Recommendation and Plant Stress Monitoring

This repository contains the final code deployed to an AI embedded system created during the USDA REEU @ UTSA of Summer 2026. The REEU (Undergraduate Research and Extension Experience) hosted by the US Department of Agriculture focused on how to implement AI and automation for smart agriculture.

This project aims to utilize AI for a cheap, reliable, and sustainable system that may determine the suitability of crops in a specific field, as well as to monitor the crop stress of what is currently planted. With the combination of commercial sensors, RF communication, a Raspeberry Pi 4b, and a TI MSP430FR5994 microcontroller, two AI models are deployed. This repository will detail aspects of the project, as well as document the process of reproducing the system yourself.

### Notice
This project utilized generative AI for coding and debugging throughout the 8-week program. The provided release of this project should be considered unstable and for research purposes only. Any derivation of this project should be hardened and reviewed by human professionals before any commercial implementation.

## 📑 Quick Links
  - [Problem and Objectives](#problem-and-objectives)
  - [System Overview](#system-overview)
  - [Materials](#materials)
  - [Implementation](#implementation)
  - [Expected Results](#expected-results)
  - [Troubleshooting](#troubleshooting)
  - [Acknowlegements](#acknowlegements)

## 🎯 Problem and Objectives 
Description here.

## 🔍 System Overview

The system produced runs two AI models, with one crop recommendation model on the MSP430FR5994, and one crop stress detection model on the Raspberry Pi 4b. The microcontrollers communicate to each other using the TI CC1101 radio modules. The entire system is built around a simple 12V solar panel and battery setup, to maintain the sustainability goal of the project. To prevent catastrophic power loss from ruining predictions or corrupting data transmissions, checkpointing code is implemented to save all data on the MSP430's SRAM into its FRAM. When power is sustained again, any data saved is recovered and operations continue. The MSP430 collects the following data from its sensors, runs its own prediction with it, then sends the data to the Raspberry Pi for its prediction:

```
Nitrogen (mg/kg), Potassium (mg/kg), Phosphorus (mg/kg), Light-Level (lux), pH ([H+]/mol dm^-3), Temperature (°C), Humidity (%), Soil Moisture (%)
```

(System Diagram)

## 📦 Materials
- [1] MSP430FR5994 
- [1] Raspberry Pi 4b
- [2] TI CC1101 Radio Module
- [1] RS485 Soil NPK Sensor 
- [1] RS485 Soil PH Sensor
- [1] MAX485 Expansion Board
- [1] Generic Soil Moisture Sensor 
- [1] DHT22 Temperature & Humidity Sensor
- [1] Adafruit TSL2591 Light Sensor
- [1] Breadboard
- [1] Assorted Dupont Wires
- [1] 12V DC Power Adapter [(Example)](https://a.co/d/06kayYQV)


## 🛠️ Implementation
Here is the instructions to reconstruct the project shared here.

## 📊 Expected Results
Here is examples of what the constructed system should look like

## 🧹 Troubleshooting
Here is a list of common problems you may face while building this project.

### Problem Sub-Header
- Step by Step fix

## Acknowlegements
Thank you to [SpaceTeddy](https://github.com/SpaceTeddy/CC1101) for providing the CC1101 module code for the Raspberry Pi. Thank you to [abhra0897](https://github.com/abhra0897/msp430_cc1101_energia_v2) for providing the initial CC1101 module code for the MSP430. Finally, thank you to the mentors of the REEU program and their graduate students for their collaboration and guidance.
