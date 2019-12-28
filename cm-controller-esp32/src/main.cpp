#include <Arduino.h>
#include <WiFi.h>
#include "IRremoteESP8266.h"
#include "DumpIR.h"
#include "IRsend.h"

// This file IS NOT CHECKED INTO GIT
// and is in the .gitignore
// It just consists of
/*
const char* ssid     = "your network name";
const char* password = "your network password";
*/
#include "LocalWifiCredentials.h"

// Web server code liberally taken from: https://randomnerdtutorials.com/esp32-web-server-arduino-ide/

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

/*--------- functions ------------------------- */
void loop_handleWebserver();


/*------------ constants ----------------------------- */
const auto LED_PIN = 4;


/*-------------- globals -------------------------------*/

IRsend gIRsend(LED_PIN);

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;



void setup() {
  // put your setup code here, to run once:
#if DUMP_IR
  DumpIR_setup();
#else
  gIRsend.begin();

  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

#endif

}

void loop() {
  // put your main code here, to run repeatedly:
#if DUMP_IR
  DumpIR_loop();
#else
/*
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, 0x09));
  delay(2000);
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, 0x08));
  delay(2000);
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, 0x07));
  delay(10000);

  */

  loop_handleWebserver();


#endif
}
enum class WebAction {
  no_action,
  move_000,
  move_090,
  move_180, 
  move_270,
  sync
};

void loop_handleAction(WebAction action);

void loop_handleWebserver()
{
  WiFiClient client = server.available();   // Listen for incoming clients
  WebAction action = WebAction::no_action;

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /000/") >= 0) {
              action = WebAction::move_000;
            } else if (header.indexOf("GET /090/") >= 0) {
              action = WebAction::move_090;
            } else if (header.indexOf("GET /180/") >= 0) {
              action = WebAction::move_180;
            } else if (header.indexOf("GET /270/") >= 0) {
              action = WebAction::move_270;
            } else if (header.indexOf("GET /SYNC/") >= 0) {
              action = WebAction::sync;
            }

            Serial.print("Action: ");
            Serial.println((int)action);

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            // make it responsive
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            // disable fav icon
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>CM Web Server</h1>");
            
            // Fill out a bunch of 'buttons'
            // these aren't buttons in the form sense, just hrefs
            // where our handler modifies state
            // our encoding is just provide the
            client.println("<p><a href=\"/000/\"><button class=\"button\">000</button></a></p>");
            client.println("<p><a href=\"/090/\"><button class=\"button\">090</button></a></p>");
            client.println("<p><a href=\"/180/\"><button class=\"button\">180</button></a></p>");
            client.println("<p><a href=\"/270/\"><button class=\"button\">270</button></a></p>");
            client.println("<p><a href=\"/SYNC/\"><button class=\"button\">SYNC</button></a></p>");

            /*
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            // If the output26State is off, it displays the ON button       
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            */
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
  loop_handleAction(action);
}

void sendTriplet(uint8_t cmd1, uint8_t  cmd2, uint8_t  cmd3) {
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, cmd1));
  delay(1000);
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, cmd2));
  delay(1000);
  gIRsend.sendNEC(gIRsend.encodeNEC(0xAC, cmd3));
}

void loop_handleAction(WebAction action)
{
  switch(action) {
    case WebAction::move_000: {
        sendTriplet(0,0,0);
    } break;
    case WebAction::move_090: {
        sendTriplet(0,9,0);
    } break;
    case WebAction::move_180: {
        sendTriplet(1,8,0);
    } break;
    case WebAction::move_270: {
        sendTriplet(2,7,0);
    } break;
    case WebAction::sync: {
        sendTriplet(0,0,0x11);
    } break;

    case WebAction::no_action: {
      // be quiet little compiler
    } break;






  }

}

