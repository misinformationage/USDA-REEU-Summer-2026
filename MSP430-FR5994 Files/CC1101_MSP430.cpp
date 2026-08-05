/**
 * CC1101 Driver library for MSP430
 * Fully updated for native compilation under C++ class requirements.
 */
#include "cc1101.h" // Includes the structural definitions generated previously
#include "macros.h"
#include "pins.h"
#include <stdint.h>
#include <msp430.h>

// Define base CPU speed for timing microsecond calibrations (e.g., 16MHz)
#ifndef MCU_MCLK_MHZ
#define MCU_MCLK_MHZ 16
#endif

// Global variable definition (replacing the public class field)
uint8_t cc1101_debug_level = 0;

//-------------------[global default settings 868 Mhz]-------------------
static const uint8_t CC1101_GFSK_1_2_kb[] = {
                    0x07,  // IOCFG2        GDO2 Output Pin Configuration
                    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
                    0x80,  // IOCFG0        GDO0 Output Pin Configuration
                    0x07,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
                    0x57,  // SYNC1         Sync Word, High Byte
                    0x43,  // SYNC0         Sync Word, Low Byte
                    0x3E,  // PKTLEN        Packet Length
                    0x0E,  // PKTCTRL1      Packet Automation Control
                    0x45,  // PKTCTRL0      Packet Automation Control
                    0xFF,  // ADDR          Device Address
                    0x00,  // CHANNR        Channel Number
                    0x08,  // FSCTRL1       Frequency Synthesizer Control
                    0x00,  // FSCTRL0       Frequency Synthesizer Control
                    0x21,  // FREQ2         Frequency Control Word, High Byte
                    0x65,  // FREQ1         Frequency Control Word, Middle Byte
                    0x6A,  // FREQ0         Frequency Control Word, Low Byte
                    0xF5,  // MDMCFG4       Modem Configuration
                    0x83,  // MDMCFG3       Modem Configuration
                    0x13,  // MDMCFG2       Modem Configuration
                    0xA0,  // MDMCFG1       Modem Configuration
                    0xF8,  // MDMCFG0       Modem Configuration
                    0x15,  // DEVIATN       Modem Deviation Setting
                    0x07,  // MCSM2         Main Radio Control State Machine Configuration
                    0x0C,  // MCSM1         Main Radio Control State Machine Configuration
                    0x18,  // MCSM0         Main Radio Control State Machine Configuration
                    0x16,  // FOCCFG        Frequency Offset Compensation Configuration
                    0x6C,  // BSCFG         Bit Synchronization Configuration
                    0x03,  // AGCCTRL2      AGC Control
                    0x40,  // AGCCTRL1      AGC Control
                    0x91,  // AGCCTRL0      AGC Control
                    0x02,  // WOREVT1       High Byte Event0 Timeout
                    0x26,  // WOREVT0       Low Byte Event0 Timeout
                    0x09,  // WORCTRL       Wake On Radio Control
                    0x56,  // FREND1        Front End RX Configuration
                    0x17,  // FREND0        Front End TX Configuration
                    0xA9,  // FSCAL3        Frequency Synthesizer Calibration
                    0x0A,  // FSCAL2        Frequency Synthesizer Calibration
                    0x00,  // FSCAL1        Frequency Synthesizer Calibration
                    0x11,  // FSCAL0        Frequency Synthesizer Calibration
                    0x41,  // RCCTRL1       RC Oscillator Configuration
                    0x00,  // RCCTRL0       RC Oscillator Configuration
                    0x59,  // FSTEST        Frequency Synthesizer Calibration Control,
                    0x7F,  // PTEST         Production Test
                    0x3F,  // AGCTEST       AGC Test
                    0x81,  // TEST2         Various Test Settings
                    0x3F,  // TEST1         Various Test Settings
                    0x0B   // TEST0         Various Test Settings
                };


