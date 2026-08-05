#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
  
// --- BEGIN RF TRANSMITTER HEADERS --- //
#include "macros.h"
#include "pins.h"
#include "cc1101.h"

// --- END RF TRANSMITTER HEADERS --- //

#include "checkpoint.h"
#include "crop_model.h" //

// ==========================================
// GLOBALS & DEFINITIONS
// ==========================================

// --- Crop Classification --- //
const char* get_crop_name(int class_id) {
    switch (class_id) {
        case CROP_CLASS_APPLE:       return "Apple";
        case CROP_CLASS_BANANA:      return "Banana";
        case CROP_CLASS_BLACKGRAM:   return "Blackgram";
        case CROP_CLASS_CHICKPEA:    return "Chickpea";
        case CROP_CLASS_COCONUT:     return "Coconut";
        case CROP_CLASS_COFFEE:      return "Coffee";
        case CROP_CLASS_COTTON:      return "Cotton";
        case CROP_CLASS_GRAPES:      return "Grapes";
        case CROP_CLASS_JUTE:        return "Jute";
        case CROP_CLASS_KIDNEYBEANS: return "Kidneybeans";
        case CROP_CLASS_LENTIL:      return "Lentil";
        case CROP_CLASS_MAIZE:       return "Maize";
        case CROP_CLASS_MANGO:       return "Mango";
        case CROP_CLASS_MOTHBEANS:   return "Mothbeans";
        case CROP_CLASS_MUNGBEAN:    return "Mungbean";
        case CROP_CLASS_MUSKMELON:   return "Muskmelon";
        case CROP_CLASS_ORANGE:      return "Orange";
        case CROP_CLASS_PAPAYA:      return "Papaya";
        case CROP_CLASS_PIGEONPEAS:  return "Pigeonpeas";
        case CROP_CLASS_POMEGRANATE: return "Pomegranate";
        case CROP_CLASS_RICE:        return "Rice";
        case CROP_CLASS_WATERMELON:  return "Watermelon";
        default:                     return "Unknown Crop";
    }
}

// --- System & Utility Definitions --- //
#define MCU_FREQ 8000000L

// --- TSL2591 I2C Definitions --- //
#define TSL2591_ADDR            0x29
#define TSL2591_COMMAND         0xA0
#define REGISTER_ENABLE         0x00
#define REGISTER_CHAN0_L        0x14
#define REGISTER_ID             0x12
#define ENABLE_POWERON          0x01
#define ENABLE_AEN              0x02

// --- Modbus RTU (NPK & pH) Definitions --- //
#define SLAVE_ID_NPK    0x01
#define SLAVE_ID_PH     0x02
#define FUNC_READ       0x03
#define REG_NPK_START   0x001E
#define REG_NPK_COUNT   3        
#define REG_PH_HIGH     0x0006
#define REG_PH_COUNT    1
#define RX_BUFFER_SIZE  16

// --- DHT22 Definition --- //
#define DHT_PIN         BIT3      

// --- Sensor Global Variables --- //
const char hex_chars[] = "0123456789ABCDEF";
char buffer[80];

volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_index = 0;
uint16_t npk_values[3];
float current_ph = 0.0;
uint16_t current_raw_ph = 0; // Added for RF Payload

unsigned int current_light_full = 0; // Added for RF Payload
unsigned int current_light_ir = 0;   // Added for RF Payload

int current_moisture_percent = 0;
int16_t current_humidity = 0;
int16_t current_temperature = 0;

volatile uint8_t predicted_class = CROP_MODEL_INVALID_CLASS;
volatile uint8_t expected_class = 0u;
volatile uint8_t prediction_matches = 0u;
volatile uint8_t prediction_complete = 0u;

// --- BEGIN RF TRANSMITTER GLOBALS --- //
uint8_t my_address = 0x01;

// Renamed from rx_buffer to rf_rx_buffer to prevent Modbus array collision
uint8_t rf_rx_buffer[64]; 
uint8_t rf_rx_length = 0;

uint8_t rx_sender = 0;
int8_t rx_rssi = 0;
uint8_t rx_lqi = 0;

