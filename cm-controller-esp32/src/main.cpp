#include <Arduino.h>
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

*/

const auto LED_PIN = 2;

void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
}