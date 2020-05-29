//////////////////////////////////////////////////////////////////////////////////////
//																					//
//							FishinoSPIMaster.h										//
//						Library for ESP8266 WiFi module								//
//					Created by Massimo Del Fedele, 2015								//
//																					//
//  Copyright (c) 2015, 2016 and 2017 Massimo Del Fedele.  All rights reserved.		//
//																					//
//	Redistribution and use in source and binary forms, with or without				//
//	modification, are permitted provided that the following conditions are met:		//
//																					//
//	- Redistributions of source code must retain the above copyright notice,		//
//	  this list of conditions and the following disclaimer.							//
//	- Redistributions in binary form must reproduce the above copyright notice,		//
//	  this list of conditions and the following disclaimer in the documentation		//
//	  and/or other materials provided with the distribution.						//
//																					//	
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"		//
//	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE		//
//	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE		//
//	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE		//
//	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR				//
//	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF			//
//	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS		//
//	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN			//
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)			//
//	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE		//
//	POSSIBILITY OF SUCH DAMAGE.														//
//																					//
//	VERSION 1.0.0 - INITIAL VERSION													//
//	VERSION 2.0.0 - 06/01/2016 - REWROTE SPI INTERFACE AND ERROR HANDLING			//
//	VERSION 4.0.0 - 01/01/2017 - REWROTE SPI INTERFACE AND ERROR HANDLING			//
//	VERSION 5.1.0 - 04/05/2017 - USE NEW DEBUG LIBRARY								//
//	VERSION 5.2.0 - 20/05/2017 - USE NEW DEBUG LIBRARY								//
//	Version 6.0.0 -- June 2017 - USE NEW DEBUG LIBRARY								//
//	Version 7.0.0 -- June 2017 - REWROTE SPI INTERFACE								//
//	Version 7.0.1 -- June 2017 - FIXED BUG ON MEGA									//
//	Version 7.1.0 - 20/10/2017 - AVOID AUTOMATIC SOCKET CLOSE FOR DELETED CLIENTS	//
//	Version 7.3.0 - 17/12/2017 - ADD WIFI MODULE DEEP SLEEP COMMAND					//
//	Version 7.3.2 - 05/01/2018 - FIXED RSSI AND MAC HANDLING/DISPLAY				//
//																					//
//////////////////////////////////////////////////////////////////////////////////////
#include "FishinoSPIMaster.h"

#include <FishinoFlash.h>

#include <SPI.h>

#include "FishinoErrors.h"
#include "FishinoCommands.h"

#include "Fishino.h"

#define DEBUG_LEVEL_ERROR
#include <FishinoDebug.h>


#ifdef DEBUG_LEVEL_ERROR
	#define DEBUG_ERROR_CMD(s, ...) DEBUG_ERROR("CMD:%d - " s, Fishino.getLastCommand(), ##__VA_ARGS__)
#else
	#define DEBUG_ERROR_CMD(s, ...)
#endif

// SPI MODES :
//			CPOL	CPHA
// MODE 0	 0		 0
// MODE 1	 0		 1
// MODE 2	 1		 0
// MODE 3	 1		 1
static SPISettings &_spiSettings()
{
#if defined(FISHINO32)
	static SPISettings settings(12000000, MSBFIRST, SPI_MODE0);
#elif defined(FISHINO_PIRANHA)
	static SPISettings settings(10000000, MSBFIRST, SPI_MODE0);
#else
	static SPISettings settings(8000000, MSBFIRST, SPI_MODE0);
#endif
	return settings;
}

// last sent status
uint16_t FishinoSPIMasterClass::_lastSentStatus = 0;
	
// interrupted status flag
bool FishinoSPIMasterClass::_interrupted = false;
	
// last error code,if any
uint16_t FishinoSPIMasterClass::_lastError = 0;

// ESP SPI commands
enum ESP_SPI_COMMANDS
{
	ESP_STATUS_READ			= 0x04,
	ESP_STATUS_WRITE		= 0X01,
	ESP_BUFFER_READ			= 0x03,
	ESP_BUFFER_WRITE		= 0x02

} ESP_SPI_COMMANDS;

