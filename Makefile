ARDUINO_DIR  = /usr/share/arduino

BOARD_TAG    = mega2560
ARDUINO_PORT = /dev/ttyACM0

TARGET       = PinServer

ARDUINO_LIBS = Ethernet Ethernet/utility SPI

include /usr/share/arduino/Arduino.mk
