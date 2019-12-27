#include <Arduino.h>
#include "IRremoteESP8266.h"
#include "DumpIR.h"
#include "IRsend.h"

/*
Simple controller for the Channel Master CM9182?

It's designed to interface with GPredict and support the
rotctld protocol: https://www.mankier.com/1/rotctld

The protocol consists of a single letter command,
followed by arguments (if any) and terminated with \n

Since we can't actually read the position of the device
(at least without hacking things together) the result of the
'p' command will simply be the last commanded position.

Haven't decided how to handle startup.

My implementation uses : https://www.adafruit.com/product/387
As the LED. This is probably way more than I need. The datasheet
shows down to 10mA. The device can handle up to 100mA, but that would
require dragging out a transistor, meh.

The ESP32 GPIO can source 20mA and sink 28mA
(page 34: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

The voltage drop at 20mA is 1.2V (typ), 1.5V max.
At 100mA it's 1.4V.

3 ways to hook up LED:
1) Anode to VIN (which should be the 5V USB) cathode to GPIO as SINK. Max 28mA
2) Anode to 3V3, cathode to GPIO as SINK. Max 28mA
3) Cathode to GND, Anode to GPIO as Source (at 3.3V). max 20mA

Using ohms law assuming 1.2V drop:
1) 190ohms for 20mA
2) 105ohms for 20mA
3) My resistors are +/10 percent, so leave us headroom: 116ohms for 18mA

I'm going to do 3V3 sink into D2. I'm using the DOIT Devkit v1 board and those
pins are near each other to make it easo to hook in resistor and LED.

--------------------------------------

Ack. Based on testing this is TOO WEAK. Need to wire up a mosfet or NPN
to pulse much brighter. I can't find any specs as to what the 3v3 voltage regulator
can support. Sigh.

I've got a passel of 2N2222A NPN's kicking around: https://learn.sparkfun.com/tutorials/transistors/applications-i-switches
https://www.onsemi.com/pub/Collateral/PN2222-D.PDF

I'm using  500ohm for a base resistor, (3.3v - 0.7 drop = 2.6V actual). 
That gives me 5.2mA, current gain between 35 and 50 (from a couple of data sheets) = 182-260mA.
I'm aiming for 100mA for the LED.

Due to the small size of my breadboard, I can't access Vin which would be preferred. 
That's driven from USB and would handle 100mA without doubt.
But I'll just be using 3V3 and hoping I don't melt the voltage regulator. This
means a 20ohm current limiting resistor for the LED.

---------------------------------------------

Based on IR dump, the remote has a super simple protocol.

It's NEC and the keys on the remote are mapped to the following commands

The address on my device is always 0xAC

Power : 0x1C
1 : 0x01
2 : 0x02
3 : 0x03
4 : 0x04
and so on
The up arrow is 0x10
0 : 0x00
The down arrow is 0x11

Most commands consist of 3 characters.

Absolute position is the three number code
000 - 360

There are some special codes:
97 DOWN (Disable auto-power off after 8 minutes)
97 UP (Enable auto-power off)

98 DOWN (Disable auto-sync every 50 moves)
98 UP (enable auto sync)

00 DOWN (initate sync, takes about 1 minute)

*/

#define DUMP_IR false

const auto LED_PIN = 4;

IRsend gIRsend(LED_PIN);

void setup() {
  // put your setup code here, to run once:
#if DUMP_IR
  DumpIR_setup();
#else
  gIRsend.begin();
#endif

}

void loop() {
  // put your main code here, to run repeatedly:
#if DUMP_IR
  DumpIR_loop();
#else

  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, 0x09));
  delay(2000);
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, 0x08));
  delay(2000);
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, 0x07));
  delay(10000);
#endif
}