volatile bool packet_received = false;
volatile bool packet_sent = false;
// --- END RF TRANSMITTER GLOBALS --- //


// --- Function Prototypes --- //
void init_system(void);
void delay_ms(uint16_t ms);
void uart_print_string(char *str);
void format_hex16(char *dst, unsigned int val);

int i2c_write_byte(unsigned char reg, unsigned char value);
int i2c_read_word(unsigned char reg, unsigned int *result);
unsigned char i2c_read_id(void);
unsigned int read_ADC(void);
uint16_t Calculate_CRC16(uint8_t *buffer, uint8_t length);
void Modbus_Read_Register(uint8_t slave_id, uint16_t reg_addr, uint16_t reg_count);
uint16_t measure_high_pulse(void);
uint8_t read_dht22(int16_t *humidity, int16_t *temperature);
void read_sensors(void);


// --- BEGIN RF TRANSMITTER HELPERS --- //
void uart_putstring(const char *str) {
    while (*str) {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = *str++;
    }
}

void print_hex(uint8_t val) {
    char h_chars[] = "0123456789ABCDEF";
    while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = h_chars[(val >> 4) & 0x0F];
    while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = h_chars[val & 0x0F];
}

void print_num(int num) {
    if (num < 0) {
        while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = '-';
        num = -num;
    }
    char buf[6];
    int idx = 0;
    if (num == 0) {
        while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = '0';
        return;
    }
    while (num > 0 && idx < 5) {
        buf[idx++] = '0' + (num % 10);
        num /= 10;
    }
    while (idx > 0) {
        while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = buf[--idx];
    }
}
// --- END RF TRANSMITTER HELPERS --- //


