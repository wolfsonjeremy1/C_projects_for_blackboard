#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "sleep.h"

// === Memory-Mapped Registers ===
#define SW_DATA          *((volatile uint32_t *)0x41220000)
#define Seg_ctrl         *((volatile uint32_t *)0x43C10000)
#define Seg_data         *((volatile uint32_t *)0x43C10004)
#define UART1_CR         *((volatile uint32_t *)0xE0001000)
#define UART1_MR         *((volatile uint32_t *)0xE0001004)
#define UART1_BAUDGEN    *((volatile uint32_t *)0xE0001018)
#define UART1_BAUDDIV    *((volatile uint32_t *)0xE0001034)
#define UART1_SR         *((volatile uint32_t *)0xE000102C)
#define UART1_DATA       *((volatile uint32_t *)0xE0001030)
#define BaudGen115200    0x7C
#define BaudDiv115200    6
#define IIC_CFG          *((volatile uint32_t *)0xE0005000)
#define IIC_DATA         *((volatile uint32_t *)0xE000500C)
#define IIC_ADDR         *((volatile uint32_t *)0xE0005004)
#define IIC_XFER_SIZE    *((volatile uint32_t *)0xE0005008)
#define IIC_ISR          *((volatile uint32_t *)0xE0005014)
#define SLCR_IIC_RST     *((volatile uint32_t *)0xF8000224)
#define SLCR_UNLOCK      *((volatile uint32_t *)0xF8000008)
#define SLCR_LOCK        *((volatile uint32_t *)0xF8000004)
#define IIC_Config       0x0C0F
#define IIC_STATUS_DONE  0x02
// SPI 0 configuration
#define SPI_BASE         0xE0006000 // Configuration Register, // SPI config register
#define SPI_DATA         *((volatile uint32_t *)(SPI_BASE + 0x68)) //
#define SPI_STATUS       *((volatile uint32_t *)(SPI_BASE + 0x64))
#define SPI_CONFIG       *((volatile uint32_t *)(SPI_BASE + 0x60))
#define SPI_SLAVE_SELECT *((volatile uint32_t *)(SPI_BASE + 0x70))

// === Sensor Addresses ===
#define LM75B_ADDRESS       0x48
#define TEMP_REGISTER       0x00
#define LSM9DS1_TEMP_OUT_L  0x15
#define LSM9DS1_TEMP_OUT_H  0x16
#define LSM9DS1_OUT_Z_L_G   0x2C
#define LSM9DS1_OUT_Z_H_G   0x2D

// === UART ===
void uart1_put_char(char c) {
    while (UART1_SR & 0x10);
    UART1_DATA = (uint32_t)c;
}

void uart1_putstr(const char *s) {
    while (*s) uart1_put_char(*s++);
}

void reset_uart1() {
    UART1_CR = 0x03;
    while (UART1_CR & 0x03);
}

void configure_uart1() {
    UART1_MR = 0x20;
    UART1_CR = 0x03;
}

void set_baudrate1() {
    UART1_BAUDGEN = BaudGen115200;
    UART1_BAUDDIV = BaudDiv115200;
}

void init_uart1() {
    reset_uart1();
    set_baudrate1();
    configure_uart1();
}

// === I²C ===
void reset_iic() {
    SLCR_UNLOCK = 0xDF0D;
    SLCR_IIC_RST = 0x3;
    SLCR_IIC_RST = 0;
    SLCR_LOCK = 0x767B;
}

void iic_init() {
    IIC_CFG = IIC_Config;
    usleep(1000);
}

void i2c_read_bytes(uint8_t addr, uint8_t reg, uint8_t* data, uint8_t length) {
    IIC_ADDR = (addr << 1);
    IIC_DATA = reg;
    IIC_XFER_SIZE = 1;
    IIC_CFG |= (1 << 8);
    uint32_t timeout = 100000;
    while ((IIC_ISR & IIC_STATUS_DONE) == 0 && --timeout);
    (void)IIC_DATA;

    IIC_ADDR = (addr << 1) | 1;
    IIC_XFER_SIZE = length;
    IIC_CFG |= (1 << 8);
    for (int i = 0; i < length; i++) {
        timeout = 100000;
        while ((IIC_ISR & IIC_STATUS_DONE) == 0 && --timeout);
        data[i] = IIC_DATA & 0xFF;
    }
}

float read_lm75b_temperature() {
    uint8_t data[2];
    i2c_read_bytes(LM75B_ADDRESS, TEMP_REGISTER, data, 2);
    uint8_t msb = data[0];
    uint8_t lsb = data[1] & 0x7F;
    int16_t raw = (msb << 1) | (lsb >> 7);
    if (raw & 0x0100) raw |= 0xFE00;
    return raw * 0.5;
}

// === SPI ===
void init_spi() {
    SPI_CONFIG = 0x00000141;  // ~1 MHz clock
    SPI_SLAVE_SELECT = 0x00000001;
    usleep(1000);
}

