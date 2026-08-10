# AI-Powered Sustainable Low-Power IoT System for Crop Recommendation and Plant Stress Monitoring

This repository contains the final code deployed to an AI embedded system created during the USDA REEU @ UTSA of Summer 2026. The REEU (Undergraduate Research and Extension Experience) hosted by the US Department of Agriculture focused on how to implement AI and automation for smart agriculture.

This project aims to utilize AI for a cheap, reliable, and sustainable system that may determine the suitability of crops in a specific field, as well as to monitor the crop stress of what is currently planted. With the combination of commercial sensors, RF communication, a Raspeberry Pi 4b, and a TI MSP430FR5994 microcontroller, two AI models are deployed. This repository will detail aspects of the project, as well as document the process of reproducing the system yourself.

### Notice
This project utilized generative AI for coding and debugging throughout the 8-week program. The provided release of this project should be considered unstable and for research purposes only. Any derivation of this project should be hardened and reviewed by human professionals before any commercial implementation.

## 📑 Quick Links
  - [Problem and Objectives](#-problem-and-objectives)
  - [System Overview](#-system-overview)
  - [Materials](#-materials)
  - [Implementation](#-implementation)
  - [Expected Results](#-expected-results)
  - [Troubleshooting](#-troubleshooting)
  - [Acknowlegements](#acknowlegements)

## 🎯 Problem and Objectives 
This project's main objectives are sustainability, reliability, and affordability. The problem being tackled is the need for precision agriculture and AI technology to be infused into current agriculture practices. To be sustainable the system must be simple to repair, run off minimal resources from the existing infrastructure, and be scalable. To be reliable the system must account for its own shortfalls and maintain a high accuracy in its predictions. To maintain affordability the system must be built upon technologies that are either already relatively cheap or whose industries are trending downward in cost. Above all the system must provide a useful service to the end user to justify its deployment.

## 🔍 System Overview

The system produced runs two AI models, with one crop recommendation model on the MSP430FR5994, and one crop stress detection model on the Raspberry Pi 4b. The microcontrollers communicate to each other using the TI CC1101 radio modules. The entire system is built around a simple 12V solar panel and battery setup, to maintain the sustainability goal of the project. To prevent catastrophic power loss from ruining predictions or corrupting data transmissions, checkpointing code is implemented to save all data on the MSP430's SRAM into its FRAM. When power is sustained again, any data saved is recovered and operations continue. The MSP430 collects the following data from its sensors, runs its own prediction with it, then sends the data to the Raspberry Pi for its prediction:

```
Nitrogen (mg/kg), Potassium (mg/kg), Phosphorus (mg/kg), Light-Level (lux), 
pH ([H+]/mol dm^-3), Temperature (°C), Humidity (%), Soil Moisture (%)
```
### System Diagram

<img width="1493" height="731" alt="System_Pinout drawio" src="https://github.com/user-attachments/assets/066c8527-af0a-4564-983a-5b8b31d72a5b" />

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


# 🛠️ Implementation

The implementation of this project will require setup of hardware and software across two seperate platforms, with dozens of physical connections and parameters to match up. To make these instructions easier to follow, they will be broken up into sections. At the end of each section there will be a progress check to verify that everything is working as intended. If at any check there are errors, please refer to [#Troubleshooting](#-troubleshooting) for guidance.

## MSP430FR5994 Software Setup

### Step 1: Download the Latest Release
Navigate to the releases tab of this repository and download the latest option. Alternatively, click [here](https://www.github.com/) to start the download.

### Step 2: Download TI Code Composer Studio (CCS)
For deploying this project it is recommended to download [version 12.8.1](https://www.ti.com/tool/download/CCSTUDIO/12.8.1), as this is the developement environment used during production. Make sure to download the version appropriate for your operating system. Supported for Windows, Linux, and macOS.

### Step 3: Setup the CCS project
Launch the CCS installation script and follow the prompts to complete installation. Once the software is installed, launch it and explore the interface. To run the code in this IDE we will need to create a new project. To do so, navigate to the top-left titlebar and click:

> File > New > CCS Project

The configuration window will then popup. Under the "Target" field, type in "MSP430FR5994" and click the corresponding option in the list to the right of the field. If prompted, create a workspace. Name your project, verify the configuration, then click Finish.

<img width="628" height="380" alt="CCS_Project_Setup" src="https://github.com/user-attachments/assets/47691dae-cfee-47ad-b843-31a6ef72d0f0" />

### Step 4: Import the MSP430 Code

Locate the Project Explorer on the left-most pane and open the new CCS project. From the files downloaded from the latest release, navigate into the "MSP430-FR5994 Files" folder. Select every file within this folder, and drag it into the new CCS project. Once all of the files populate the Project Explorer, double click "main.cpp". This is the primary file that will be flashed to the MSP430FR5994. For now, you can safely delete the autogenerated "main.c" from the project, leaving everything else. Your workspace should now look like this:

<img width="1770" height="1050" alt="Import_MSP430_Code" src="https://github.com/user-attachments/assets/7909745a-44ca-4457-b94a-f7b7c4bd7806" />

Finally, we need to change how the project is compiled by the software. The C Dialect will need to be set to "Compile Program in C99 mode", allowing the code to execute many of its internal loops. This setting is located by right-clicking your project in the Project Explorer, and following these menus:

> Right-Click > Properties > Build > MSP430 Compiler > Advanced Options > Language Options

### Progress Check: Flashing the MSP430

To verify that Code Composer Studio and the MSP430FR5994 code have been setup correctly, it is best to test flashing the code to the MSP430 board without any sensors connected first. Plug in the MSP430 board with a MicroUSB cable to your computer, and verify that an LED has turned on indicating power. Also verify that the headers connecting the top and bottom of the board and firmly seated. Without these headers, the programmer section of the board will not be able to communicate with the rest of the board, resulting in failure to flash:

<img width="118" height="60" alt="Flash" src="https://github.com/user-attachments/assets/cf959aac-bbfc-477c-b218-68d654d2f11d" />

With the hardware connected, right-click the project and select "Build Project", or click the hammer icon in the quick-action bar to build the project. This will tell CCS to verify that there are no fatal errors in the code. In the Console, you should see "Build Finished" with no errors. It is ok if warnings appear, so long as no fatal errors are detected. Once the build is complete, navigate back to the quick-action bar and click the folder icon to flash the project to the MSP430 board. If no errors appeared during the build, and no errors appear during the flash, then the project is successfully setup. 

<img width="426" height="259" alt="headers" src="https://github.com/user-attachments/assets/139980b8-2f79-4038-ba28-fe38438975ba" />
  
## MSP430FR5994 Hardware Setup

### Step 1: Setup RS485 Address Programmer Project

Before getting tangled in dozens of wires between the sensors, MSP430, and breadboards, the two RS485 sensors need to be re-programmed to properly communicate with the MSP430 board. Included in the latest release download is the "RS485 Address Programmer" folder, with one "main.c" file inside. Similarly to the primary project we just created, we are going to create a New CCS Project, as well as configure its C Dialect to be "Compile Program in C99 mode". Once this is setup, drag the main.c file into the project directory, overwriting the autogenerated main.c file the project created. Locate this code block at the top of the file:

```c++
/* =====================================================================
 * CONFIGURATION BLOCK
 * ===================================================================== */
const uint8_t CURRENT_ADDRESS   = 0x01;    // Previous or Factory default address
const uint8_t NEW_ADDRESS       = 0x02;    // Desired new Modbus address
const uint16_t ADDRESS_REGISTER = 0x0100; // Equipment address register (0x0100)
```
To use this programmer these three config values will need to be changed based on our needs. Make a note that the desired address for the NPK Sensor is `0x01`, and for the PH Sensor is `0x02`.

### Step 2: Hookup the MAX485 Board and PH Sensor

When working with the MSP430 board, make sure it is always unplugged before connecting any pins, wires, or sensors. Following the [system diagram](#system-diagram), connect the following pins on the MAX485 Board:

> DE → P4.2 | RE → P4.1 | DI → P6.0 | RO → P6.1 

Next, make sure the 12V Power Adapter is unplugged. Once verified, check the wires of the PH sensor. There should be 4 total wires, each of varying colors. Note that the colors of each wire are not standard, and as such you must double-check the manual that came with the sensor to properly label each wire. The common convention assumed in this project is the following:

> Black → Ground | Brown → Voltage | Yellow → A Line | Blue → B Line

Connect the voltage line to the positive end of the power adapter, and the ground line to the negative end of the power adapter. Connect the A and B lines to the A and B receptacles on the MAX485 Board. Using Dupont wires, connect one end into the negative end of the power adapter, and the other end into a breadboard. Using the breadboard, connect the GND pin of the MAX485 Board to the ground line of the power adapter. This ensures common ground between the RS485 sensors and the MAX485 Board, which is important for maintaining signal integrity. Connect the 3v3 pin on the right-side of the MSP430 to the breadboard powerrail. Connect the VCC pin on the MAX485 Board to this powerrail. Finally, use a small screwdriver to tighten the clamps on the MAX485 Board's A and B receptacles, as well as on the 12V adapter. 

> 🛑 DO NOT USE 5V FOR ANY COMPONENTS IN THIS PROJECT, AS THEY CAN BE DAMAGED. 

### Step 3: Reprogram the PH Sensor

Before plugging in either the MSP430 or the 12V adapter, make sure all pins are properly seated and clamped down tight. Also, check the manuals for both the NPK and PH sensor to determine what the factory default address is for each sensor. Typically this is `0x01`, which will be assumed for this project. Because the NPK sensor's address is already correct, we just need to program the PH sensor's address to `0x02`.

Once the wiring and manuals have been verified, connect the MSP430 to the computer and plug in the 12V wall adapter. In Code Composer Studio, navigate to the titlebar and click `View > Terminal`. This will open a terminal pane within the window. Before configuring this terminal to show the output of the programmer, we need to know which COM port the MSP430's UART is communicating through. The quick way to determine this is to open Device Manager in Windows, navigate to COM ports, and then un-plug and re-plug in the MSP430. This will show two COM ports appearing, such as COM11 and COM12. Device Manager should give descriptive names for these COM ports, with the UART port being of interest. With this port identified, open a new terminal in CCS with `CTRL+SHIFT+ALT+T` or with the quick-action bar. Use the following configuration: 

> Terminal : Serial | Baud rate : 9600 | Data size : 8 | Parity : none | Stop bits : 1 | Encoding: Default

With the terminal setup, make sure the `CURRENT_ADDRESS = 0x01` and `NEW_ADDRESS = 0x02`. Refer to the sensor manual to verify that `ADDRESS_REGISTER = 0x0100`. Finally, flash the code to the MSP430 and watch the terminal for confirmation. To quickly re-run the program, press the RESET button on the MSP430. If all is correct the terminal should output:

```
--- RS485 Sensor Re-Addressing Tool ---
Target Register:  0x0100
Previous Address: 0x01
New Address:      0x02
Sending Modbus Write Command to Sensor...
Address update command sent successfully!
```

Once the PH sensor is properly addressed, flash the main project from "main.cpp" back onto the MSP430 board. If this is not done, then the next time the board is connected to power with both RS485 sensors plugged in it will address both of them as `0x02`, and the NPK sensor will need to be re-addressed as `0x01` to fix this mistake.

### Step 4: Connect the TI-CC1101 Radio Module

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
