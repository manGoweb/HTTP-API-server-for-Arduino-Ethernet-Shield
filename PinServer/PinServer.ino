/*
  Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 The server accepts two HTTP RPC calls:

 / - returns JSON object with the current pin values
 /set?pin2=1;pin9=0 - sets pin 2 to HIGH, pin 9 to LOW
 /set?pin2=1;pin9=0;pin53=2 - sets pin 2 to HIGH, pin 9 to LOW
                              and pin 53 to blinking mode
 /set?pin3=100 - switches pin 3 to opposite level for 100ms
                 (requests are not processed during that time)

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Set of output pins connected - as defined in the pins[] array

 To have the sketch advertise itself using Bonjour, clone
	git://github.com/neophob/EthernetBonjour.git
 in your ~/sketchbook/libraries/ directory. Alternatively, simply
 remove all EthernetBonjour references in this file and all the rest
 should still work.

 loosely based on Web Server example by David A. Mellis and Tom Ioge
 created 5 Jan 2013
 by Petr Baudis
 Copyright (c) 2012 Fuerte International. All rights reserved.

 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetBonjour.h>

// A set of output pins to be controlled by the web server
// We list all pins except those occupied by the Ethernet shield
// and 0, 1 which are used for Serial communication for now
// For Arduino Mega and similar:
int pins[] = { 2, 3, 4, 5, 6, 7, 8, 9,
		14, 15, 16, 17, 18, 19, 20, 21,
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
		44, 45, 46, 47, 48, 49, 50, 51, 52, 53 };
// For Arduino Uno and similar:
// int pins[] = { 2, 3, 4, 5, 6, 7, 8, 9 };
#define pins_n (sizeof(pins) / sizeof(pins[0]))

// Status of pins; by default, all pins are set to 0
int pin_states[pins_n] = { 0 };

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

// Timeout during HTTP communication; this default is extremely aggressive
unsigned long timeout = 15000; /* [ms] */

// Blink interval
unsigned long blinklength = 1000; /* [ms] */
// Current blink state
bool blinkstate = false;

void setup() {
  // Set mode of all pins[] to output
  for (int i = 0; i < pins_n; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], pin_states[i]);
  }

  // Open serial communications
  Serial.begin(9600);

  // start the Ethernet connection
  while (!Ethernet.begin(mac)) {
    Serial.println("Ethernet setup error");
    delay(1000);
  }

  // start the HTTP server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // register bonjour service
  EthernetBonjour.begin("fuerte-arduino");
  EthernetBonjour.addServiceRecord("Arduino Bonjour Pin Webserver._http",
                                   80, MDNSServiceTCP);
}

