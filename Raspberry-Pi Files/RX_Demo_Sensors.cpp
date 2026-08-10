// RX_Demo_Sensors.cpp
//
// Raspberry Pi 4 CC1101 receiver for the exact 18-byte payload transmitted
// by the MSP430 main.cpp.
//
// This version preserves all raw values AND converts scaled integers into
// the floating-point format expected by the nutrient HAT.
//
// Examples:
//   temperature raw 235 -> 23.5 C
//   humidity raw 552    -> 55.2 %
//   pH raw 658          -> 6.58
//
// No value is deleted. Raw and converted values are both logged.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <wiringPi.h>
#include "cc1100_raspi.h"

namespace {

constexpr std::size_t RX_BUFFER_SIZE = 64;
constexpr std::size_t SENSOR_PAYLOAD_SIZE = 18;

CC1100 cc1100;
volatile bool packet_ready = false;

uint8_t rx_fifo[RX_BUFFER_SIZE] = {0};
uint8_t packet_length = 0;
uint8_t receiver_address = 0;
uint8_t sender_address = 0;
int8_t rssi_dbm = 0;
uint8_t lqi = 0;

volatile uint8_t my_address = 0x03;
uint8_t expected_sender = 0x01;
uint8_t channel = 10;
uint8_t ism_setting = 2;   // 433/434 MHz
uint8_t mode_setting = 1;  // GFSK 1.2 kbps

uint16_t read_be_u16(const uint8_t* bytes) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(bytes[0]) << 8) |
        static_cast<uint16_t>(bytes[1])
    );
}

int16_t read_be_i16(const uint8_t* bytes) {
    return static_cast<int16_t>(read_be_u16(bytes));
}

double tenths_to_double(int16_t raw_value) {
    return static_cast<double>(raw_value) / 10.0;
}

double unsigned_tenths_to_double(uint16_t raw_value) {
    return static_cast<double>(raw_value) / 10.0;
}

double hundredths_to_double(uint16_t raw_value) {
    return static_cast<double>(raw_value) / 100.0;
}

std::string bytes_to_hex(const uint8_t* bytes, std::size_t count) {
    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            output << ' ';
        }
        output << std::setw(2) << static_cast<unsigned>(bytes[i]);
    }

    return output.str();
}

uint8_t parse_frequency(const char* value) {
    switch (std::atoi(value)) {
        case 315: return 1;
        case 433:
        case 434: return 2;
        case 868: return 3;
        case 915: return 4;
        default:
            throw std::runtime_error(
                "Unsupported frequency. Use 315, 433/434, 868, or 915."
            );
    }
}

uint8_t parse_modulation(const char* value) {
    switch (std::atoi(value)) {
        case 1:   return 1;
        case 38:  return 2;
        case 100: return 3;
        case 250: return 4;
        case 500: return 5;
        case 4:   return 6;
        default:
            throw std::runtime_error(
                "Unsupported modulation. Use 1, 38, 100, 250, 500, or 4."
            );
    }
}

void radio_interrupt() {
    if (packet_ready) {
        return;
    }

    if (cc1100.packet_available()) {
        packet_length = 0;
        receiver_address = 0;
        sender_address = 0;
        rssi_dbm = 0;
        lqi = 0;

        cc1100.get_payload(
            rx_fifo,
            packet_length,
            receiver_address,
            sender_address,
            rssi_dbm,
            lqi
        );

        packet_ready = true;
    }
}

const uint8_t* find_payload() {
    // Handles either an 18-byte payload-only buffer or the common
    // SpaceTeddy framed buffer: [length, receiver, sender, payload...].
    if (packet_length == SENSOR_PAYLOAD_SIZE) {
        return &rx_fifo[0];
    }

    if (packet_length >= SENSOR_PAYLOAD_SIZE + 2) {
        return &rx_fifo[3];
    }

    if (rx_fifo[0] >= SENSOR_PAYLOAD_SIZE + 2) {
        return &rx_fifo[3];
    }

    return nullptr;
}