static const uint8_t CC1101_GFSK_38_4_kb[] = {
                    0x07,  // IOCFG2        GDO2 Output Pin Configuration
                    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
                    0x80,  // IOCFG0        GDO0 Output Pin Configuration
                    0x07,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
                    0x57,  // SYNC1         Sync Word, High Byte
                    0x43,  // SYNC0         Sync Word, Low Byte
                    0x3E,  // PKTLEN        Packet Length
                    0x0E,  // PKTCTRL1      Packet Automation Control
                    0x45,  // PKTCTRL0      Packet Automation Control
                    0xFF,  // ADDR          Device Address
                    0x00,  // CHANNR        Channel Number
                    0x06,  // FSCTRL1       Frequency Synthesizer Control
                    0x00,  // FSCTRL0       Frequency Synthesizer Control
                    0x21,  // FREQ2         Frequency Control Word, High Byte
                    0x65,  // FREQ1         Frequency Control Word, Middle Byte
                    0x6A,  // FREQ0         Frequency Control Word, Low Byte
                    0xCA,  // MDMCFG4       Modem Configuration
                    0x83,  // MDMCFG3       Modem Configuration
                    0x13,  // MDMCFG2       Modem Configuration
                    0xA0,  // MDMCFG1       Modem Configuration
                    0xF8,  // MDMCFG0       Modem Configuration
                    0x34,  // DEVIATN       Modem Deviation Setting
                    0x07,  // MCSM2         Main Radio Control State Machine Configuration
                    0x0C,  // MCSM1         Main Radio Control State Machine Configuration
                    0x18,  // MCSM0         Main Radio Control State Machine Configuration
                    0x16,  // FOCCFG        Frequency Offset Compensation Configuration
                    0x6C,  // BSCFG         Bit Synchronization Configuration
                    0x43,  // AGCCTRL2      AGC Control
                    0x40,  // AGCCTRL1      AGC Control
                    0x91,  // AGCCTRL0      AGC Control
                    0x02,  // WOREVT1       High Byte Event0 Timeout
                    0x26,  // WOREVT0       Low Byte Event0 Timeout
                    0x09,  // WORCTRL       Wake On Radio Control
                    0x56,  // FREND1        Front End RX Configuration
                    0x17,  // FREND0        Front End TX Configuration
                    0xA9,  // FSCAL3        Frequency Synthesizer Calibration
                    0x0A,  // FSCAL2        Frequency Synthesizer Calibration
                    0x00,  // FSCAL1        Frequency Synthesizer Calibration
                    0x11,  // FSCAL0        Frequency Synthesizer Calibration
                    0x41,  // RCCTRL1       RC Oscillator Configuration
                    0x00,  // RCCTRL0       RC Oscillator Configuration
                    0x59,  // FSTEST        Frequency Synthesizer Calibration Control,
                    0x7F,  // PTEST         Production Test
                    0x3F,  // AGCTEST       AGC Test
                    0x81,  // TEST2         Various Test Settings
                    0x3F,  // TEST1         Various Test Settings
                    0x0B   // TEST0         Various Test Settings
                };

static const uint8_t CC1101_GFSK_100_kb[] = {
                    0x07,  // IOCFG2        GDO2 Output Pin Configuration
                    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
                    0x80,  // IOCFG0        GDO0 Output Pin Configuration
                    0x07,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
                    0x57,  // SYNC1         Sync Word, High Byte
                    0x43,  // SYNC0         Sync Word, Low Byte
                    0x3E,  // PKTLEN        Packet Length
                    0x0E,  // PKTCTRL1      Packet Automation Control
                    0x45,  // PKTCTRL0      Packet Automation Control
                    0xFF,  // ADDR          Device Address
                    0x00,  // CHANNR        Channel Number
                    0x08,  // FSCTRL1       Frequency Synthesizer Control
                    0x00,  // FSCTRL0       Frequency Synthesizer Control
                    0x21,  // FREQ2         Frequency Control Word, High Byte
                    0x65,  // FREQ1         Frequency Control Word, Middle Byte
                    0x6A,  // FREQ0         Frequency Control Word, Low Byte
                    0x5B,  // MDMCFG4       Modem Configuration
                    0xF8,  // MDMCFG3       Modem Configuration
                    0x13,  // MDMCFG2       Modem Configuration
                    0xA0,  // MDMCFG1       Modem Configuration
                    0xF8,  // MDMCFG0       Modem Configuration
                    0x47,  // DEVIATN       Modem Deviation Setting
                    0x07,  // MCSM2         Main Radio Control State Machine Configuration
                    0x0C,  // MCSM1         Main Radio Control State Machine Configuration
                    0x18,  // MCSM0         Main Radio Control State Machine Configuration
                    0x1D,  // FOCCFG        Frequency Offset Compensation Configuration
                    0x1C,  // BSCFG         Bit Synchronization Configuration
                    0xC7,  // AGCCTRL2      AGC Control
                    0x00,  // AGCCTRL1      AGC Control
                    0xB2,  // AGCCTRL0      AGC Control
                    0x02,  // WOREVT1       High Byte Event0 Timeout
                    0x26,  // WOREVT0       Low Byte Event0 Timeout
                    0x09,  // WORCTRL       Wake On Radio Radio Control
                    0xB6,  // FREND1        Front End RX Configuration
                    0x17,  // FREND0        Front End TX Configuration
                    0xEA,  // FSCAL3        Frequency Synthesizer Calibration
                    0x0A,  // FSCAL2        Frequency Synthesizer Calibration
                    0x00,  // FSCAL1        Frequency Synthesizer Calibration
                    0x11,  // FSCAL0        Frequency Synthesizer Calibration
                    0x41,  // RCCTRL1       RC Oscillator Configuration
                    0x00,  // RCCTRL0       RC Oscillator Configuration
                    0x59,  // FSTEST        Frequency Synthesizer Calibration Control,
                    0x7F,  // PTEST         Production Test
                    0x3F,  // AGCTEST       AGC Test
                    0x81,  // TEST2         Various Test Settings
                    0x3F,  // TEST1         Various Test Settings
                    0x0B   // TEST0         Various Test Settings
                };