//////////////////////////////////////////////////////////////////////////////////////////

inline static void csLow(void)
{
	::digitalWrite(WIFICS, LOW);
}

inline static void csHigh(void)
{
	::digitalWrite(WIFICS, HIGH);
}

// pic32 is too fast sometimes
#ifdef _FISHINO_PIC32_xx

	#define scNsDelay(nsDelay)                    \
	{                                               \
	   register uint32_t startCnt = _CP0_GET_COUNT(); \
	   register uint32_t waitCnt = nsDelay / ( 1000000000L / (__PIC32_pbClk / 2) );  \
	   while( _CP0_GET_COUNT() - startCnt < waitCnt ) ; \
	}

	#define DELAY_RDSTATUS_SELECT		delayMicroseconds(2)
	#define DELAY_RDSTATUS_DESELECT
	
	#define DELAY_WRSTATUS_SELECT
	#define DELAY_WRSTATUS_DESELECT		delayMicroseconds(6)
	
	#define DELAY_RDBUF_SELECT
	#define DELAY_RDBUF_DESELECT		delayMicroseconds(6)
	
	#define DELAY_WRBUF_SELECT			delayMicroseconds(1)
	// 2 is also ok
	#define DELAY_WRBUF_DESELECT		delayMicroseconds(6)



/*
	#define DELAY_RDSTATUS_SELECT		delayMicroseconds(20)
	#define DELAY_RDSTATUS_DESELECT		delayMicroseconds(20)
	
	#define DELAY_WRSTATUS_SELECT		delayMicroseconds(20)
	#define DELAY_WRSTATUS_DESELECT		delayMicroseconds(20)
	
	#define DELAY_RDBUF_SELECT			delayMicroseconds(20)
	// 2 is also ok
	#define DELAY_RDBUF_DESELECT		delayMicroseconds(20)
	
	#define DELAY_WRBUF_SELECT			delayMicroseconds(20)
	#define DELAY_WRBUF_DESELECT		delayMicroseconds(20)
*/

	
#else

	#define DELAY_RDSTATUS_SELECT
	#define DELAY_RDSTATUS_DESELECT
	
	#define DELAY_WRSTATUS_SELECT
	#define DELAY_WRSTATUS_DESELECT
	
	#define DELAY_RDBUF_SELECT
	#define DELAY_RDBUF_DESELECT
	
	#define DELAY_WRBUF_SELECT
	#define DELAY_WRBUF_DESELECT
#endif

//////////////////////////////////////////////////////////////////////////////////////////

// show status bytes in human readable word
#ifdef DEBUG_LEVEL_INFO

	#define STATUSLINE(x) if(stat & x) DEBUG_PRINT(#x" "); bad = false
	static void printStatus(uint16_t stat)
	{
		bool bad = true;
		
		STATUSLINE(SLV_WAITING_COMMAND	);
		STATUSLINE(SLV_COMMAND_OK		);
		STATUSLINE(SLV_COMMAND_ACK		);
		STATUSLINE(SLV_COMMAND_ERROR	);
		STATUSLINE(SLV_RECEIVING_DATA	);
		STATUSLINE(SLV_RECEIVE_OK		);
		STATUSLINE(SLV_RECEIVE_ERROR	);
		STATUSLINE(SLV_SENDING_DATA		);
		STATUSLINE(SLV_SEND_OK			);
		STATUSLINE(MST_SENDING_COMMAND	);
		STATUSLINE(MST_COMMAND_ACK		);
		STATUSLINE(MST_RECEIVE_DONE		);
		STATUSLINE(MST_RECEIVE_OK		);
		STATUSLINE(MST_SEND_DONE		);
		STATUSLINE(MST_SEND_OK			);

		STATUSLINE(OUT_OF_SYNC			);
		
		if(bad)
			DEBUG_PRINT("*BAD STATUS*");
	}
	
	#define DEBUG_INFO_STATUS(s, stat)					\
		{												\
			DEBUG_INFO(s"(%" PRIu16 ")", stat);\
			printStatus(stat);							\
			DEBUG_INFO_N("\n");							\
		}