// ==========================================
// MAIN APPLICATION ENTRY
// ==========================================
int main(void) {
    unsigned char tsl_id = 0; //
    crop_model_input_t input; //
    uint16_t i; //

    init_system(); //
    delay_ms(1500); //

    uart_print_string("--- Unified MSP430FR5994 AI Sensor Hub Initialization ---\r\n"); //

    // 1. Verify TSL2591 Light Sensor Online State //
    tsl_id = i2c_read_id(); //
    if (tsl_id != 0x50) { //
        sprintf(buffer, "TSL2591 Light: FAILED (Read: 0x  )\r\n"); //
        buffer[30] = hex_chars[(unsigned int)((tsl_id >> 4) & 0x0F)]; //
        buffer[31] = hex_chars[(unsigned int)(tsl_id & 0x0F)]; //
        uart_print_string(buffer); //
        while(1); //
    } else {
        uart_print_string("TSL2591 Light: ONLINE\r\n"); //
        i2c_write_byte(REGISTER_ENABLE, ENABLE_POWERON | ENABLE_AEN); //
    }

    // 2. Perform NPK Modbus Manual Diagnostic Loopback //
    P4OUT |= (BIT1 | BIT2); //
    UCA3TXBUF = 0xAA; //
    while (!(UCA3IFG & UCTXIFG)); //
    while (UCA3STATW & UCBUSY); //
    P4OUT &= ~(BIT1 | BIT2); //
    delay_ms(1); //

    uart_print_string("System Hardware Verification Complete.\r\n"); //
    uart_print_string("---------------------------------------------\r\n"); //

    // --- BEGIN RF TRANSMITTER INITIALIZATION --- //
    CC1101_MSP430 cc1101;

    uart_putstring("\r\n--- System Boot: Initializing CC1101 ---\r\n");

    if (!cc1101.begin(&my_address)) {
        uart_putstring("[ERROR] CC1101 Initialization Failed! Hanging system.\r\n");
        P1OUT |= BV(0);
        while (1);
    }

    cc1101.set_channel(10);
    uart_putstring("[SUCCESS] CC1101 Active.\r\n");

    uint8_t reg_addr   = cc1101.spi_read_register(ADDR);
    uint8_t reg_channr = cc1101.spi_read_register(CHANNR);
    uint8_t reg_freq2  = cc1101.spi_read_register(FREQ2);

    uart_putstring("\r\n=== CC1101 SYNC PARAMETERS ===\r\n");
    uart_putstring("address: "); print_num(reg_addr); uart_putstring("\r\n");
    uart_putstring("channel: "); print_num(reg_channr); uart_putstring("\r\n");

    uart_putstring("frequency: ");
    if (reg_freq2 == 0x0C) { uart_putstring("315"); } 
    else if (reg_freq2 == 0x21 || reg_freq2 == 0x10 || reg_freq2 == 0x02) { uart_putstring("434"); } 
    else if (reg_freq2 == 0x24) { uart_putstring("868"); } 
    else if (reg_freq2 == 0x25) { uart_putstring("915"); } 
    else { uart_putstring("434"); }
    uart_putstring("\r\n");

    uart_putstring("modulation: "); uart_putstring("1"); uart_putstring("\r\n");
    uart_putstring("==============================\r\n\r\n");

    P1OUT |= BV(6);
    __delay_cycles(100000);
    P1OUT &= ~BV(6);
    // --- END RF TRANSMITTER INITIALIZATION --- //

    __bis_SR_register(GIE);

    // 3. Initialize Static Crop Model Parameters //
    input.rainfall = 202.93553161621094f; //
    input.soil_type = CROP_SOIL_LOAMY; //
    input.sunlight_exposure = 8.67735481262207f; //
    input.wind_speed = 10.109875679016113f; //
    expected_class = 20u; //

    // Main Runtime Loop //
    while(1) {
        // Poll hardware sensors //
        read_sensors(); //

        // 4. Populate dynamic model inputs //
        input.nitrogen = (float)npk_values[0]; //
        input.P = (float)npk_values[1]; //
        input.K = (float)npk_values[2]; //
        input.ph = current_ph; //
        input.soil_moisture = (float)current_moisture_percent; //
        input.temperature = (float)current_temperature / 10.0f; //
        input.humidity = (float)current_humidity / 10.0f; //

        // 5. Execute Machine Learning Model //
        predicted_class = crop_model_predict(&input); //

        if (predicted_class == expected_class) { //
            prediction_matches = 1u; //
        } else {
            prediction_matches = 0u; //
        }
        prediction_complete = 1u; //

        sprintf(buffer, "AI Model      -> Predicted Crop: %s (Class %d)\r\n", get_crop_name((int)predicted_class), (int)predicted_class); //
        uart_print_string(buffer); //
        uart_print_string("---------------------------------------------\r\n"); //


        // --- BEGIN RF TRANSMITTER LOOP CODE --- //
        uart_putstring(">> Attempting Packet TX...\r\n");
        P1OUT |= BV(0);

        // Pack Sensor Payload (18 Bytes - Network Byte Order / Big Endian)
        uint8_t tx_payload[18];

        // 1. ADC Soil Moisture
        tx_payload[0] = ((uint16_t)current_moisture_percent >> 8) & 0xFF;
        tx_payload[1] = (uint16_t)current_moisture_percent & 0xFF;

        // 2. DHT22 Temperature
        tx_payload[2] = ((uint16_t)current_temperature >> 8) & 0xFF;
        tx_payload[3] = (uint16_t)current_temperature & 0xFF;

        // 3. Ambient Temperature (Duplicated Temperature)
        tx_payload[4] = ((uint16_t)current_temperature >> 8) & 0xFF;
        tx_payload[5] = (uint16_t)current_temperature & 0xFF;

        // 4. DHT22 Humidity
        tx_payload[6] = ((uint16_t)current_humidity >> 8) & 0xFF;
        tx_payload[7] = (uint16_t)current_humidity & 0xFF;

        // 5. Light Sensor Lux (Fixed variable name)
        tx_payload[8] = ((uint16_t)current_light_full >> 8) & 0xFF;
        tx_payload[9] = (uint16_t)current_light_full & 0xFF;

        // 6. Modbus pH (Raw val, requires /100.0f division on receive)
        tx_payload[10] = (current_raw_ph >> 8) & 0xFF;
        tx_payload[11] = current_raw_ph & 0xFF;

        // 7. Modbus NPK - Nitrogen
        tx_payload[12] = (npk_values[0] >> 8) & 0xFF;
        tx_payload[13] = npk_values[0] & 0xFF;

        // 8. Modbus NPK - Phosphorus
        tx_payload[14] = (npk_values[1] >> 8) & 0xFF;
        tx_payload[15] = npk_values[1] & 0xFF;

        // 9. Modbus NPK - Potassium
        tx_payload[16] = (npk_values[2] >> 8) & 0xFF;
        tx_payload[17] = npk_values[2] & 0xFF;

        bool tx_status = cc1101.send_packet(my_address, 0x00, tx_payload, sizeof(tx_payload), 3);

        if (tx_status) {
            packet_sent = true;
            uart_putstring("   [TX SUCCESS]\r\n");
            P1OUT &= ~BV(0);
            st(P1OUT |= BV(6); __delay_cycles(50000); P1OUT &= ~BV(6););
        } else {
            packet_sent = false;
            uart_putstring("   [TX FAILED]\r\n");
            P1OUT &= ~BV(0);
        }

        uart_putstring("<< Entering Listening Mode (RX Loop)...\r\n");
        cc1101.receive();

        bool packet_processed = false;

        for (i = 0; i < 5000; i++) {
            if (cc1101.packet_available()) {
                uart_putstring("   [NOTIFICATION] Radio FIFO indicates data available.\r\n");

                rf_rx_length = sizeof(rf_rx_buffer); 
                rx_sender = 0xEE;
                rx_rssi = -99;

                if (cc1101.get_payload(rf_rx_buffer, &rf_rx_length, &my_address, &rx_sender, &rx_rssi, &rx_lqi)) {

                    if (rx_sender == 0xEE || rf_rx_length == 0) {
                        uart_putstring("   [ERROR CATCH] get_payload returned true, but variables are unpopulated/invalid!\r\n");
                        break;
                    }

                    packet_received = true;
                    packet_processed = true;

                    uart_putstring("   [RX SUCCESS] From Node: 0x");
                    print_hex(rx_sender);
                    uart_putstring(" | Len: ");
                    print_num((int)rf_rx_length);
                    uart_putstring(" | RSSI: ");
                    print_num((int)rx_rssi);
                    uart_putstring(" dBm\r\n");

                    uart_putstring("   [RAW DATA]: ");
                    for(uint8_t b = 0; b < rf_rx_length && b < 16; b++) {
                        print_hex(rf_rx_buffer[b]);
                        while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = ' ';
                    }
                    uart_putstring("\r\n");

                    st(
                        P1OUT |= (BV(0) | BV(6));
                        __delay_cycles(100000);
                        P1OUT &= ~(BV(0) | BV(6));
                    );
                    break;
                } else {
                    uart_putstring("   [ERROR CATCH] packet_available() was true, but get_payload() failed!\r\n");
                    break;
                }
            }
            __delay_cycles(100);
        }

        if (!packet_processed) {
            uart_putstring("   [TIMEOUT/DIAGNOSTIC] Poll window expired without valid payload extraction.\r\n");
        }

        uart_print_string("---------------------------------------------\r\n");
        // --- END RF TRANSMITTER LOOP CODE --- //

        // Regulates DHT22 compliance cycle interval //
        delay_ms(2500); //
    }
}