void emit_packet(const uint8_t* payload) {
    // Decode every raw 16-bit value exactly as transmitted.
    const uint16_t moisture_raw = read_be_u16(payload + 0);
    const int16_t ambient_temperature_raw = read_be_i16(payload + 2);
    const int16_t soil_temperature_raw = read_be_i16(payload + 4);
    const uint16_t humidity_raw = read_be_u16(payload + 6);
    const uint16_t light_raw = read_be_u16(payload + 8);
    const uint16_t ph_raw = read_be_u16(payload + 10);
    const uint16_t nitrogen_raw = read_be_u16(payload + 12);
    const uint16_t phosphorus_raw = read_be_u16(payload + 14);
    const uint16_t potassium_raw = read_be_u16(payload + 16);

    // Convert only according to the scale used by the MSP430 transmitter.
    const double soil_moisture =
        static_cast<double>(moisture_raw);

    const double ambient_temperature =
        tenths_to_double(ambient_temperature_raw);

    const double soil_temperature =
        tenths_to_double(soil_temperature_raw);

    const double humidity =
        unsigned_tenths_to_double(humidity_raw);

    // main.cpp sends current_light_full directly, so no divisor is applied.
    const double light_intensity =
        static_cast<double>(light_raw);

    const double soil_ph =
        hundredths_to_double(ph_raw);

    const double nitrogen =
        static_cast<double>(nitrogen_raw);

    const double phosphorus =
        static_cast<double>(phosphorus_raw);

    const double potassium =
        static_cast<double>(potassium_raw);

    // Archive all raw values and all converted values.
    std::ostringstream raw_json;
    raw_json.imbue(std::locale::classic());
    raw_json << std::fixed << std::setprecision(2);

    raw_json
        << "{"
        << "\"sender_address\":" << static_cast<unsigned>(sender_address) << ","
        << "\"receiver_address\":" << static_cast<unsigned>(receiver_address) << ","
        << "\"packet_length\":" << static_cast<unsigned>(packet_length) << ","
        << "\"rssi_dbm\":" << static_cast<int>(rssi_dbm) << ","
        << "\"lqi\":" << static_cast<unsigned>(lqi) << ","
        << "\"payload_hex\":\"" << bytes_to_hex(payload, SENSOR_PAYLOAD_SIZE) << "\","

        << "\"Soil_Moisture_raw\":" << moisture_raw << ","
        << "\"Ambient_Temperature_raw\":" << ambient_temperature_raw << ","
        << "\"Soil_Temperature_raw\":" << soil_temperature_raw << ","
        << "\"Humidity_raw\":" << humidity_raw << ","
        << "\"Light_Intensity_raw\":" << light_raw << ","
        << "\"Soil_pH_raw\":" << ph_raw << ","
        << "\"Nitrogen_Level_raw\":" << nitrogen_raw << ","
        << "\"Phosphorus_Level_raw\":" << phosphorus_raw << ","
        << "\"Potassium_Level_raw\":" << potassium_raw << ","

        << "\"Soil_Moisture_converted\":" << soil_moisture << ","
        << "\"Ambient_Temperature_converted\":" << ambient_temperature << ","
        << "\"Soil_Temperature_converted\":" << soil_temperature << ","
        << "\"Humidity_converted\":" << humidity << ","
        << "\"Light_Intensity_converted\":" << light_intensity << ","
        << "\"Soil_pH_converted\":" << soil_ph << ","
        << "\"Nitrogen_Level_converted\":" << nitrogen << ","
        << "\"Phosphorus_Level_converted\":" << phosphorus << ","
        << "\"Potassium_Level_converted\":" << potassium
        << "}";

    // Exact nine-feature dictionary expected by predict_and_log.py/HAT.
    std::ostringstream model_json;
    model_json.imbue(std::locale::classic());
    model_json << std::fixed << std::setprecision(2);

    model_json
        << "{"
        << "\"Soil_Moisture\":" << soil_moisture << ","
        << "\"Ambient_Temperature\":" << ambient_temperature << ","
        << "\"Soil_Temperature\":" << soil_temperature << ","
        << "\"Humidity\":" << humidity << ","
        << "\"Light_Intensity\":" << light_intensity << ","
        << "\"Soil_pH\":" << soil_ph << ","
        << "\"Nitrogen_Level\":" << nitrogen << ","
        << "\"Phosphorus_Level\":" << phosphorus << ","
        << "\"Potassium_Level\":" << potassium
        << "}";

    std::cout << "RAW_PACKET_JSON:" << raw_json.str() << std::endl;
    std::cout << "SENSOR_JSON:" << model_json.str() << std::endl;

    std::fprintf(
        stderr,
        "[CONVERTED] moisture=%.2f temp1=%.2f temp2=%.2f "
        "humidity=%.2f light=%.2f pH=%.2f N=%.2f P=%.2f K=%.2f\\n",
        soil_moisture,
        ambient_temperature,
        soil_temperature,
        humidity,
        light_intensity,
        soil_ph,
        nitrogen,
        phosphorus,
        potassium
    );
}

}  // namespace