static const uint8_t CC1101_MSK_250_kb[] = {
                    0x07,  // IOCFG2        GDO2 Output Pin Configuration
                    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
                    0x80,  // IOCFG0        GDO0 Output Pin Configuration
                    0x07,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
                    0x57,  // SYNC1         Sync Word, High Byte
                    0x43,  // SYNC0         Sync Word, Low Byte
                    0x3E,  // PKTLEN        Packet Length
                    0x0E,  // PKTCTRL1      Packet Automation Control
                    0x45,  // PKTCTRL0      Packet Automation Control
                    0xFF,  // ADDR          Device Address
                    0x00,  // CHANNR        Channel Number
                    0x0B,  // FSCTRL1       Frequency Synthesizer Control
                    0x00,  // FSCTRL0       Frequency Synthesizer Control
                    0x21,  // FREQ2         Frequency Control Word, High Byte
                    0x65,  // FREQ1         Frequency Control Word, Middle Byte
                    0x6A,  // FREQ0         Frequency Control Word, Low Byte
                    0x2D,  // MDMCFG4       Modem Configuration
                    0x3B,  // MDMCFG3       Modem Configuration
                    0x73,  // MDMCFG2       Modem Configuration
                    0xA0,  // MDMCFG1       Modem Configuration
                    0xF8,  // MDMCFG0       Modem Configuration
                    0x00,  // DEVIATN       Modem Deviation Setting
                    0x07,  // MCSM2         Main Radio Control State Machine Configuration
                    0x0C,  // MCSM1         Main Radio Control State Machine Configuration
                    0x18,  // MCSM0         Main Radio Control State Machine Configuration
                    0x1D,  // FOCCFG        Frequency Offset Compensation Configuration
                    0x1C,  // BSCFG         Bit Synchronization Configuration
                    0xC7,  // AGCCTRL2      AGC Control
                    0x00,  // AGCCTRL1      AGC Control
                    0xB2,  // AGCCTRL0      AGC Control
                    0x02,  // WOREVT1       High Byte Event0 Timeout
                    0x26,  // WOREVT0       Low Byte Event0 Timeout
                    0x09,  // WORCTRL       Wake On Radio Control
                    0xB6,  // FREND1        Front End RX Configuration
                    0x17,  // FREND0        Front End TX Configuration
                    0xEA,  // FSCAL3        Frequency Synthesizer Calibration
                    0x0A,  // FSCAL2        Frequency Synthesizer Calibration
                    0x00,  // FSCAL1        Frequency Synthesizer Calibration
                    0x11,  // FSCAL0        Frequency Synthesizer Calibration
                    0x41,  // RCCTRL1       RC Oscillator Configuration
                    0x00,  // RCCTRL0       RC Oscillator Configuration
                    0x59,  // FSTEST        Frequency Synthesizer Calibration Control,
                    0x7F,  // PTEST         Production Test
                    0x3F,  // AGCTEST       AGC Test
                    0x81,  // TEST2         Various Test Settings
                    0x3F,  // TEST1         Various Test Settings
                    0x0B   // TEST0         Various Test Settings
                };

static const uint8_t CC1101_MSK_500_kb[] = {
                    0x07,  // IOCFG2        GDO2 Output Pin Configuration
                    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
                    0x80,  // IOCFG0        GDO0 Output Pin Configuration
                    0x07,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
                    0x57,  // SYNC1         Sync Word, High Byte
                    0x43,  // SYNC0         Sync Word, Low Byte
                    0x3E,  // PKTLEN        Packet Length
                    0x0E,  // PKTCTRL1      Packet Automation Control
                    0x45,  // PKTCTRL0      Packet Automation Control
                    0xFF,  // ADDR          Device Address
                    0x00,  // CHANNR        Channel Number
                    0x0C,  // FSCTRL1       Frequency Synthesizer Control
                    0x00,  // FSCTRL0       Frequency Synthesizer Control
                    0x21,  // FREQ2         Frequency Control Word, High Byte
                    0x65,  // FREQ1         Frequency Control Word, Middle Byte
                    0x6A,  // FREQ0         Frequency Control Word, Low Byte
                    0x0E,  // MDMCFG4       Modem Configuration
                    0x3B,  // MDMCFG3       Modem Configuration
                    0x73,  // MDMCFG2       Modem Configuration
                    0xA0,  // MDMCFG1       Modem Configuration
                    0xF8,  // MDMCFG0       Modem Configuration
                    0x00,  // DEVIATN       Modem Deviation Setting
                    0x07,  // MCSM2         Main Radio Control State Machine Configuration
                    0x0C,  // MCSM1         Main Radio Control State Machine Configuration
                    0x18,  // MCSM0         Main Radio Control State Machine Configuration
                    0x1D,  // FOCCFG        Frequency Offset Compensation Configuration
                    0x1C,  // BSCFG         Bit Synchronization Configuration
                    0xC7,  // AGCCTRL2      AGC Control
                    0x40,  // AGCCTRL1      AGC Control
                    0xB2,  // AGCCTRL0      AGC Control
                    0x02,  // WOREVT1       High Byte Event0 Timeout
                    0x26,  // WOREVT0       Low Byte Event0 Timeout
                    0x09,  // WORCTRL       Wake On Radio Control
                    0xB6,  // FREND1        Front End RX Configuration
                    0x17,  // FREND0        Front End TX Configuration
                    0xEA,  // FSCAL3        Frequency Synthesizer Calibration
                    0x0A,  // FSCAL2        Frequency Synthesizer Calibration
                    0x00,  // FSCAL1        Frequency Synthesizer Calibration
                    0x19,  // FSCAL0        Frequency Synthesizer Calibration
                    0x41,  // RCCTRL1       RC Oscillator Configuration
                    0x00,  // RCCTRL0       RC Oscillator Configuration
                    0x59,  // FSTEST        Frequency Synthesizer Calibration Control,
                    0x7F,  // PTEST         Production Test
                    0x3F,  // AGCTEST       AGC Test
                    0x81,  // TEST2         Various Test Settings
                    0x3F,  // TEST1         Various Test Settings
                    0x0B   // TEST0         Various Test Settings
                };