// ==========================================
// UNIFIED SENSOR POLLING ENGINE
// ==========================================
void read_sensors(void) { //
    // --- 1. TSL2591 Light Sensor Execution --- //
    if (i2c_read_word(REGISTER_CHAN0_L, &current_light_full) && //
        i2c_read_word(REGISTER_CHAN0_L + 2, &current_light_ir)) { //
        char hexbuf[5]; //
        uart_print_string("TSL2591 Light -> Full Spectrum: 0x"); //
        format_hex16(hexbuf, current_light_full); //
        uart_print_string(hexbuf); //
        uart_print_string(" | Infrared: 0x"); //
        format_hex16(hexbuf, current_light_ir); //
        uart_print_string(hexbuf); //
        uart_print_string("\r\n"); //
    } else {
        uart_print_string("TSL2591 Light -> I2C Bus Timeout!\r\n"); //
    }

    // --- 2. Analog Soil Moisture Sensor Execution --- //
    const int ADC_DRY = 2740; //
    const int ADC_WET = 1700; //
    unsigned int analog_value = read_ADC(); //
    current_moisture_percent = ((long)(ADC_DRY - (int)analog_value) * 100) / (ADC_DRY - ADC_WET); //

    if (current_moisture_percent < 0)   current_moisture_percent = 0; //
    if (current_moisture_percent > 100) current_moisture_percent = 100; //

    sprintf(buffer, "Soil Moisture -> Raw ADC: %d | Calculated: %d%%\r\n", (int)analog_value, current_moisture_percent); //
    uart_print_string(buffer); //

    // --- 3. Modbus NPK Soil Sensor Execution --- //
    Modbus_Read_Register(SLAVE_ID_NPK, REG_NPK_START, REG_NPK_COUNT); //
    delay_ms(300); //

    {
        uint8_t npk_frame_len = 3 + (2 * REG_NPK_COUNT); //
        uint8_t npk_total_len = npk_frame_len + 2; //

        if (rx_index >= npk_total_len && rx_buffer[1] == FUNC_READ) { //
            uint16_t received_crc = rx_buffer[npk_frame_len] | (rx_buffer[npk_frame_len + 1] << 8); //
            if (received_crc == Calculate_CRC16((uint8_t*)rx_buffer, npk_frame_len)) { //
                npk_values[0] = (rx_buffer[3] << 8) | rx_buffer[4]; //
                npk_values[1] = (rx_buffer[5] << 8) | rx_buffer[6]; //
                npk_values[2] = (rx_buffer[7] << 8) | rx_buffer[8]; //
                sprintf(buffer, "NPK Sensor    -> N: %d mg/kg | P: %d mg/kg | K: %d mg/kg\r\n", //
                        (int)npk_values[0], (int)npk_values[1], (int)npk_values[2]); //
                uart_print_string(buffer); //
            } else {
                uart_print_string("NPK Sensor    -> Frame Error: CRC Mismatch\r\n"); //
            }
        } else {
            uart_print_string("NPK Sensor    -> Timeout / Exception Received\r\n"); //
        }
    }

    // --- 4. Modbus Soil pH Sensor Execution --- //
    Modbus_Read_Register(SLAVE_ID_PH, REG_PH_HIGH, REG_PH_COUNT); //
    delay_ms(300); //

    if (rx_index >= 7 && rx_buffer[1] == FUNC_READ) { //
        uint16_t received_crc = rx_buffer[5] | (rx_buffer[6] << 8); //
        if (received_crc == Calculate_CRC16((uint8_t*)rx_buffer, 5)) { //
            uint16_t raw_ph = (rx_buffer[3] << 8) | rx_buffer[4]; //
            current_raw_ph = raw_ph;  // Store Raw Value for RF Payload Transmission
            current_ph = (float)raw_ph / 100.0f; //
            int ph_whole = (int)current_ph; //
            int ph_frac  = (int)((current_ph - ph_whole) * 100); //
            if (ph_frac < 0) ph_frac = -ph_frac; //

            if (ph_frac < 10) { //
                sprintf(buffer, "pH Sensor     -> Soil pH Value: %d.0%d\r\n", ph_whole, ph_frac); //
            } else {
                sprintf(buffer, "pH Sensor     -> Soil pH Value: %d.%d\r\n", ph_whole, ph_frac); //
            }
            uart_print_string(buffer); //
        } else {
            uart_print_string("pH Sensor     -> Frame Error: CRC Mismatch\r\n"); //
        }
    } else {
        uart_print_string("pH Sensor     -> Timeout / Exception Received\r\n"); //
    }

    // --- 5. DHT22 Temperature & Humidity Sensor Execution --- //
    if (read_dht22(&current_humidity, &current_temperature)) { //
        int16_t temp_whole = current_temperature / 10; //
        int16_t temp_frac  = current_temperature % 10; //
        if (temp_frac < 0) temp_frac = -temp_frac; //

        int16_t humid_whole = current_humidity / 10; //
        int16_t humid_frac  = current_humidity % 10; //

        sprintf(buffer, "DHT22 Climate -> Temp: %d.%d C | Humidity: %d.%d%%\r\n", //
                (int)temp_whole, (int)temp_frac, (int)humid_whole, (int)humid_frac); //
        uart_print_string(buffer); //
    } else {
        uart_print_string("DHT22 Climate -> Sensor Checksum or Handshake Error...\r\n"); //
    }

    // --- 6. System Supply Voltage (from checkpoint module) ---
    sprintf(buffer, "Syst. Voltage -> %d mV\r\n", (int)checkpoint_get_last_voltage_mv());
    uart_print_string(buffer);


    P1OUT ^= BV(6);          // Toggle LED (P1.6) to signal a completed cycle
    __delay_cycles(50000);   // Short blink delay (optional)
    P1OUT ^= BV(6);          // Toggle back
}

