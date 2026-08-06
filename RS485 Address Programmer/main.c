#include <msp430.h>
#include <stdint.h>

/* =====================================================================
 * CONFIGURATION BLOCK
 * ===================================================================== */
const uint8_t CURRENT_ADDRESS   = 0x01;    // Previous or Factory default address
const uint8_t NEW_ADDRESS       = 0x02;    // Desired new Modbus address
const uint16_t ADDRESS_REGISTER = 0x0100; // Equipment address register (0x0100)

/* =====================================================================
 * PINOUTS & HARDWARE DEFINITIONS
 * MAX485 DE -> P4.2
 * MAX485 RE -> P4.1
 * MAX485 DI -> P6.0 (UART TX for Sensor via MAX485)
 * MAX485 RO -> P6.1 (UART RX for Sensor via MAX485)
 *
 * LaunchPad Backchannel UART (eZ-FET to PC USB):
 * TX -> P2.0
 * RX -> P2.1
 * ===================================================================== */
#define RS485_CTRL_DIR P4DIR
#define RS485_CTRL_OUT P4OUT
#define RS485_DE       BIT2
#define RS485_RE       BIT1

/* =====================================================================
 * DIRECT UART HELPER FUNCTIONS (Bypasses printf limitations)
 * ===================================================================== */
void uart_putstring(const char *str) {
    while (*str) {
        while (!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = *str++;
    }
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

void print_hex(uint16_t val) {
    char h_chars[] = "0123456789ABCDEF";
    while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = h_chars[(val >> 12) & 0x0F];
    while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = h_chars[(val >> 8) & 0x0F];
    while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = h_chars[(val >> 4) & 0x0F];
    while (!(UCA0IFG & UCTXIFG)); UCA0TXBUF = h_chars[val & 0x0F];
}

/* =====================================================================
 * HELPER FUNCTIONS
 * ===================================================================== */

// Calculate Modbus RTU CRC-16
uint16_t ModbusCRC16(const uint8_t *buf, uint8_t len) {
    uint16_t crc = 0xFFFF;
    uint8_t pos;
    for (pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// Transmit the raw Modbus frame over UCA3 to the MAX485 board
void sendModbusFrame(const uint8_t *frame, uint8_t len) {
    RS485_CTRL_OUT |= (RS485_DE | RS485_RE); // Enable Driver (TX)
    __delay_cycles(1000);

    for (uint8_t i = 0; i < len; i++) {
        while(!(UCA3IFG & UCTXIFG));
        UCA3TXBUF = frame[i];
    }

    while(UCA3STATW & UCBUSY); // Wait for shift register to clear
    RS485_CTRL_OUT &= ~(RS485_DE | RS485_RE); // Return to Receiver mode
}

/* =====================================================================
 * MAIN PROGRAM
 * ===================================================================== */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    // --- Clock Configuration: 8MHz Base Setting ---
    CSCTL0_H = CSKEY_H;
    CSCTL1 = DCOFSEL_3 | DCORSEL;
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
    CSCTL0_H = 0;

    // --- Disable GPIO Power-On Default High-Impedance Mode ---
    PM5CTL0 &= ~LOCKLPM5;

    // --- GPIO Setup for MAX485 Control ---
    RS485_CTRL_DIR |= (RS485_DE | RS485_RE);
    RS485_CTRL_OUT &= ~(RS485_DE | RS485_RE);

    // --- UART Setup 1: UCA0 for PC Terminal (Pins P2.0 / P2.1) ---
    P2SEL0 &= ~(BIT0 | BIT1);
    P2SEL1 |=  (BIT0 | BIT1); // Configure P2.0/P2.1 for UART peripheral

    UCA0CTLW0 = UCSWRST | UCSSEL__SMCLK;
    UCA0BR0 = 52;             // 9600 baud @ 8MHz SMCLK
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x4900 | UCOS16 | 0x0001;
    UCA0CTLW0 &= ~UCSWRST;

    // --- UART Setup 2: UCA3 for MAX485 Sensor (Pins P6.0 / P6.1) ---
    P6SEL0 |= BIT0 | BIT1;
    P6SEL1 &= ~(BIT0 | BIT1);

    UCA3CTLW0 = UCSWRST | UCSSEL__SMCLK;
    UCA3BR0 = 52;             // 9600 baud @ 8MHz SMCLK
    UCA3BR1 = 0x00;
    UCA3MCTLW = 0x4900 | UCOS16 | 0x0001;
    UCA3CTLW0 &= ~UCSWRST;

    // Startup delay for stability
    __delay_cycles(100000);

    // --- Display Startup Information ---
    uart_putstring("\r\n--- RS485 Sensor Re-Addressing Tool ---\r\n");

    uart_putstring("Target Register: 0x");
    print_hex(ADDRESS_REGISTER);
    uart_putstring("\r\n");

    uart_putstring("Previous Address: 0x");
    print_hex(CURRENT_ADDRESS);
    uart_putstring("\r\n");

    uart_putstring("New Address:      0x");
    print_hex(NEW_ADDRESS);
    uart_putstring("\r\n");

    // --- Assemble Modbus Frame ---
    uint8_t frame[8];
    frame[0] = CURRENT_ADDRESS;
    frame[1] = 0x06; // Function 0x06: Write Single Register

    frame[2] = (uint8_t)(ADDRESS_REGISTER >> 8);
    frame[3] = (uint8_t)(ADDRESS_REGISTER & 0xFF);

    frame[4] = (uint8_t)(NEW_ADDRESS >> 8);
    frame[5] = (uint8_t)(NEW_ADDRESS & 0xFF);

    uint16_t crc = ModbusCRC16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);
    frame[7] = (uint8_t)(crc >> 8);

    // --- Execute ---
    uart_putstring("Sending Modbus Write Command to Sensor...\r\n");
    sendModbusFrame(frame, 8);
    uart_putstring("Address update command sent successfully!\r\n");

    // Trap CPU in low power mode once complete
    __bis_SR_register(LPM0_bits);

    return 0;
}
