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
This project's main objectives are sustainability, reliability, and affordability. The problem being tackled is the need for precision agriculture and AI technology to be infused into current agriculture practices. To be sustainable the system must be simple to repair, run off minimal resources from the existing infrastructure, and be scalable. To be reliable the system must account for its own shortfalls and maintain a high accuracy in its predictions. To maintain affordability the system must be built upon technologies that are either already relatively cheap or whose industries are trending downward in cost. Above all the system must provide a useful service to the end user to justify its deployment.

## 🔍 System Overview

The system produced runs two AI models, with one crop recommendation model on the MSP430FR5994, and one crop stress detection model on the Raspberry Pi 4b. The microcontrollers communicate to each other using the TI CC1101 radio modules. The entire system is built around a simple 12V solar panel and battery setup, to maintain the sustainability goal of the project. To prevent catastrophic power loss from ruining predictions or corrupting data transmissions, checkpointing code is implemented to save all data on the MSP430's SRAM into its FRAM. When power is sustained again, any data saved is recovered and operations continue. The MSP430 collects the following data from its sensors, runs its own prediction with it, then sends the data to the Raspberry Pi for its prediction:

```
Nitrogen (mg/kg), Potassium (mg/kg), Phosphorus (mg/kg), Light-Level (lux), 
pH ([H+]/mol dm^-3), Temperature (°C), Humidity (%), Soil Moisture (%)
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
With the system setup with no errors during the flashing process, these are some results you can expect to get. On the MSP430 side:

```
--------------------------------------------
TSL2591 Light -> Full Spectrum: 0x105B | Infrared: 0x30E
Soil Moisture -> Raw ADC: 2655 | Calculated: 8%
NPK Sensor -> N: 3 mg/kg | P: 1 mg/kg | K: 7 mg/kg
pH Sensor -> Soil pH Value: 7.71
DHT22 Climate -> Temp: 22.5 C | Humidity: 60.4%
Syst. Voltage -> 3300 mV
AI Model -> Predicted Crop: Mango (Class 12)
--------------------------------------------
```

On the Raspberry Pi side:

<img width="1919" height="1027" alt="Pi4-Dashboard" src="https://github.com/user-attachments/assets/152c3603-4070-410a-a70c-616f5abda0b9" />

## 🧹 Troubleshooting
Due to the nature of this system being multimodal, errors in deployment can show up in every single moving component and in many different ways. To avoid as many of these errors as possible, it is highly recommended to follow the instructions provided as closely as possible. Whenever an error occurs, follow this general procedure for an easier diagnosis:

1. If CCS or the terminal provided an error, copy it and search it online. While obvious, many simple errors can be caught and fixed from online resources.

2. Check the physical wiring of your components. Make sure that the data lines to your components are the same data lines being checked in the code. Follow the provided physical pinouts if you wish to use the code without modifying internal pinouts.

3. Make sure your components' voltage and ground connections are in the proper power rails.

4. Double check the GROUNDING of the components. While this was just stated above, it is such a simple yet time-consuming mistake to make that it is worth double and triple checking the GROUND. Again, check the ground wires!

5. If the software is running but no results are on your screen, make sure your serial terminal is active and tuned to the correct COM port and Baud Rate. The default Baud Rate is 9600 for the MSP430.

6. If the MSP430 code is failing to compile with errors similar to "i is not defined" or anything relating to the loop functions, try setting the MSP430 Compiler -> Advanced -> Language Settings to "Compile in C99 mode".

7. For sensor-specific errors, consult their manuals for registeries and hardware identification. This is especially important for the RS485 sensors, as their registers need to be in sync with the code to properly communicate.

## Acknowlegements
Thank you to [SpaceTeddy](https://github.com/SpaceTeddy/CC1101) for providing the CC1101 module code for the Raspberry Pi. Thank you to [abhra0897](https://github.com/abhra0897/msp430_cc1101_energia_v2) for providing the initial CC1101 module code for the MSP430. Finally, thank you to the mentors of the REEU program and their graduate students for their collaboration and guidance.