static const uint8_t CC1101_OOK_4_8_kb[] = {
                    0x06,  // IOCFG2        GDO2 Output Pin Configuration
                    0x2E,  // IOCFG1        GDO1 Output Pin Configuration
                    0x06,  // IOCFG0        GDO0 Output Pin Configuration
                    0x47,  // FIFOTHR       RX FIFO and TX FIFO Thresholds
                    0x57,  // SYNC1         Sync Word, High Byte
                    0x43,  // SYNC0         Sync Word, Low Byte
                    0xFF,  // PKTLEN        Packet Length
                    0x04,  // PKTCTRL1      Packet Automation Control
                    0x05,  // PKTCTRL0      Packet Automation Control
                    0x00,  // ADDR          Device Address
                    0x00,  // CHANNR        Channel Number
                    0x06,  // FSCTRL1       Frequency Synthesizer Control
                    0x00,  // FSCTRL0       Frequency Synthesizer Control
                    0x21,  // FREQ2         Frequency Control Word, High Byte
                    0x65,  // FREQ1         Frequency Control Word, Middle Byte
                    0x6A,  // FREQ0         Frequency Control Word, Low Byte
                    0x87,  // MDMCFG4       Modem Configuration
                    0x83,  // MDMCFG3       Modem Configuration
                    0x3B,  // MDMCFG2       Modem Configuration
                    0x22,  // MDMCFG1       Modem Configuration
                    0xF8,  // MDMCFG0       Modem Configuration
                    0x15,  // DEVIATN       Modem Deviation Setting
                    0x07,  // MCSM2         Main Radio Control State Machine Configuration
                    0x30,  // MCSM1         Main Radio Control State Machine Configuration
                    0x18,  // MCSM0         Main Radio Control State Machine Configuration
                    0x14,  // FOCCFG        Frequency Offset Compensation Configuration
                    0x6C,  // BSCFG         Bit Synchronization Configuration
                    0x07,  // AGCCTRL2      AGC Control
                    0x00,  // AGCCTRL1      AGC Control
                    0x92,  // AGCCTRL0      AGC Control
                    0x87,  // WOREVT1       High Byte Event0 Timeout
                    0x6B,  // WOREVT0       Low Byte Event0 Timeout
                    0xFB,  // WORCTRL       Wake On Radio Control
                    0x56,  // FREND1        Front End RX Configuration
                    0x17,  // FREND0        Front End TX Configuration
                    0xE9,  // FSCAL3        Frequency Synthesizer Calibration
                    0x2A,  // FSCAL2        Frequency Synthesizer Calibration
                    0x00,  // FSCAL1        Frequency Synthesizer Calibration
                    0x1F,  // FSCAL0        Frequency Synthesizer Calibration
                    0x41,  // RCCTRL1       RC Oscillator Configuration
                    0x00,  // RCCTRL0       RC Oscillator Configuration
                    0x59,  // FSTEST        Frequency Synthesizer Calibration Control
                    0x7F,  // PTEST         Production Test
                    0x3F,  // AGCTEST       AGC Test
                    0x81,  // TEST2         Various Test Settings
                    0x35,  // TEST1         Various Test Settings
                    0x09   // TEST0         Various Test Settings
                };

// Patable index: -30  -20- -15  -10   0    5    7    10 dBm
static const uint8_t patable_power_315[] = {0x17,0x1D,0x26,0x69,0x51,0x86,0xCC,0xC3};
static const uint8_t patable_power_433[] = {0x6C,0x1C,0x06,0x3A,0x51,0x85,0xC8,0xC0};
static const uint8_t patable_power_868[] = {0x03,0x17,0x1D,0x26,0x50,0x86,0xCD,0xC0};
static const uint8_t patable_power_915[] = {0x0B,0x1B,0x6D,0x67,0x50,0x85,0xC9,0xC1};

//----------------------------------[END]---------------------------------------

//-------------------------[CC1101 reset function]------------------------------
void CC1101_MSP430::reset(void)                  // reset defined in CC1101 datasheet
{
    SPI_DRIVE_CSN_LOW();
    while (SPI_SO_IS_HIGH());             // wait for chip ready (XOSC stable) before proceeding
    SPI_DRIVE_CSN_HIGH();
    __delay_cycles(40 * MCU_MCLK_MHZ);    // native hardware microsecond delay

    SPI_DRIVE_CSN_LOW();
    while (SPI_SO_IS_HIGH());             // wait for chip ready again before issuing SRES
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = SRES;
    while (UCB1STATW & UCBUSY);
    uint8_t dummy = UCB1RXBUF;            // clear RX flag
    while (SPI_SO_IS_HIGH());             // wait for reset to complete
    SPI_DRIVE_CSN_HIGH();
    __delay_cycles(1000 * MCU_MCLK_MHZ);  // 1ms native delay
}
//-----------------------------[END]--------------------------------------------

//------------------------[set Power Down]--------------------------------------
void CC1101_MSP430::powerdown(void)
{
    sidle();
    spi_write_strobe(SPWD);                               // CC1101 Power Down
}
//-----------------------------[end]--------------------------------------------