// ==========================================
// SYSTEM INITIALIZATION ENGINE
// ==========================================
void init_system(void) { //
    WDTCTL = WDTPW | WDTHOLD; //

    // 1. Clock Configuration: 8MHz Base Setting //
    CSCTL0_H = CSKEY_H; //
    CSCTL1 = DCOFSEL_3 | DCORSEL; //
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK; //
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1; //
    CSCTL0_H = 0; //

    // 2. Physical Pin Function Assignments //
    P2SEL1 |= (BIT0 | BIT1); //
    P2SEL0 &= ~(BIT0 | BIT1); //
    P6SEL0 |= BIT0 | BIT1; //
    P6SEL1 &= ~(BIT0 | BIT1); //
    P4DIR |= BIT1 | BIT2; //
    P4OUT &= ~(BIT1 | BIT2); //
    P7SEL1 &= ~(BIT0 | BIT1); //
    P7SEL0 |= (BIT0 | BIT1); //
    P1SEL0 |= BIT2; //
    P1SEL1 |= BIT2; //
    P1DIR &= ~DHT_PIN; //
    P1REN &= ~DHT_PIN; //
    P1OUT &= ~DHT_PIN; //

    // --- BEGIN RF TRANSMITTER PIN CONFIG --- //
    st(
        P1DIR |= (BV(0) | BV(6));
        P1OUT &= ~(BV(0) | BV(6));
    );
    // --- END RF TRANSMITTER PIN CONFIG --- //

    PM5CTL0 &= ~LOCKLPM5; //

    // 3. Peripheral Module Parameter Configurations //
    UCA0CTLW0 = UCSWRST | UCSSEL__SMCLK; //
    UCA0BR0 = 52; //
    UCA0BR1 = 0x00; //
    UCA0MCTLW = 0x4900 | UCOS16 | 0x0001; //
    UCA0CTLW0 &= ~UCSWRST; //

    UCA3CTLW0 = UCSWRST | UCSSEL__SMCLK; //
    UCA3BR0 = 52; //
    UCA3BR1 = 0x00; //
    UCA3MCTLW = 0x4900 | UCOS16 | 0x0001; //
    UCA3CTLW0 &= ~UCSWRST; //
    UCA3IE |= UCRXIE; //

    UCB2CTLW0 |= UCSWRST; //
    UCB2CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK; //
    UCB2BRW = 80; //
    UCB2CTLW0 &= ~UCSWRST; //

    ADC12CTL0 = ADC12ON | ADC12SHT0_2; //
    ADC12CTL1 = ADC12SHP; //
    ADC12MCTL0 = ADC12INCH_2 | ADC12VRSEL_0; //

    TA0CTL = TASSEL__SMCLK | ID__8 | MC__STOP | TACLR; //

    // 4. Checkpoint Initialize
    checkpoint_init();
}

