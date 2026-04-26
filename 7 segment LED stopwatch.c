// Jeremy Wolfson
// 30JUN2025
// Code based on code found on realdigital.org site and produced in part by Microsoft Copilot
///
///
////////////////////
#include <stdint.h> // For specific integer types
#include "sleep.h"   // For delay functions
#include <stdio.h>  // For input/output functions

// Memory-mapped register definitions
#define  Seg_ctrl *((uint32_t *)0x43c10000)  // 7-segment control register
#define  Seg_data *((uint32_t*)0x43C10004)  // 7-segment digit data register
#define  Button_Data *((uint32_t*)0x41200000)  // Buttons are the lower 4 bits

// Function to display a number on the 7-segment display
void display_on_7_seg (uint16_t number) {
 
uint32_t  temp = 0; // Setting a new integer as a numerical sandbox buffer
  Seg_ctrl = 1; // Enabling 7-segment to be in binary mode, 0 for hex.
  number = 1221; // Setting the number to be displayed on the 4 7-segment's
// Enable display in binary mode
// You have a 16 bit number in . That will display on al 4 7 segments.
// Convert it to something that will be put out on the displays.
// Put the proper value in temp.

    temp |= 0x00808080; // Turn off ALL decimal points
    temp |= (number % 10) | ((number / 10 % 10) << 8) | ((number / 100 % 10) << 16) | ((number / 1000) << 24);
 // code snippet was originally produced by Copilot on 23JUNE2025
 // (number % 10 ) for the 0-9 LSB | ((number / 10 % 10) << 8) for the 10-99 2nd LSB
 // ((number / 100 % 10) << 16) for the 100 - 999 2nd MSB | ((number / 1000) << 24); for the 1000 - 9999 MSB
    Seg_data = temp;    // Writing to the 7 segment data register, 43c10004 from the sandbox buffer
   
}

int main(void)
 {
 
uint16_t  counter = 0; // Initiallizing a new counting buffer
int  stopwatchRunning = 0; // Flag to check if stopwatch is running
uint32_t  button_state; // Initiallizing a new sandbox buffer, for Button_Data
while (1) {
        if (stopwatchRunning) {
            counter++; // Incrementally increasing counter buffer for each clock cycle
            sleep(1); // Sleep for 1 second
        }

        button_state = Button_Data; // update sandbox buffer with live data for utilization

        if ( (button_state & 0x01) == 0x01) { // If the LSB, from the right, is equal to the value of 1, Button 0 starts the stopwatch
            stopwatchRunning = 1;
        }

        display_on_7_seg(counter); // The display is updated on a seemingly continuous basis
    }

// Display the counter value
return 0;
}