//---------------------[CC1101 set debug level]---------------------------------
uint8_t CC1101_MSP430::set_debug_level(uint8_t set_debug)
{
    cc1101_debug_level = set_debug;        //set debug level of CC1101 outputs
    return cc1101_debug_level;
}
//-----------------------------[end]--------------------------------------------

//---------------------[CC1101 get debug level]---------------------------------
uint8_t CC1101_MSP430::get_debug_level(void)
{
    return cc1101_debug_level;
}
//-----------------------------[end]--------------------------------------------

//----------------------[CC1101 init functions]---------------------------------
uint8_t CC1101_MSP430::begin(volatile uint8_t *My_addr)
{
    uint8_t CC1101_freq_select, CC1101_mode_select, CC1101_channel_select;
    uint8_t partnum, version;

    // Pin setups handled here (maps to the internal setup configurations)
    P3DIR &= ~BV(1); // Ensure GDO0 is input or handled via macro definitions

    set_debug_level(1);            //set debug level of CC1101 outputs

    if(cc1101_debug_level > 0){
        uart_puti(0); // Custom fallback mechanism or string hook
    }

    spi_begin();                          //inits SPI Interface
    reset();                              //CC1101 init reset

    spi_write_strobe(SFTX);__delay_cycles(100 * MCU_MCLK_MHZ);//flush the TX_fifo content
    spi_write_strobe(SFRX);__delay_cycles(100 * MCU_MCLK_MHZ);//flush the RX_fifo content

    partnum = spi_read_register(PARTNUM); //reads CC1101 partnumber
    version = spi_read_register(VERSION); //reads CC1101 version number

    //checks if valid Chip ID is found. Usualy 0x03 or 0x14. if not -> abort
    if(version == 0x00 || version == 0xFF){
            return FALSE;
        }

    if(cc1101_debug_level > 0){
        uart_puthex_byte(partnum);
        uart_puthex_byte(version);
    }

    //default settings
    *My_addr = 0x00;
    CC1101_freq_select = CC1101_FREQ_434MHZ;     //433.92MHz
    CC1101_mode_select = CC1101_MODE_GFSK_1_2_KB; //gfsk 1.2kbps
    CC1101_channel_select = 0x01;

    //set modulation mode
    set_mode(CC1101_mode_select);

    //set ISM band
    set_ISM(CC1101_freq_select);

    //set channel
    set_channel(CC1101_channel_select);

    //set output power amplifier
    set_output_power_level(0);            //set PA to 0dBm as default

    //set my receiver address
    set_myaddr(*My_addr);                  //set addr

    receive();                            //set CC1101 in receive mode

    return TRUE;
}
//-------------------------------[end]------------------------------------------

//-----------------[finish's the CC1101 operation]------------------------------
void CC1101_MSP430::end(void)
{
    powerdown();                          //power down CC1101
    spi_end();                            //disable SPI Interface
}
//-------------------------------[end]------------------------------------------

//-----------------------[show all CC1101 registers]----------------------------
void CC1101_MSP430::show_register_settings(void)
{
    if(cc1101_debug_level > 0){
        uint8_t config_reg_verify[CFG_REGISTER], Patable_verify[CFG_REGISTER];
        uint8_t i;

        spi_read_burst(READ_BURST,config_reg_verify,CFG_REGISTER);  //reads all 47 config register
        spi_read_burst(PATABLE_BURST,Patable_verify,8);              //reads output power settings

        for(i = 0 ; i < CFG_REGISTER; i++)  //showes rx_buffer for debug
        {
            uart_puthex_byte(config_reg_verify[i]);
        }

        for(i = 0 ; i < 8; i++)         //showes rx_buffer for debug
        {
            uart_puthex_byte(Patable_verify[i]);
        }
    }
}
//-------------------------------[end]------------------------------------------

//----------------------------[idle mode]---------------------------------------
uint8_t CC1101_MSP430::sidle(void)
{
    uint8_t marcstate;

    spi_write_strobe(SIDLE);              //sets to idle first. must be in
    marcstate = 0xFF;                     //set unknown/dummy state value

    while(marcstate != 0x01)              //0x01 = sidle
    {
        marcstate = (spi_read_register(MARCSTATE) & 0x1F); //read out state of CC1101 to be sure in RX
    }
    __delay_cycles(100 * MCU_MCLK_MHZ);
    return TRUE;
}
//-------------------------------[end]------------------------------------------

//---------------------------[transmit mode]------------------------------------
uint8_t CC1101_MSP430::transmit(void)
{
    uint8_t marcstate;

    sidle();                               //sets to idle first.
    spi_write_strobe(STX);                 //sends the data over air

    marcstate = 0xFF;                     //set unknown/dummy state value

    while(marcstate != 0x01)              //0x01 = ILDE after sending data
    {
        marcstate = (spi_read_register(MARCSTATE) & 0x1F); //read out state of CC1101 to be sure in IDLE and TX is finished
    }
    __delay_cycles(100 * MCU_MCLK_MHZ);
    return TRUE;
}
//-------------------------------[end]------------------------------------------