// ==========================================
// LOW LEVEL PROTOCOL DRIVERS & DRIVER LOGIC
// ==========================================
void uart_print_string(char *str) { //
    while (*str) { //
        while (!(UCA0IFG & UCTXIFG)); //
        UCA0TXBUF = *str++; //
    }
}

void format_hex16(char *dst, unsigned int val) { //
    dst[0] = hex_chars[(val >> 12) & 0x0F]; //
    dst[1] = hex_chars[(val >> 8) & 0x0F]; //
    dst[2] = hex_chars[(val >> 4) & 0x0F]; //
    dst[3] = hex_chars[val & 0x0F]; //
    dst[4] = '\0'; //
}

void delay_ms(uint16_t ms) { //
    while (ms--) { //
        __delay_cycles(8000); //
    }
}

// --- TSL2591 I2C Subroutines --- //
int i2c_write_byte(unsigned char reg, unsigned char value) { //
    unsigned int timeout = 5000; //
    while ((UCB2STATW & UCBBUSY) && --timeout); //
    if (timeout == 0) return 0; //

    UCB2I2CSA = TSL2591_ADDR; //
    UCB2CTLW0 |= UCTR | UCTXSTT; //
    timeout = 5000; //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //
    if (timeout == 0) return 0; //
    UCB2TXBUF = (reg | TSL2591_COMMAND); //
    timeout = 5000; //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //
    if (timeout == 0) return 0; //
    UCB2TXBUF = value; //
    timeout = 5000; //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //
    UCB2CTLW0 |= UCTXSTP; //
    return 1; //
}

