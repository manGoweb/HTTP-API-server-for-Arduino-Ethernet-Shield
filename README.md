HTTP-API-server-for-Arduino-Ethernet-Shield
===========================================

Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 The server accepts two HTTP RPC calls:

 / - returns JSON object with the current pin values
 /set?pin2=1;pin9=0 - sets pin 2 to HIGH, pin 9 to LOW
 /set?pin2=1;pin9=0;pin53=2 - sets pin 2 to HIGH, pin 9 to LOW
                              and pin 53 to blinking mode

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Set of output pins connected - as defined in the pins[] array


 To have the sketch advertise itself using Bonjour, clone
	git://github.com/neophob/EthernetBonjour.git
 in your ~/sketchbook/libraries/ directory. Alternatively, simply
 remove all EthernetBonjour references in this file and all the rest
 should still work.

 loosely based on Web Server example by David A. Mellis and Tom Ioge
 created 5 Jan 2013 by Petr Baudis

 Copyright (c) 2012 Fuerte International. All rights reserved.

http://www.fuerteint.com/