//---------------------------[receive mode]-------------------------------------
uint8_t CC1101_MSP430::receive(void)
{
    uint8_t marcstate;

    sidle();                               //sets to idle first.
    spi_write_strobe(SRX);                 //writes receive strobe (receive mode)

    marcstate = 0xFF;                     //set unknown/dummy state value

    while(marcstate != 0x0D)              //0x0D = RX
    {
        marcstate = (spi_read_register(MARCSTATE) & 0x1F); //read out state of CC1101 to be sure in RX
    }
    __delay_cycles(100 * MCU_MCLK_MHZ);
    return TRUE;
}
//-------------------------------[end]------------------------------------------

//------------[enables WOR Mode  EVENT0 ~1890ms; rx_timeout ~235ms]--------------------
void CC1101_MSP430::wor_enable(void)
{
    sidle();

    spi_write_register(MCSM0, 0x18);    //FS Autocalibration
    spi_write_register(MCSM2, 0x01);    //MCSM2.RX_TIME = 1b

    // configure EVENT0 time
    spi_write_register(WOREVT1, 0xFF);  //High byte Event0 timeout
    spi_write_register(WOREVT0, 0x7F);  //Low byte Event0 timeout

    // configure EVENT1 time
    spi_write_register(WORCTRL, 0x78);  //WOR_RES=0b; tEVENT1=0111b=48d -> 48*(750/26MHz)= 1.385ms

    spi_write_strobe(SFRX);             //flush RX buffer
    spi_write_strobe(SWORRST);          //resets the WOR timer to the programmed Event 1
    spi_write_strobe(SWOR);             //put the radio in WOR mode when CSn is released

    __delay_cycles(100 * MCU_MCLK_MHZ);
}
//-------------------------------[end]------------------------------------------

//------------------------[disable WOR Mode]-------------------------------------
void CC1101_MSP430::wor_disable(void)
{
    sidle();                             //exit WOR Mode
    spi_write_register(MCSM2, 0x07);    //stay in RX. No RX timeout
}
//-------------------------------[end]------------------------------------------

//------------------------[resets WOR Timer]------------------------------------
void CC1101_MSP430::wor_reset(void)
{
    sidle();                             //go to IDLE
    spi_write_register(MCSM2, 0x01);    //MCSM2.RX_TIME = 1b
    spi_write_strobe(SFRX);             //flush RX buffer
    spi_write_strobe(SWORRST);          //resets the WOR timer to the programmed Event 1
    spi_write_strobe(SWOR);             //put the radio in WOR mode when CSn is released

    __delay_cycles(100 * MCU_MCLK_MHZ);
}
//-------------------------------[end]------------------------------------------

//-------------------------[tx_payload_burst]-----------------------------------
uint8_t CC1101_MSP430::tx_payload_burst(uint8_t my_addr, uint8_t rx_addr, uint8_t *txbuffer, uint8_t length)
{
    // Flush first - a stray leftover byte from a prior send sitting in the
    // TX FIFO would otherwise get prepended to this packet, corrupting the
    // frame the other end sees. SFTX is only valid in IDLE/TXFIFO_UNDERFLOW/
    // OVERFLOW per the datasheet - we're normally still in RX here (from the
    // previous receive() call), so go idle first or the flush is a no-op.
    sidle();
    spi_write_strobe(SFTX);
    spi_write_burst(TXFIFO_BURST, txbuffer, length);
    return transmit();
}

//------------------------------------------------------------------------------
// FIXED: Appended Missing Methods for CC1101_MSP430
//------------------------------------------------------------------------------

// 1. Hardware Interface Blocks (SPI and UART stubs or adaptations)
void CC1101_MSP430::spi_begin(void)
{
    // 1. Configure Chip Select (CSn) as an output and set it HIGH (idle state)
    SPI_CONFIG_CSN_PIN_AS_OUTPUT();
    SPI_DRIVE_CSN_HIGH();

    // 2. Fire the macro that turns on UCB1, configures clocks, and sets P5.0/P5.1/P5.2 to SPI mode
    SPI_INIT();

    // 3. Configure the GDO interrupt pins as inputs so they don't float
    CONFIG_GDO0_PIN_AS_INPUT();
    CONFIG_GDO2_PIN_AS_INPUT();
}

void CC1101_MSP430::spi_write_strobe(uint8_t strobe)
{
    SPI_DRIVE_CSN_LOW();
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = strobe;
    while (UCB1STATW & UCBUSY);
    uint8_t dummy = UCB1RXBUF; // Clear RX flag
    SPI_DRIVE_CSN_HIGH();
}

uint8_t CC1101_MSP430::spi_read_register(uint8_t reg)
{
    uint8_t val;
    SPI_DRIVE_CSN_LOW();
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = (reg | 0x80); // Read operation bitmask (rw=1)
    while (UCB1STATW & UCBUSY);
    val = UCB1RXBUF; // Dummy read

    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = 0x00; // Send dummy byte to clock out data
    while (UCB1STATW & UCBUSY);
    val = UCB1RXBUF;
    SPI_DRIVE_CSN_HIGH();
    return val;
}

uint8_t CC1101_MSP430::check_crc(uint8_t lqi)
{
    // Bit 7 of the second appended status byte is CRC_OK, set by the CC1101
    // hardware CRC checker (see get_payload()).
    return (lqi & 0x80) ? TRUE : FALSE;
}

void CC1101_MSP430::uart_puthex_byte(uint8_t byte)
{
    // Write your hexadecimal UART debugging transmission output here
}

