/**************************************************************************/
/*!
@file     BlueSMiRF.h
@author   Thomas Ricci

This is a library for the Sparkfun BlueSMiRF bluetooth breakout board
https://www.sparkfun.com/products/12577
This chip uses Serial to communicate, 2 pins (RX and TX) are required to interface
*/
/**************************************************************************/

#ifndef BlueSMiRF_included
#define BlueSMiRF_included

#if ARDUINO >= 100
#include "Arduino.h"
#include "SoftwareSerial.h"
#else
#include "WProgram.h"
#include "NewSoftSerial.h"
#endif

//#define BLUESMiRF_DEBUG

#define DEFAULT_BOUD 115200
#define MAX_DISCOVERABLE 3 //9 MAX for RN-41 Bluetooth module

#define BLUESMiRF_DEFAULT_TIMEOUT_MS 500

enum BlueSMiRF_Status{ CONFIG, NORMAL };

class BlueSMiRF
{
private:
	uint16_t _baud;
	BlueSMiRF_Status _status;
	unsigned long _lastInquiry;
	boolean _inquiring;
	char _replybuffer[32];
 #if ARDUINO >= 100
	 SoftwareSerial * _swSerial;
 #else
	 NewSoftSerial * _swSerial;
 #endif
	void startConfigMode();
	void endConfigMode();
	void flushInput();
	void sendCmd(const char * cmd, boolean newLineEnding = true);
	uint8_t readline(uint16_t timeout = BLUESMiRF_DEFAULT_TIMEOUT_MS, boolean multiline = false);
public:
 #if ARDUINO >= 100
	 BlueSMiRF(SoftwareSerial * serial);
 #else
	 BlueSMiRF(NewSoftSerial  * serial);
 #endif
 	char discovered[MAX_DISCOVERABLE][12];
	void begin(uint16_t baud = 115200);
	int8_t discovery(uint8_t scanSeconds = 10);
	boolean detect(char * id);
	void linkQuality();
	void disableDiscoverability();
	void enableDiscoverability();
	void help();
	void reboot();
	void setName(char * name);
	void settings();
	void extendedSettings();
	void serialConfigMode();
};

#endif