#else

	#define DEBUG_INFO_STATUS(s, stat)
	
#endif

// read slave's status -- return 0 on error, a status value on success
uint16_t FishinoSPIMasterClass::readStatus(void)
{
	uint16_t lowStat, highStat;
	
	// select the interface
	WIFISPI.beginTransaction(_spiSettings());
	csLow();
	DELAY_RDSTATUS_SELECT;
	
	WIFISPI.transfer((uint8_t)ESP_STATUS_READ);
	lowStat = WIFISPI.transfer((uint8_t)0x00);
	lowStat |= ((uint16_t)WIFISPI.transfer((uint8_t)0x00)) << 8;
	highStat = WIFISPI.transfer((uint8_t)0x00);
	highStat |= ((uint16_t)WIFISPI.transfer((uint8_t)0x00)) << 8;

	// deselect the interface
	DELAY_RDSTATUS_DESELECT;
	csHigh();
	WIFISPI.endTransaction();
	
	if(lowStat == (highStat ^ 0xffff))
		return lowStat;
//	DEBUG_ERROR("Bad status checksum : %04" PRIx16 ", %04" PRIx16 "\n", lowStat, highStat);
	
	return 0;
}

// read slave's status
// used just to send an interrupt to SPI loop
void FishinoSPIMasterClass::writeStatus(uint16_t stat)
{
	if(_lastSentStatus != stat)
		DEBUG_INFO_STATUS("Sending status ", stat);
	_lastSentStatus = stat;
	
	// select the interface
	WIFISPI.beginTransaction(_spiSettings());
	csLow();
	DELAY_WRSTATUS_SELECT;
		
	WIFISPI.transfer((uint8_t)ESP_STATUS_WRITE);
	WIFISPI.transfer((uint8_t)stat);
	WIFISPI.transfer((uint8_t)(stat >> 8));
	stat ^= 0xffff;
	WIFISPI.transfer((uint8_t)stat);
	WIFISPI.transfer((uint8_t)(stat >> 8));
	
	// deselect the interface
	DELAY_WRSTATUS_DESELECT;
	csHigh();
	WIFISPI.endTransaction();
}

// send and out of sync condition and wait till slave status become SLV_WAITING_COMMAND
// signaling that previous command has been aborted and it's ready to process
// a new one
void FishinoSPIMasterClass::sendOutOfSync(void)
{
	_interrupted = true;
	DEBUG_ERROR("Sending out of sync to slave\n");
	uint32_t tim = millis() + SLV_RESYNC_TIMEOUT;
	while(readStatus() != SLV_WAITING_COMMAND && millis() < tim)
	{
		delay(1);
		writeStatus(OUT_OF_SYNC);
	}
}

// acknowledge an out-of-sync condition pushing MST_SEND_COMMAND and waiting for SLV_WAITING_COMMAND
void FishinoSPIMasterClass::acknowledgeOutOfSync(void)
{
	_interrupted = true;
	uint32_t tim = millis() + SLV_RESYNC_TIMEOUT;
	while(readStatus() != SLV_WAITING_COMMAND && millis() < tim)
	{
		delay(1);
		writeStatus(MST_SENDING_COMMAND);
	}
}

