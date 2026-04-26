//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EE 3420: Lab 4 – Traffic Simulator 
// Author: Jeremy Wolfson
// Contributions: Chazon Mingarine, CHATGPT (assumed through Mingarine, otherwise null this contribution), Carlos De Castro, Jonathan Martinez, Mark W. Welker
// Last edit: 7/11/25
//
// Note: All references to "PMOD" are always assumed to be PMOD C.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#define LED_Data	    *((uint32_t *)0x41210000) // The LED's we will be using is LED 12, one of 3 RGB light systems on the Blackboard.
#define Switch_Data *((uint32_t *)0x41220000) // the same address from lab 1, 2, and 3.
// Define the addresses for the GPIO inputs directly
#define Bank0_Input *((uint32_t *)0xE000A060) // bank0 inputs
#define Bank1_Input *((uint32_t *)0xE000A064) // bank1 inputs
#define Bank2_Input *((uint32_t *)0xE000A068) // bank2 inputs
#define Bank3_Input *((uint32_t *)0xE000A06C) // bank3 inputs
// The GPIO BANK output data
#define Bank0_Output *((uint32_t *)0xE000A040) // bank0 outputs
#define Bank1_Output *((uint32_t *)0xE000A044) // bank1 outputs
#define Bank2_Output *((uint32_t *)0xE000A048) // bank2 outputs
#define Bank3_Output *((uint32_t *)0xE000A04C) // bank3 outputs
// The GPIO BANK Direction data
#define Bank0_Dir *((uint32_t *)0xE000A204) // bank0 direction
#define Bank1_Dir *((uint32_t *)0xE000A244) // bank1 direction
#define Bank2_Dir *((uint32_t *)0xE000A284) // bank2 direction
#define Bank3_Dir *((uint32_t *)0xE000A2C4) // bank3 direction
// The GPIO BANK Enable data
#define Bank0_Enable *((uint32_t *)0xE000A208) // bank0 enabler
#define Bank1_Enable *((uint32_t *)0xE000A248) // bank1 enabler
#define Bank2_Enable *((uint32_t *)0xE000A288) // bank2 enabler
#define Bank3_Enable *((uint32_t *)0xE000A2C8) // bank3 enabler
#define red_light 0x20000
#define yellow_light 0x60000
#define green_light 0x40000
#define red_packet 0x8000
#define yellow_packet 0x20000
#define green_packet 0X10000

void sleep_2_seconds() { // No-op function instead of sleep function, looking for best efficiency if the counter isn't needed elsewhere.
    volatile uint32_t i;
    for (i = 0; i < 10000000; i++) {
        __asm__("nop");
    }
}

void sleep_1_second() { // *2 For a 1 second sleep alternative solution (no op) just to make int main look good/clean, despite there being literally more characters to parse now.
    volatile uint32_t i;
    for (i = 0; i < 10000000; i++) {
        __asm__("nop");
    }
}

int main(void) { // "initialization" of variable values that are relied on by underlying HDL code...
    Bank0_Dir = 0x70000;
    Bank0_Enable = 0x70000;
    Bank2_Dir = 0x78000;
    Bank2_Enable = 0x78000;

    while (1) {

        if (Switch_Data & 0x1) { // SW0 state is on, this board is set as master.

            Bank2_Output = red_packet; // Set a default status of red light.
            Bank0_Output = red_light; // Default light Output, just light a light intersection defaulting to flashing red when there is no control over the system over a triggering, "else" type manor.
            sleep_2_seconds();

            Bank0_Output = green_light; // The ideal light state for the ideal driver - over PMOD pin 2 (tx [MOSI]), slave is green.
            sleep_2_seconds();

            Bank0_Output = yellow_light; // Floor it or hold it - over PMOD pin 3 (tx [MOSI]), slave is yellow.
            sleep_2_seconds();

            Bank0_Output = red_light; // If there's no-one at midnight around, creep through the intersection, watching out for Grandma Jane escaping the nursing home.
                                      // Over PMOD pin 3 (tx [MOSI]), slave is red, master is still red. Always watch out for Grandma.
            sleep_2_seconds();

            Bank2_Output = green_packet; // "Beam us up Mr. Scott." - MOSI * 2 so the master board can go green - over PMOD pin 2 (tx [MOSI]) and PMOD pin 7 (rx [MISO]), master is green.
            sleep_2_seconds();

            Bank2_Output = yellow_packet; // Why did the traffic light break up with the car?
                                          // Because the car kept packet its bags every time it saw a yellow packet! Okay, this joke was provided by Google Gemini, only this part of the code
                                          // Over PMOD pin 3 (tx [MOSI]) and PMOD pin 8 (rx [MISO]), master is yellow.
            sleep_1_second();

            Bank2_Output = red_packet; // Everyone is back to square one, red state for everyone to end the While cycle, safely - over PMOD pin 1 (tx [MOSI]) and PMOD pin 9 (rx [MISO]), master is red.
        }

        else { // If switch 0 is not triggered, the default state of the board is to act as slave, waiting for MOSI messages.

            if (Bank2_Input & 0x80000) { // If this MOSI packet matches this description, the output is triggered for the MISO, sending red.

                Bank0_Output = red_light;
            }
            if (Bank2_Input & 0x200000) { // If this MOSI packet matches this description, the is triggered for the MISO, sending yellow.

                Bank0_Output = yellow_light;
            }
            if (Bank2_Input & 0x100000) { // If this MOSI packet matches this description, the is triggered for the MISO, sending green.

                Bank0_Output = green_light;
            }
        }
    }
    return 1;
} // End of the function.
