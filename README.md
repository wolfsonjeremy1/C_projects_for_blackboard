HARDWARE - 
Xilinx Zynq-based Blackboard
  SoC Architecture - ARM Cortex-A9 Processing System with FPGA
  Peripherals - GPIO, UART, SPI, I²C, ADC, PWM, timers, and interrupt controllers
  Memory - 512 MB DDR3 for memory-mapped I/O
  Data I/O - FIFO buffering

// C CODE

LED / Switch Register Control Direct memory
  // Mapped I/O used to link slide switch inputs to LED outputs on a Zynq platform. Continuous register reads and writes enable real-time hardware state updates via pointer-based access

7-Segment Stopwatch Memory
  // Mapped registers used to control a 7-segment display and hardware timer. Button inputs mapped to start/stop/reset states with interval timing logic

UART Console with switch operator & Prime Calculator 
  // UART communication implemented using FIFO register access for TX/RX over a serial interface based on switch enabled register status as a controller. User input parsed and processed to return computed results through the console

PMOD GPIO Traffic Controller 
  // PMOD GPIO used for digital signaling between master/slave states on a Zynq platform with alternating boards for loop timing. Bitwise control logic enforces synchronized traffic light state transitions

SPI / I2C Sensor Interface reading
  // SPI and I²C protocols used for register-level communication with external sensors. Raw data converted and streamed via UART for monitoring

ADC input conversion with Servo Control 
  // ADC input from a potentiometer mapped to servo control through register-based output. Real-time values displayed on a 7-segment interface