void CC1101_MSP430::uart_puti(int val)
{
    // Write your integer UART debugging transmission output here
}

// 2. Configuration Helper Mappings
void CC1101_MSP430::set_mode(uint8_t mode)
{
    // NOTE: this used to be a no-op, which meant the CC1101 was left running
    // on its power-on-reset register defaults - no sync-word tuned for our
    // link, no address filtering (PKTCTRL1.ADR_CHK) and no CRC autoflush
    // (PKTCTRL1.CRC_AUTOFLUSH). That is what let it "find" and accept any
    // stray RF energy on the band as a packet. Loading one of the real
    // profiles below is what actually enables that filtering in hardware.
    const uint8_t *cfg;

    switch (mode) {
        case CC1101_MODE_GFSK_1_2_KB:  cfg = CC1101_GFSK_1_2_kb;  break;
        case CC1101_MODE_GFSK_38_4_KB: cfg = CC1101_GFSK_38_4_kb; break;
        case CC1101_MODE_GFSK_100_KB:  cfg = CC1101_GFSK_100_kb;  break;
        case CC1101_MODE_MSK_250_KB:   cfg = CC1101_MSK_250_kb;   break;
        case CC1101_MODE_MSK_500_KB:   cfg = CC1101_MSK_500_kb;   break;
        case CC1101_MODE_OOK_4_8_KB:    cfg = CC1101_OOK_4_8_kb;   break;
        default:                       cfg = CC1101_GFSK_1_2_kb;  break;
    }

    sidle();                                            // must be out of RX/TX before reconfiguring
    spi_write_burst(0x00, (uint8_t *)cfg, CFG_REGISTER); // load all 47 config registers starting at 0x00
}

void CC1101_MSP430::set_ISM(uint8_t band)
{
    // The register profiles above all embed the same ~868MHz FREQx bytes
    // (they were copied from the same SmartRF template); this is what
    // actually re-tunes the synthesizer to the requested ISM band. It must
    // run AFTER set_mode() since set_mode() overwrites FREQ2/FREQ1/FREQ0
    // with the profile's (wrong-band) defaults.
    uint8_t freq2, freq1, freq0;

    switch (band) {
        case CC1101_FREQ_315MHZ:
            freq2 = 0x0C; freq1 = 0x1D; freq0 = 0x89;
            break;
        case CC1101_FREQ_868MHZ:
            freq2 = 0x21; freq1 = 0x62; freq0 = 0x76;
            break;
        case CC1101_FREQ_915MHZ:
            freq2 = 0x23; freq1 = 0x31; freq0 = 0x3B;
            break;
        case CC1101_FREQ_434MHZ:
        default:
            freq2 = 0x10; freq1 = 0xB0; freq0 = 0x71;
            break;
    }

    spi_write_register(FREQ2, freq2);
    spi_write_register(FREQ1, freq1);
    spi_write_register(FREQ0, freq0);

    cc1101_ism_band = band; // remembered so set_output_power_level() can load the matching PATABLE
}

void CC1101_MSP430::set_channel(uint8_t channel)
{
    spi_write_register(CHANNR, channel);
}

void CC1101_MSP430::set_output_power_level(int8_t dBm)
{
    // NOTE: this used to be a no-op. Every register profile above sets
    // FREND0.PA_POWER = 7 (FREND0 = 0x17), which tells the radio to
    // transmit using PATABLE[7] as its constant "on" power for GFSK/MSK.
    // With this function doing nothing, PATABLE was never written and sat
    // at its power-on-reset value - effectively 0x00 (no real RF output)
    // for every index above 0. That let this MSP430 hear the Pi (RX
    // doesn't depend on local TX power) while the Pi never heard anything
    // back. Loading the real per-band ramp table below fixes that.
    //
    // The `dBm` argument isn't mapped to a specific PATABLE index here
    // since FREND0.PA_POWER is fixed at 7 (max) by the loaded profile -
    // this loads the whole ramp so index 7 (and any other index a future
    // FREND0 change might select) has a valid, calibrated value.
    const uint8_t *patable;

    switch (cc1101_ism_band) {
        case CC1101_FREQ_315MHZ: patable = patable_power_315; break;
        case CC1101_FREQ_868MHZ: patable = patable_power_868; break;
        case CC1101_FREQ_915MHZ: patable = patable_power_915; break;
        case CC1101_FREQ_434MHZ:
        default:                 patable = patable_power_433; break;
    }

    set_patable((uint8_t *)patable);
}

void CC1101_MSP430::set_patable(uint8_t *patable_arr)
{
    // Base PATABLE address is 0x3E; spi_write_burst() ORs in the burst-write
    // bit (0x40) itself, giving 0x7E == PATABLE_BURST per the datasheet.
    spi_write_burst(0x3E, patable_arr, 8);
}

void CC1101_MSP430::set_myaddr(uint8_t addr)
{
    spi_write_register(ADDR, addr);
}