// wait for a status from slave
// return the got one
uint16_t FishinoSPIMasterClass::waitStatus(uint16_t recvStat, uint32_t timeout)
{
	uint16_t rec;
	DEBUG_INFO_STATUS("Waiting for status ", recvStat);

	// fast path	
	for(uint8_t i = 0; i < 100; i++)
	{
//		writeStatus();
		rec = readStatus();
		if(rec & recvStat)
		{
			DEBUG_INFO_STATUS("Got status", rec);
			return true;
		}
		if(rec == OUT_OF_SYNC)
		{
			DEBUG_ERROR("Got out of sync (timeout %" PRIu32 " mSec) from slave -- try to resync\n", timeout);
			acknowledgeOutOfSync();
			return false;
		}
//		delayMicroseconds(1);
	}

	// slow path
	uint32_t longTim = millis() + timeout;
	while(millis() < longTim)
	{
//		writeStatus();
		rec = readStatus();
		if(rec & recvStat)
		{
			DEBUG_INFO_STATUS("Got status", rec);
			return true;
		}
		if(rec == OUT_OF_SYNC)
		{
			DEBUG_ERROR("Got out of sync (timeout %" PRIu32 " mSec) from slave -- try to resync\n", timeout);
			acknowledgeOutOfSync();
			return false;
		}
//		delay(SLV_SHORT_TIMEOUT);
	}
	
	// nope, could not get it, resync
	DEBUG_ERROR("Timeout (%" PRIu32 " mSec) -- sending out-of-sync to slave\n", timeout);
	DEBUG_INFO_STATUS("Timeout -- try to resync -- Last status was ", rec);
	sendOutOfSync();
	return false;

}

//////////////////////////////////////////////////////////////////////////////////////////

// read a chunk of data from slave
bool FishinoSPIMasterClass::readChunk(uint8_t *buf, uint8_t len, uint32_t timeout)
{
	// wait till slave is ready, or timeout
	if(!waitStatus(SLV_SENDING_DATA, timeout))
		return false;
	
	// select the interface
	WIFISPI.beginTransaction(_spiSettings());
	csLow();
	DELAY_RDBUF_SELECT;
	
	// write command + dummy address
	WIFISPI.transfer((uint8_t)ESP_BUFFER_READ);
	WIFISPI.transfer((uint8_t)0);

	// transfer all bytes
	while(len--)
		*buf++ = WIFISPI.transfer((uint8_t)0x00);

	// deselect the interface
	DELAY_RDBUF_DESELECT;
	csHigh();
	WIFISPI.endTransaction();
	
	// signal we're done receiving
	writeStatus(MST_RECEIVE_OK);

	if(!waitStatus(SLV_SEND_OK, SPI_ACK_TIMEOUT))
		return false;

	// signal we're done receiving
	writeStatus(MST_RECEIVE_DONE);

	return true;
}