uint8_t spi_read_byte(uint8_t reg) {
    uint32_t command = 0x80000000 | (reg << 16);
    SPI_DATA = command;
    uint32_t timeout = 100000;
    while (!(SPI_STATUS & 0x00000001) && --timeout);
    return (SPI_DATA & 0xFF);
}

float read_temp_spi_lsm9ds1() {
    uint8_t low = spi_read_byte(LSM9DS1_TEMP_OUT_L);
    usleep(10);
    uint8_t high = spi_read_byte(LSM9DS1_TEMP_OUT_H);
    int16_t raw = (high << 8) | low;
    return raw / 16.0 + 25.0;
}

int16_t read_z_angular_rate() {
    uint8_t zl = spi_read_byte(LSM9DS1_OUT_Z_L_G);
    usleep(10);
    uint8_t zh = spi_read_byte(LSM9DS1_OUT_Z_H_G);
    return (int16_t)((zh << 8) | zl);
}

// === Display ===
void display_on_7_seg(uint8_t val) {
    Seg_ctrl = 1;
    Seg_data = 0x003F;
    int32_t My7SegNumber = 0x00808080;
    My7SegNumber |= (val % 10) | ((val / 10 % 10) << 8) | ((val / 100 % 10) << 16) | ((val / 1000) << 24);
    Seg_data = My7SegNumber;
}

void display_temperature(uint8_t temp) {
    if (temp > 255) temp = 255;
    display_on_7_seg(temp);
    char buffer[64];
    sprintf(buffer, "Display Temp: %u°C\r\n", temp);
    uart1_putstr(buffer);
}

// === Printing Helpers ===
void uart_print_float(const char* label, float val) {
    char buffer[64];
    sprintf(buffer, "%s%.2f\r\n", label, val);
    uart1_putstr(buffer);
}

void uart_print_int(const char* label, int val) {
    char buffer[64];
    sprintf(buffer, "%s%d\r\n", label, val);
    uart1_putstr(buffer);
}

// === Main Routine ===
int main() {
    bool prev_sw = false;

    init_uart1();
    reset_iic();
    iic_init();
    init_spi();

    uart1_putstr("\r\n-- Sensor Fusion Monitor --\r\n");

    int loop_ticks = 0;

    while (1) {
        bool curr_sw = (SW_DATA & 0x01);

        if (curr_sw != prev_sw) {
            prev_sw = curr_sw;
            if (curr_sw) {
                uart1_putstr("Switch 0 is pressed: Currently displaying the SPI Temp\r\n");
            } else {
                uart1_putstr("Switch 0 is released: Currently displaying IIC Temp\r\n");
            }
            usleep(100000);
        }

        if (curr_sw) {
            float spi_temp = read_temp_spi_lsm9ds1();
            uart_print_float("SPI Temp (LSM9DS1): ", spi_temp);
            display_temperature((uint8_t)spi_temp);
            display_on_7_seg(spi_temp);
            sleep(1);
        } else {
            float iic_temp = read_lm75b_temperature();
            uart_print_float("IIC Temp (LM75B): ", iic_temp);
            display_temperature((uint8_t)iic_temp);
            display_on_7_seg(iic_temp);
            sleep(1);
        }

        if (++loop_ticks >= 5) {
            loop_ticks = 0;
            int16_t z_rate = read_z_angular_rate();
            uart_print_int("Z-axis Angular Rate: ", z_rate);
            display_on_7_seg(z_rate);
            uart1_putstr("Sleeping indefinetly until Switch 0 is retoggled to revitalize this heartbeat hardware you have.\r\n");
            while(1);
        }

        usleep(100000);
    }
    return 0;
}