// 3. Packet Handling and State Checking Engine
uint8_t CC1101_MSP430::packet_available(void)
{
    uint8_t rxbytes_reg = spi_read_register(RXBYTES);

    if (rxbytes_reg & 0x80) {
        // RX FIFO overflow - whatever is sitting in the FIFO is corrupt.
        // Flush it and go back to listening rather than letting it be
        // read out and reported as a "packet".
        spi_write_strobe(SFRX);
        receive();
        return FALSE;
    }

    uint8_t rxbytes = rxbytes_reg & 0x7F;

    // get_payload() needs at least the length byte + rx_addr + tx_addr
    // bytes + the 2 appended status bytes to extract anything meaningful.
    // Reporting "available" too early (the old `> 0` check) is what caused
    // every noise burst/partial reception to fire a NOTIFICATION + failed
    // get_payload() pair into the log.
    return (rxbytes > 3) ? TRUE : FALSE;
}

uint8_t CC1101_MSP430::send_packet(uint8_t my_addr, uint8_t rx_addr, uint8_t *txbuffer, uint8_t length, uint8_t tx_retries)
{
    uint8_t packet[64];

    // Frame format the reference driver on the other end (Raspberry Pi
    // RX/TX demo) expects:
    //   [pkt_len] [rx_addr] [tx_addr] [payload...]
    // pkt_len counts everything after itself: rx_addr + tx_addr + payload.
    // This previously only wrote [pkt_len][rx_addr][payload...] - the
    // tx_addr (sender) byte was never written and pkt_len was one byte
    // short, so every frame sent from here didn't match what the Pi's
    // parser expects and got silently discarded/misread on that end.
    packet[0] = length + 2;  // rx_addr + tx_addr + payload
    packet[1] = rx_addr;     // destination address (also what hw ADR_CHK matches against)
    packet[2] = my_addr;     // sender address, so the receiver knows who sent it

    // TI ULP 13.1 Hint: Optimization by counting down
    for(uint8_t i = length; i > 0; i--) {
        packet[2 + i] = txbuffer[i - 1];
    }

    return tx_payload_burst(my_addr, rx_addr, packet, length + 3);
}

uint8_t CC1101_MSP430::get_payload(uint8_t *rxbuffer, uint8_t *length, uint8_t *my_addr, uint8_t *rx_addr, int8_t *rssi, uint8_t *lqi)
{
    uint8_t rxbytes = spi_read_register(RXBYTES) & 0x7F;
    if (rxbytes > 3) {
        // FIXED: Using standard single byte RX FIFO register define
        uint8_t pkt_len = spi_read_register(RXFIFO_SINGLE_BYTE);

        // Byte 2 is the destination address the hardware ADR_CHK already
        // matched against our own address - not useful to the caller, so
        // it's read and discarded rather than reported as "who sent this".
        spi_read_register(RXFIFO_SINGLE_BYTE);

        // Byte 3 is tx_addr - the actual sender's address per the frame
        // format above. This is what callers (main.cpp's rx_sender) want.
        *rx_addr = spi_read_register(RXFIFO_SINGLE_BYTE);
        *length = pkt_len - 2; // exclude rx_addr + tx_addr from the payload length

        // TI ULP 13.1 Hint: Optimization by counting down
        for (unsigned int i = *length; i > 0; i--) {
            rxbuffer[i - 1] = spi_read_register(RXFIFO_SINGLE_BYTE);
        }

        *rssi = (int8_t)spi_read_register(RXFIFO_SINGLE_BYTE);

        // Second appended status byte: bit7 = CRC_OK, bits[6:0] = LQI estimate.
        uint8_t status_byte = spi_read_register(RXFIFO_SINGLE_BYTE);
        *lqi = status_byte & 0x7F;

        spi_write_strobe(SFRX); // Flush remaining bytes if needed
        receive();              // Re-enter listening state

        // PKTCTRL1.CRC_AUTOFLUSH (set by set_mode()) already drops bad-CRC
        // packets in hardware before they're readable here, but double check
        // in software too - belt and suspenders against noise being
        // reported as a valid payload.
        if (!check_crc(status_byte)) {
            return FALSE;
        }

        return TRUE;
    }
    return FALSE;
}

void CC1101_MSP430::spi_write_register(uint8_t reg, uint8_t value)
{
    SPI_DRIVE_CSN_LOW();

    // Wait for TX buffer to be ready, then send the register address byte
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = reg;
    while (UCB1STATW & UCBUSY);
    volatile uint8_t dummy1 = UCB1RXBUF; // Clear RX flag

    // Wait for TX buffer, then send the data value byte
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = value;
    while (UCB1STATW & UCBUSY);
    volatile uint8_t dummy2 = UCB1RXBUF; // Clear RX flag

    SPI_DRIVE_CSN_HIGH();
}

void CC1101_MSP430::spi_write_burst(uint8_t reg, uint8_t *buffer, uint8_t length)
{
    SPI_DRIVE_CSN_LOW();

    // Send register address byte with burst write bitmask enabled (0x40)
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = (reg | 0x40);
    while (UCB1STATW & UCBUSY);
    volatile uint8_t dummy = UCB1RXBUF; // Clear RX flag

    // Stream the data buffer backwards to satisfy TI ULP 13.1 (down-counting loop optimization)
    for (uint8_t i = length; i > 0; i--) {
        while (!(UCB1IFG & UCTXIFG));
        UCB1TXBUF = buffer[length - i];
        while (UCB1STATW & UCBUSY);
        dummy = UCB1RXBUF; // Clear RX flag
    }

    SPI_DRIVE_CSN_HIGH();
}
