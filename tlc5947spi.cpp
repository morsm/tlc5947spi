/******************************************************************************
tlc5947spi.cpp
Tool to send array of 12-bit values to TLC5947 PWM 24-ch driver via SPI
(c) Maarten ter Mors, 2019

Based on
spitest.cpp
Raspberry Pi SPI interface demo
Byron Jacquot @ SparkFun Electronics>
4/2/2014
https://github.com/sparkfun/Pi_Wedge

The board was connected as follows:
(Raspberry Pi)(Serial 7 Segment)
GND  -> GND
5V -> Vcc
CE0  -> SS (Shift Select)
SCK  -> SCK 
MOSI -> SDI
MISO -> SDO

To build this file, I use the command:
>  g++ spitest.cpp -lwiringPi

Then to run it, first the spi kernel module needs to be loaded.  This can be 
done using the GPIO utility.
> gpio load spi
> ./a.out

This test uses the single-segment mode of the 7 segment display.  It shifts a 
bit through the display characters, lighting a single character of each at a 
time.

Development environment specifics:
Tested on Raspberry Pi V2 hardware, running Raspbian.
Building with GCC 4.6.3 (Debian 4.6.3-14+rpi1)

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

#include <iostream>
#include <errno.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <cstring>


using namespace std;

// channel is the wiringPi name for the chip select (or chip enable) pin.
// Set this to 0 or 1, depending on how it's connected.
static const int CHANNEL = 0;

int main(int argc, char **argv)
{
   int fd, result;
   unsigned int values[24];         // 24 12-bit values
   unsigned char buffer[36];        // 288 bits = 24 * 12 => 36 bytes

   cout << "Initializing" << endl ;

   // Configure the interface.
   // CHANNEL insicates chip select,
   // 500000 indicates bus speed.
   fd = wiringPiSPISetup(CHANNEL, 500000);

   cout << "Init result: " << fd << endl;

   // clear buffers
   memset(buffer, 0, 36);
   memset(values, 0, 24 * sizeof(unsigned int));

   // Read values from command line
   for (int i=1; i<argc; i++)
   {
       if (i > 23) break;

       int signedVal = atoi(argv[i]);
       if (signedVal < 0) signedVal = 0;
       if (signedVal > 4095) signedVal = 4095;

       values[i-1] = (unsigned int) signedVal;

       cout << "Value " << i << " " << values[i-1] << endl;
   }

   // Convert to buffer. The order is: Value 23, bit 11, Value 23, bit 10, ..., value 0 bit 1, value 0 bit 0.
   // Big Endian 12-bit values, in other words.
   for (int i=0; i<24; i+=2)
   {
        unsigned int firstVal = values[i], secondVal = values[i+1];
        unsigned int bufidx = 35 - 3 * i / 2;

        buffer[bufidx]     = (unsigned char) (firstVal & 0xff);                                         // LSB of first value (last in buffer)
        buffer[bufidx - 1] = (unsigned char) ((firstVal & 0xf00) >> 8) | ((secondVal & 0xf) << 4);      // 4 MSBs of first value, first, followed by 4 LSBs of second value
        buffer[bufidx - 2] = (unsigned char) ((secondVal & 0xff0) >> 4);                                // MSB of second value
   }

   cout << "Writing buffer: " << endl;
   for (int i=0; i<36; i++) cout << i << ": " <<((int) buffer[i]) << endl;

   wiringPiSPIDataRW(CHANNEL, buffer, 36);
}