/*// This is the sample code to interface to the SPI, IIC  
//
/////
/////////////////////
#include <stdint.h> // so I can use the integers I want
#include <stdio.h> // so I can use sprintf
#include "sleep.h"
////////////
// SPI defines
/////////////
// spi registers to work with the spi IP block in zynq
//we want to talk to SPI0
// define the minimum set to use
#define SPI0_CFG *((uint32_t *) 0xE0006000)// SPI config register
#define SPI0_EN *((uint32_t *) 0xE0006014)// SPI Enable register
#define SPI0_SR *((uint32_t *) 0xE0006004)// SPI Enable register
//#define SPI0_DEL *((uint32_t *) 0xE0006018)// SPI intraframe delays register
#define SPI0_TXD *((uint32_t *) 0xE000601C)// SPI write data port register
#define SPI0_RXD *((uint32_t *) 0xE0006020)// SPI read data port register
//#define SPI0_DWELL *((uint32_t *) 0xE0006024)// SPI dwell before start register
//#define SPI0_TXWR *((uint32_t *) 0xE0006028)// SPI transmit FIFO not full level register
//#define SPI0_ID *((uint32_t *) 0xE00060FC)// SPI Module ID
/////////////////////////////////////
#define LSM9DS1_Who 0x0f
#define LSM9DS1_CTRL_Reg1 0x10
#define LSM9DS1_Temp_G_low 0x15
#define LSM9DS1_Temp_G_high 0x16
#define JUNK 0
/////////////////////////////////////////////
//
// UnLock SPI
//
//SLCR addresses for SPI reset
#define SLCR_LOCK *( (uint32_t *) 0xF8000004)
#define SLCR_UNLOCK *( (uint32_t *) 0xF8000008)
#define SLCR_SPI_RST *( (uint32_t *) 0xF800021C)
//SLCR lock and unlock keys
#define UNLOCK_KEY 0xDF0D
#define LOCK_KEY 0x767B
////////////////////////
// Useful defines
#define timedelay 200000
#define CFG_NoSS 0x0BC27
#define CFG_SS0 0x08C27
#define CFG_SS0_Start 0x18C27
////////////
// IIC defines
///////////
#define IIC_CFG *((uint32_t *) 0xE0005000)// IIC config register
#define IIC_STAT *((uint32_t *) 0xE0005004)// IIC Status config register
#define IIC_ADDR *((uint32_t *) 0xE0005008)// IIC Address register
#define IIC_DATA *((uint32_t *) 0xE000500C)// IIC Data register
#define IIC_TSIZE *((uint32_t *) 0xE0005014)// IIC Transfer Size register
#define IIC_ISR *((uint32_t *) 0xE0005010)// IIC Interupt Status register
#define IIC_Config 0x0C0F // 0x0C0E write 0x0C0F read
//SLCR Register addresses and lock/unlock key values for Resetting SPI by writing to Zynq's SLCR Block F8...004 (1 below)
#define SLCR_LOCK *( (uint32_t *) 0xF8000004)
#define SLCR_UNLOCK *( (uint32_t *) 0xF8000008)
#define SLCR_IIC_RST *( (uint32_t *) 0xF8000224)
#define UNLOCK_KEY 0xDF0D // SLCI_Unlock (F8... 08) must be set to  this value to start SPI TX/RX communication (2 lines above)
#define LOCK_KEY 0x767B // SLCR_Lock (F8...04) must be set to this value to end SPI TX/RX communication (4 lines above)
#define TimerDelay 200000
#define LM75B_Addr 0x48

void reset_iic(void)
{
    SLCR_UNLOCK = UNLOCK_KEY; //unlock SLCRs
    SLCR_IIC_RST = 0x3; //assert I2C reset
    SLCR_IIC_RST = 0; //deassert I2C reset
    SLCR_LOCK = LOCK_KEY; //relock SLCRs
}
void reset_SPI(void) // This is required to initiallize the SPI controller, write it out if needed for reference
{
    int i = 0; //i for delay
    SLCR_UNLOCK = UNLOCK_KEY; //unlock SLCRs
    SLCR_SPI_RST = 0xF; //assert SPI reset
    for (i = 0; i < 1000; i++); //make sure Reset occurs
    SLCR_SPI_RST = 0; //deassert
    SLCR_LOCK = LOCK_KEY; //relock SLCRs
}
void iic_init() {

    IIC_CFG = IIC_Config;

}
/////
void WRITE_SPI(uint8_t adr, uint8_t WRITE_BYTE)
{
    SPI0_CFG = 0x8027;//Slave Select 0
    dummy_read = SPI0_RXD;
}

uint8_t READ_SPI(uint8_t adr)
{
    SPI0_CFG = 0x8027;//Slave Select 0

    return return_read;
}


int main() {

    int8_t SPI_LO = 0;
    int8_t SPI_HI = 0;
    int16_t SPI_Temp = 0;
    int LowByte, HiByte, temp, temp2, DispBytes;
    int temp16, tempc, i;

    //
    //
    ///////////////////////////////////
    // SPI Init 
    //////////////////////////////////
    uint32_t Junk;
    uint32_t TempLo, TempHi;
    reset_SPI();
    // configure SPI
    SPI0_CFG = CFG_NoSS;
    SPI0_EN = 1;
    //
    WRITE_SPI(0x10, 0xA0);
    WRITE_SPI(0x20, 0xA0);
    WRITE_SPI(0x23, 0x10);
    ///////
    // IIC iitialization
    /////////////////
    reset_iic(); //need icc reset here to communicate with I2c
    iic_init();


    while (1) { // in a loop so I can watch it through debugger
        //////////
        //SPI
        //////////////
        SPI_LO = READ_SPI(0x15);//Low
        SPI_HI = READ_SPI(0x16);//High-

        ///////////////
        // IIC	
        /////////////////
        IIC_TSIZE = 2; //transfering 2 bytes
        IIC_ADDR = LM75B_Addr;
        while ((IIC_ISR & 1) != 1) {
            //			temp = IIC_ISR;
            //			temp2 = IIC_STAT;
            //			usleep(TimerDelay);
        }	//wait here till transfer complete
            //read first two bytes, will be the temp
        HiByte = IIC_DATA;
        LowByte = IIC_DATA;



    }
*/