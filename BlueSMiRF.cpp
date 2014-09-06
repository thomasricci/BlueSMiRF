/**************************************************************************/
/*!
@file     BlueSMiRF.cpp
@author   Thomas Ricci

This is a library for the Sparkfun BlueSMiRF bluetooth breakout board
https://www.sparkfun.com/products/12577
This chip uses Serial to communicate, 2 pins (RX and TX) are required to interface
*/
/**************************************************************************/

#include "BlueSMiRF.h"
#include <string.h>
#include "MemoryFree.h"

const char Exit_DebugMode = '!';
const char * BEGINCONFIG_CMD = "$$$";
const char * ENDCONFIG_CMD = "---";
const char * COMMA = ",";
const char * NL = "\n";
const char * CR = "\r";
const char * CRNL = "\r\n";
const char * INFO_CMD = "D";
const char * EXTINFO_CMD = "E";
const char * INQUIRY_CMD = "I,%d";
const char * INQUIRYN_CMD = "IN,%d";
const char * BOUD_CMD = "U,%d,N";
const char * SETNAME_CMD ="SN,%s";
const char * REBOOT_CMD ="R,1";
const char * HELP_CMD = "H";
const char * LINKQLT_CMD = "L";
const char * QUIET_CMD = "Q";
const char * WAKE_CMD = "W";

#if ARDUINO >= 100
BlueSMiRF::BlueSMiRF(SoftwareSerial * serial) {
#else
BlueSMiRF::BlueSMiRF(NewSoftSerial * serial) {
#endif
	_swSerial = serial;
	_status = NORMAL;
	_swSerial->begin(DEFAULT_BOUD);
	_inquiring = false;
}

void BlueSMiRF::flushInput(){
	while(_swSerial->available())
		_swSerial->read();
}

void BlueSMiRF::sendCmd(const char* cmd, boolean newLineEnding){
	if(newLineEnding)
		_swSerial->println(cmd);
	else
		_swSerial->print(cmd);
		
	delay(100);
	flushInput();
}

void BlueSMiRF::begin(uint16_t baud){
	_baud = baud;
	startConfigMode();
	char cmd[16];
	sprintf(cmd, BOUD_CMD, _baud);
  	_swSerial->println(cmd);
  	_swSerial->begin(_baud);
	endConfigMode();
	_status = NORMAL;	
}

void BlueSMiRF::settings(){
	if (_status == NORMAL)
		startConfigMode();
	
	sendCmd(INFO_CMD);
}

void BlueSMiRF::extendedSettings(){
	if (_status == NORMAL)
		startConfigMode();
	
	sendCmd(EXTINFO_CMD);
}

void BlueSMiRF::setName(char * name){
	startConfigMode();
	char cmd [strlen(name) + 3];
	sprintf(cmd, SETNAME_CMD, name);
	sendCmd(cmd);
	endConfigMode();
}

int8_t BlueSMiRF::discovery(uint8_t scanSeconds){

	unsigned long currentTime = millis();
	// Check if its already time for our process
	if(!_inquiring && ((currentTime - _lastInquiry) > scanSeconds * 1000)) {
		if(_status == NORMAL)
			startConfigMode();
		
		char cmd[8];
		sprintf(cmd, INQUIRYN_CMD, scanSeconds - 4);
		
		flushInput();
		
		// Start the inquiry scan
		_swSerial->println(cmd);
		_inquiring = true;
		_lastInquiry = currentTime;
		
		#ifdef BLUESMiRF_DEBUG
		Serial.println(F("DISCOVERY session started"));
		Serial.print(F("Free Memory: "));
		Serial.println(freeMemory());
		#endif
	}
		
	//delay(polling - 1000);
	else if(_inquiring && (currentTime - _lastInquiry > (scanSeconds - 4) * 1000)){
		
		uint8_t len;
		uint8_t n = 0;
		boolean nl = false;
		
		//Reads first line: Inquiry,T=7,COD=0
		readline(); 
		//Reads line containing discovered device: Inquiry,T=7,COD=0
		do{
			len = readline();
			
			if(len <= 0)
				continue;
			
			char * temp;
			char* mac = strtok_r(_replybuffer, COMMA, &temp);
			
			if(mac == NULL || n > MAX_DISCOVERABLE)
				continue;
			
			strcpy(discovered[n], mac);
			
			#ifdef BLUESMiRF_DEBUG
			Serial.print(F("Found: "));
			Serial.println(discovered[n]);
			Serial.print(F("Memory free: "));
			Serial.println(freeMemory());
			#endif
			
			n++;
		} while(len > 0);
		
		_inquiring = false;
		
		return n;
	}
	
	return -1;
}

boolean BlueSMiRF::detect(char* id){
	int8_t n  = discovery();

	for(int8_t i=0; i<n; i++)		
		if(strncmp(discovered[i], id, 12) == 0)
			return true;
			
	return false;
}

void BlueSMiRF::startConfigMode(){
	if(_status == CONFIG) 
		return;
	
	sendCmd(BEGINCONFIG_CMD, false);
	_status = CONFIG;
}

void BlueSMiRF::endConfigMode(){
	sendCmd(ENDCONFIG_CMD);
	_status = NORMAL;
}

void BlueSMiRF::reboot(){
	sendCmd(REBOOT_CMD);
	_status = NORMAL;
}

void BlueSMiRF::help(){
	sendCmd(HELP_CMD);
}

void BlueSMiRF::disableDiscoverability(){
	sendCmd(QUIET_CMD);
}

void BlueSMiRF::enableDiscoverability(){
	sendCmd(WAKE_CMD);
}

void BlueSMiRF::linkQuality(){
	sendCmd(LINKQLT_CMD);
}

uint8_t BlueSMiRF::readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;
  
  while (timeout--) {
    if (replyidx >= 65)
      break;

    while(_swSerial->available()) {
      char c =  _swSerial->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)  
          continue;
        
        if (!multiline) {
          timeout = 0;        
          break;
        }
      }
      _replybuffer[replyidx] = c;
      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);
      replyidx++;
    }
    
    if (timeout == 0) 
      break;
    
    delay(1);
  }
  _replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

void BlueSMiRF::serialConfigMode(){
	 Serial.println(F("## +++ BLUESMiRF CONFIGURATION MODE ##"));
	 startConfigMode();
	 char inSerial;
	 do{
		 if (_swSerial->available())
		 {
			 Serial.print((char)_swSerial->read());
		 }
		 if (Serial.available())
		 {
			 inSerial = (char)Serial.read();

			 if (inSerial == Exit_DebugMode){
				 endConfigMode();
				 continue;
			 }
			 _swSerial->print(inSerial);
		 }

	 } while (inSerial != Exit_DebugMode);
	 Serial.println(F("## --- BLUESMiRF CONFIGURATION MODE ##"));
}