int main(int argc, char** argv) {
    int option = 0;

    try {
        while ((option = getopt(argc, argv, "ha:s:c:f:m:")) != -1) {
            switch (option) {
                case 'h':
                    std::fprintf(
                        stderr,
                        "Options: -a receiver -s sender -c channel "
                        "-f frequency -m modulation\\n"
                    );
                    return EXIT_SUCCESS;

                case 'a':
                    my_address = static_cast<uint8_t>(std::atoi(optarg));
                    break;

                case 's':
                    expected_sender = static_cast<uint8_t>(std::atoi(optarg));
                    break;

                case 'c':
                    channel = static_cast<uint8_t>(std::atoi(optarg));
                    break;

                case 'f':
                    ism_setting = parse_frequency(optarg);
                    break;

                case 'm':
                    mode_setting = parse_modulation(optarg);
                    break;

                default:
                    return EXIT_FAILURE;
            }
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Argument error: %s\\n", error.what());
        return EXIT_FAILURE;
    }

    if (wiringPiSetup() == -1) {
        std::fprintf(stderr, "wiringPiSetup() failed.\\n");
        return EXIT_FAILURE;
    }

    cc1100.begin(my_address);
    cc1100.set_mode(mode_setting);
    cc1100.set_ISM(ism_setting);
    cc1100.set_channel(channel);
    cc1100.set_output_power_level(0);
    cc1100.set_myaddr(my_address);
    cc1100.spi_write_register(IOCFG2, 0x06);

    pinMode(GDO2, INPUT);
    pullUpDnControl(GDO2, PUD_DOWN);

    if (wiringPiISR(GDO2, INT_EDGE_RISING, &radio_interrupt) < 0) {
        std::fprintf(stderr, "Could not attach GDO2 interrupt.\\n");
        return EXIT_FAILURE;
    }

    cc1100.receive();

    std::fprintf(
        stderr,
        "[READY] receiver=%u sender_filter=%u channel=%u\\n",
        static_cast<unsigned>(my_address),
        static_cast<unsigned>(expected_sender),
        static_cast<unsigned>(channel)
    );

    while (true) {
        if (!packet_ready) {
            delay(2);
            continue;
        }

        if (expected_sender == 0 || sender_address == expected_sender) {
            const uint8_t* payload = find_payload();

            if (payload != nullptr) {
                emit_packet(payload);
            } else {
                std::fprintf(
                    stderr,
                    "[ERROR] Unexpected packet layout. length=%u first=%u\\n",
                    static_cast<unsigned>(packet_length),
                    static_cast<unsigned>(rx_fifo[0])
                );
            }
        } else {
            std::fprintf(
                stderr,
                "[IGNORED] sender=%u expected=%u\\n",
                static_cast<unsigned>(sender_address),
                static_cast<unsigned>(expected_sender)
            );
        }

        packet_ready = false;
        cc1100.receive();
    }
}