int i2c_read_word(unsigned char reg, unsigned int *result) { //
    unsigned char low_byte = 0, high_byte = 0; //
    unsigned int timeout = 5000; //
    while ((UCB2STATW & UCBBUSY) && --timeout); //
    if (timeout == 0) return 0; //

    UCB2I2CSA = TSL2591_ADDR; //
    UCB2CTLW0 |= UCTR | UCTXSTT; //
    timeout = 5000; //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //
    if (timeout == 0) return 0; //
    UCB2TXBUF = (reg | TSL2591_COMMAND); //
    timeout = 5000; //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //

    UCB2CTLW0 &= ~UCTR; //
    UCB2CTLW0 |= UCTXSTT; //
    timeout = 5000; //
    while ((UCB2CTLW0 & UCTXSTT) && --timeout); //
    if (timeout == 0) return 0; //

    timeout = 5000; //
    while (!(UCB2IFG & UCRXIFG0) && --timeout); //
    if (timeout == 0) return 0; //
    low_byte = UCB2RXBUF; //
    UCB2CTLW0 |= UCTXSTP; //
    timeout = 5000; //
    while (!(UCB2IFG & UCRXIFG0) && --timeout); //
    if (timeout == 0) return 0; //
    high_byte = UCB2RXBUF; //

    *result = (high_byte << 8) | low_byte; //
    return 1; //
}

unsigned char i2c_read_id(void) { //
    unsigned char id = 0; //
    unsigned int timeout = 5000; //
    UCB2I2CSA = TSL2591_ADDR; //
    UCB2CTLW0 |= UCTR | UCTXSTT; //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //
    if (timeout == 0) return 0x00; //
    UCB2TXBUF = (REGISTER_ID | TSL2591_COMMAND); //
    while (!(UCB2IFG & UCTXIFG0) && --timeout); //
    if (timeout == 0) return 0x00; //

    UCB2CTLW0 &= ~UCTR; //
    UCB2CTLW0 |= UCTXSTT; //
    timeout = 5000; //
    while ((UCB2CTLW0 & UCTXSTT) && --timeout); //
    if (timeout == 0) return 0x00; //

    UCB2CTLW0 |= UCTXSTP; //
    while (!(UCB2IFG & UCRXIFG0) && --timeout); //
    if (timeout == 0) return 0x00; //
    id = UCB2RXBUF; //
    return id; //
}

// --- ADC Subroutines --- //
unsigned int read_ADC(void) { //
    ADC12CTL0 |= ADC12ENC | ADC12SC; //
    while (!(ADC12IFGR0 & ADC12IFG0)); //
    return ADC12MEM0; //
}

// --- Modbus RTU Subroutines --- //
uint16_t Calculate_CRC16(uint8_t *buffer, uint8_t length) { //
    uint16_t crc = 0xFFFF; //
    for (uint8_t i = 0; i < length; i++) { //
        crc ^= (uint16_t)buffer[i]; //
        for (uint8_t j = 0; j < 8; j++) { //
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001; //
            else crc >>= 1; //
        }
    }
    return crc; //
}