// Read HTTP request and send the appropriate reply
void handleClient(EthernetClient &client) {
  // the WAIT() macro waits for next byte to arrive, handling unexpected
  // disconnects and timeouts
  unsigned long t_start = millis();
#define WAIT() do { \
  while (client.connected() && !client.available()) { \
    /* Wait on, just check for timeout */ \
    if (millis() - t_start > timeout) { \
      Serial.println("timeout"); \
      return; \
    } \
  } \
  if (!client.connected()) { \
    Serial.println("connection interrupted"); \
    /* Oops, connection interrupted. */ \
    return; \
  } \
} while (0)

  // first, read the first line of the request, which should
  // be in format GET <path> HTTP/1.<anything>

  char reqline[256];
  int reqline_i = 0;
  do {
    WAIT();
    reqline[reqline_i] = client.read();
  } while (reqline[reqline_i++] != '\n');
  // newline received

  // now, read the rest of data, up to the first blank line,
  // and just ignore it. :-)
  boolean currentLineIsBlank = true;
  while (1) {
    WAIT();

    char c = client.read();

    if (c == '\n' && currentLineIsBlank) {
      // if you've gotten to the end of the line (received a newline
      // character) and the line is blank, the http request has ended,
      // so you can send a reply
      break;
    } else if (c == '\n') {
      // you're starting a new line
      currentLineIsBlank = true;
    } else if (c != '\r') {
      // you've gotten a character on the current line
      currentLineIsBlank = false;
    }
  }

  // now, figure out what the request was all about

  if (strncmp(reqline, "GET ", 4)) {
    Serial.print("501 Unknown method in: ");
    Serial.println(reqline);
    client.println("HTTP/1.1 501 Unknown or unsupported method");
    client.println("Connnection: close");
    client.println();
    return;
  }

  char *path = &reqline[4];
  while (isspace(path[0])) path++;
  if (path[0] != '/') {
bad_request:
    Serial.print("400 Bad request in: ");
    Serial.println(reqline);
    client.println("HTTP/1.1 400 Bad request");
    client.println("Connnection: close");
    client.println();
    return;
  }

  char *afterpath = strchr(path, ' ');
  if (!afterpath || strncmp(afterpath + 1, "HTTP/1.", 7)) {
    goto bad_request;
  }
  *afterpath = 0;

  // Now, path[] contains the request path.
  // Figure out what shall we do now:
  if (!strcmp(path, "/")) {
    // List status of all pins
    Serial.println("Listing pin status");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connnection: close");
    client.println();
    client.print("{");
    // Print pin status
    for (int i = 0; i < pins_n; i++) {
      client.print("\"pin");
      client.print(pins[i], DEC);
      client.print("\":");
      client.print(pin_states[i], DEC);
      if (i < pins_n - 1)
	client.print(",");
    }
    client.println("}");

  } else if (!strncmp(path, "/set?", 5)) {
    // Change pin status for listed pins
    Serial.print("Setting pin status for: ");
    Serial.println(path);
    for (char *qstring = path + 4; qstring; qstring = strchr(qstring, ';')) {
      qstring++;
      Serial.print("qstring "); Serial.println(qstring);
      if (strncmp(qstring, "pin", 3))
	goto bad_request;
      char *qstringval;
      int pinnum = strtol(qstring + 3, &qstringval, 10);
      Serial.print("qstringval "); Serial.println(qstringval);
      if (qstringval[0] != '=')
	goto bad_request;
      qstringval++;
      if (!isdigit(qstringval[0]))
	goto bad_request;
      int new_state = atoi(qstringval);

      // Now, pinnum is pin number and new_state is new pin state
      // Check if pinnum is in set (index will be in pin_i)
      int pin_i;
      for (pin_i = 0; pin_i < pins_n; pin_i++)
	if (pins[pin_i] == pinnum) {
	  break;
	}
      if (pin_i == pins_n) {
	Serial.print("403 Not found in: ");
	Serial.println(path);
	client.println("HTTP/1.1 403 Pin not in output set");
	client.println("Connnection: close");
	client.println();
	return;
      }

      Serial.print(pinnum, DEC);
      Serial.print(" <- ");
      Serial.println(new_state, DEC);
      if (new_state < 2)
	digitalWrite(pinnum, new_state);
      if (new_state < 50) {
	pin_states[pin_i] = new_state;
      } else {
	int state = pin_states[pin_i];
	if (state > 1) state = 1;
	digitalWrite(pinnum, !state);
	delay(new_state);
	digitalWrite(pinnum, state);
      }
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connnection: close");
    client.println("");
    client.println("1");

  } else {
    Serial.print("404 Not found in: ");
    Serial.println(reqline);
    client.println("HTTP/1.1 404 Not found");
    client.println("Connnection: close");
    client.println();
  }
}

void loop() {
  // following routines must be called periodically
  Ethernet.maintain();
  EthernetBonjour.run();

  // perform blinking
  int blinkphase = (millis() / blinklength) % 2;
  if (blinkphase != blinkstate) {
    // we should change blink state
    blinkstate = blinkphase;
    Serial.print("changing blink state to ");
    Serial.println(blinkstate, DEC);
    for (int i = 0; i < pins_n; i++) {
      if (pin_states[i] == 2)
	digitalWrite(pins[i], blinkstate);
    }
  }

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");

    handleClient(client);

    // give the web browser time to receive the reply
    // XXX: this does not seem to be required; uncomment
    // in case of communication problems
    // delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