// write a chunk of data to slave
bool FishinoSPIMasterClass::writeChunk(uint8_t const *buf, uint8_t len, uint32_t timeout)
{
	// wait till slave is ready, or timeout
	if(!waitStatus(SLV_RECEIVING_DATA, timeout))
		return false;
	
	// select the interface
	WIFISPI.beginTransaction(_spiSettings());
	csLow();
	DELAY_WRBUF_SELECT;

	// write command + dummy address
	WIFISPI.transfer((uint8_t)ESP_BUFFER_WRITE);
	WIFISPI.transfer((uint8_t)0);

	// transfer all bytes
	while(len--)
		WIFISPI.transfer(*buf++);

	// deselect the interface
	DELAY_WRBUF_DESELECT;
	csHigh();
	WIFISPI.endTransaction();
	
	// signal we're done sending
	writeStatus(MST_SEND_OK);

	if(!waitStatus(SLV_RECEIVE_OK, SPI_ACK_TIMEOUT))
		return false;

	// signal we're done sending
	writeStatus(MST_SEND_DONE);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

// read 8 bit data from slave
bool FishinoSPIMasterClass::read8(uint8_t &b)
{
	return readChunk(&b, 1, SPI_READY_TIMEOUT);
}

// write 8 bit data to slave
bool FishinoSPIMasterClass::write8(uint8_t b)
{
	return writeChunk(&b, 1, SPI_READY_TIMEOUT);
}

// read 16 bit data from slave
bool FishinoSPIMasterClass::read16(uint16_t &w)
{
	return readChunk((uint8_t *)&w, 2, SPI_READY_TIMEOUT);
}

// write 16 bit data to slave
bool FishinoSPIMasterClass::write16(uint16_t w)
{
	return writeChunk((uint8_t const *)&w, 2, SPI_READY_TIMEOUT);
}

// read 32 bit data from slave
bool FishinoSPIMasterClass::read32(uint32_t &dw)
{
	return readChunk((uint8_t *)&dw, 4, SPI_READY_TIMEOUT);
}

// write 32 bit data to slave
bool FishinoSPIMasterClass::write32(uint32_t dw)
{
	return writeChunk((uint8_t const *)&dw, 4, SPI_READY_TIMEOUT);
}

// read a buffer of data from slave
bool FishinoSPIMasterClass::read(uint8_t *buf, uint32_t len)
{
	while(len > 64)
	{
		if(!readChunk(buf, 64, SPI_READY_TIMEOUT))
			return false;
		buf += 64;
		len -= 64;
	}

	if(len)
		return readChunk(buf, len, SPI_READY_TIMEOUT);
	return true;
}

// write a buffer of data to slave
bool FishinoSPIMasterClass::write(uint8_t const *buf, uint32_t len)
{
	while(len > 64)
	{
		if(!writeChunk(buf, 64, SPI_READY_TIMEOUT))
			return false;
		buf += 64;
		len -= 64;
	}

	if(len)
		return writeChunk(buf, len, SPI_READY_TIMEOUT);
	return true;
}

// read a string from slave
// it allocates dynamically the buffer which must be freed by caller
bool FishinoSPIMasterClass::readString(char *&s)
{
	// initialize result
	s = NULL;
	
	// read string size
	uint32_t len;
	if(!read32(len) || !len)
		return false;
	char *buf = (char *)malloc(len + 1);
	if(!buf)
		return false;
	if(!read((uint8_t *)buf, len))
	{
		free(buf);
		return false;
	}
	buf[len] = 0;
	s = buf;
	return true;
}

// writes a string to slave
bool FishinoSPIMasterClass::writeString(const char *s)
{
	// just to be sure
	if(!s)
		return false;
	
	// write string length
	uint32_t len = strlen(s);
	if(!write32(len))
		return false;
	
	// write data
	return write((uint8_t const *)s, len);
}

bool FishinoSPIMasterClass::writeString(const __FlashStringHelper *s)
{
#ifdef _FISHINO_PIC32_
	return writeString((const char *)s);
#else
	char *buf = strdup(s);
	if(!buf)
		return false;
	bool res = writeString(buf);

	free(buf);
	return res;
#endif
}

// read an IP from slave
bool FishinoSPIMasterClass::readIP(IPAddress &ip)
{
	uint32_t dw;
	if(!read32(dw))
		return false;
	ip = dw;
	return true;
}

// write an IP to slave
bool FishinoSPIMasterClass::writeIP(IPAddress const &ip)
{
	uint32_t dw = ip;
	return write32(dw);
}

// write an 8 bit command to master
bool FishinoSPIMasterClass::writeCommand(uint8_t cmd)
{
	DEBUG_INFO_N("\n\n");
	
	// reset any interrupted state
	_interrupted = false;
	
	uint16_t command = cmd | (((uint16_t)(cmd ^ 0xff)) << 8);
	DEBUG_INFO("Sending command %" PRIx16 "\n", command);
	
	// wait for slave to be ready for command
	// we use a small timeout here
	if(!waitStatus(SLV_WAITING_COMMAND, SPI_CMDREADY_TIMEOUT))
		return false;

	// signal we're done sending
	writeStatus(MST_SENDING_COMMAND);

	DEBUG_INFO("Writing command\n");
	if(!write16(command))
	{
		DEBUG_ERROR("write16() error\n");
		return false;
	}

	// now check if command is ok
	uint16_t res = waitStatus(SLV_COMMAND_OK | SLV_COMMAND_ERROR, SPI_CMDREADY_TIMEOUT);
	if(!res)
		return false;
	
	// signal command read done
	writeStatus(MST_COMMAND_ACK);

	if(res == SLV_COMMAND_ERROR)
	{
		DEBUG_INFO("Got command error status from slave - bailing out\n");
		return false;
	}

	DEBUG_INFO("Sending command %" PRIx16 " OK\n", command);
	return true;
}

// read command result from master
bool FishinoSPIMasterClass::readCommandResult(uint32_t timeout)
{
	uint8_t res;
#ifdef __FISHINO_ERRORS_DETAILED__
	static bool reenter = false;
#endif
	if(!readChunk(&res, 1, timeout))
		return false;
	if(!res)
	{
		// read error code
		if(!read16(_lastError))
			_lastError = ERROR_SPI_LOCAL_GENERIC;
#ifdef __FISHINO_ERRORS_DETAILED__
		if(!reenter)
		{
			reenter = true;
			printLastErrorMessage();
			reenter = false;
		}
#endif
	}
	else
		_lastError = 0;
	return res;
}

// write e reset command to master
bool FishinoSPIMasterClass::writeReset(void)
{
	uint32_t tim = millis() + 5000;
	while(millis() < tim)
	{
#if defined(FISHINO32) || defined(FISHINO_PIRANHA)

		// pic32 fishino support hardware reset for ESP
		
		// send a reset pulse to esp
		digitalWrite(WIFIRESET, LOW);
		delay(50);
		digitalWrite(WIFIRESET, HIGH);
		delay(100);
		
#elif defined(FISHINO_SHARK)

		// pic32 fishino support hardware reset for ESP
		
		// send a reset pulse to esp
		digitalWrite(WIFICHPD, LOW);
		delay(50);
		digitalWrite(WIFICHPD, HIGH);
		delay(100);

#elif defined(_FISHINO_MEGA_R2_)

		// fishino MEGA R2 fishino support hardware reset for ESP
		pinMode(WIFICS, OUTPUT);
		digitalWrite(WIFICS, LOW);
		
		// send a reset pulse to esp
		digitalWrite(WIFI_CHPD, LOW);
		pinMode(WIFI_CHPD, OUTPUT);
		delay(50);
		pinMode(WIFI_CHPD, INPUT);
		delay(100);
		
		digitalWrite(WIFICS, HIGH);

#else
		// issue e software reset
		writeCommand(CMD_SYSRESET);
#endif
	
		uint32_t tim2 = millis() + 5000;
		while(millis() < tim2)
		{
			uint16_t stat = readStatus();
			// wait till slave command loop is ready
			if(stat == SLV_WAITING_COMMAND)
				return true;
#ifdef DEBUG_LEVEL_INFO
			printStatus(stat);
#endif
		}
	}

	DEBUG_ERROR("Reset failed\n");
	return false;
}

uint16_t FishinoSPIMasterClass::getLastError(void)
{
	return _lastError;
}

#ifdef __FISHINO_ERRORS_DETAILED__
void FishinoSPIMasterClass::printLastErrorMessage(void)
{
	// local messages can't be retrieved from remote
	// so we generate locally
	if(_lastError < 2000)
		DEBUG_ERROR("Unknown error %d\n", _lastError);
	// otherwise retrieve the message from host
	else
	{
		char *errStr;
		if(
			!writeCommand(CMD_SYSGETERRORSTR)	||
			!write16(_lastError)				||
			!readCommandResult()				||
			!readString(errStr)
		)
		{
			DEBUG_ERROR("Error retrienving error string");
		}
		else
		{
			DEBUG_ERROR("Last error:%s\n", errStr);
			free(errStr);
		}
	}
}
#endif


// constructor
FishinoSPIMasterClass::FishinoSPIMasterClass()
{
// ss pin MUST be an output for SPI to work
/*
#if defined(_FISHINO_UNO_R2_)
	pinMode(10, OUTPUT);
#elif defined(_FISHINO_MEGA_R2_)
	pinMode(SS, OUTPUT);
#endif
*/
}

// destructor
FishinoSPIMasterClass::~FishinoSPIMasterClass()
{
}

FishinoSPIMasterClass SPIMaster;