void Modbus_Read_Register(uint8_t slave_id, uint16_t reg_addr, uint16_t reg_count) { //
    uint8_t frame[8]; //
    frame[0] = slave_id; //
    frame[1] = FUNC_READ; //
    frame[2] = (reg_addr >> 8) & 0xFF; //
    frame[3] = reg_addr & 0xFF; //
    frame[4] = (reg_count >> 8) & 0xFF; //
    frame[5] = reg_count & 0xFF; //
    uint16_t crc = Calculate_CRC16(frame, 6); //
    frame[6] = crc & 0xFF; //
    frame[7] = (crc >> 8) & 0xFF; //

    rx_index = 0; //
    P4OUT |= (BIT1 | BIT2); //
    for (int i = 0; i < 8; i++) { //
        while (!(UCA3IFG & UCTXIFG)); //
        UCA3TXBUF = frame[i]; //
    }
    while (UCA3STATW & UCBUSY); //
    P4OUT &= ~(BIT1 | BIT2); //
}

// --- DHT22 1-Wire Subroutines --- //
uint16_t measure_high_pulse(void) { //
    uint16_t count = 0; //
    TA0R = 0; //
    TA0CTL |= MC__CONTINUOUS; //
    while(!(P1IN & DHT_PIN)) { //
        if (TA0R > 250) { TA0CTL &= ~MC_3; return 0; } //
    }
    TA0R = 0; //
    while(P1IN & DHT_PIN) { //
        if (TA0R > 250) { TA0CTL &= ~MC_3; return 0; } //
    }
    count = TA0R; //
    TA0CTL &= ~MC_3; //
    return count; //
}

uint8_t read_dht22(int16_t *humidity, int16_t *temperature) { //
    uint8_t data[5] = {0,0,0,0,0}; //
    uint8_t i, j; //
    uint16_t pulse_len; //

    P1DIR |= DHT_PIN; //
    P1OUT &= ~DHT_PIN; //
    delay_ms(20); //
    P1OUT |= DHT_PIN; //
    P1DIR &= ~DHT_PIN; //
    __delay_cycles(40 * 8); //

    TA0R = 0; //
    TA0CTL |= MC__CONTINUOUS; //
    while(P1IN & DHT_PIN) { if (TA0R > 1000) { TA0CTL &= ~MC_3; return 0; } } //
    while(!(P1IN & DHT_PIN)) { if (TA0R > 1000) { TA0CTL &= ~MC_3; return 0; } } //
    while(P1IN & DHT_PIN) { if (TA0R > 1000) { TA0CTL &= ~MC_3; return 0; } } //
    TA0CTL &= ~MC_3; //

    for (i = 0; i < 5; i++) { //
        for (j = 0; j < 8; j++) { //
            pulse_len = measure_high_pulse(); //
            if (pulse_len == 0) return 0; //
            data[i] <<= 1; //
            if (pulse_len > 40) { //
                data[i] |= 1; //
            }
        }
    }
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) == data[4]) { //
        *humidity = (data[0] << 8) | data[1]; //
        *temperature = ((data[2] & 0x7F) << 8) | data[3]; //
        if (data[2] & 0x80) *temperature = -(*temperature); //
        return 1; //
    }
    return 0; //
}

// ==========================================
// INTERRUPT SERVICE ROUTINES
// ==========================================
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__) //
#pragma vector=USCI_A3_VECTOR //
__interrupt void USCI_A3_ISR(void) //
#elif defined(__GNUC__) //
void __attribute__ ((interrupt(USCI_A3_VECTOR))) USCI_A3_ISR (void) //
#endif
{
    switch(__even_in_range(UCA3IV, USCI_UART_UCTXCPTIFG)) { //
        case USCI_UART_UCRXIFG: //
            if (rx_index < RX_BUFFER_SIZE) { //
                rx_buffer[rx_index++] = UCA3RXBUF; //
            }
            break; //
        default: break; //
    }
}
