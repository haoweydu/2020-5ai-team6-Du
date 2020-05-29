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
#ifndef __FISHINO_SPI_MASTER_H
#define __FISHINO_SPI_MASTER_H

#include <Arduino.h>
#include <IPAddress.h>
#include <SPI.h>

#ifndef WIFICS
#define WIFICS		10
#endif

#ifndef WIFISPI
#define WIFISPI		SPI
#endif

// timeout for SPI operations
#define SPI_READY_TIMEOUT			3001	// ready for data xfer timeout
#define SPI_CMDREADY_TIMEOUT		6002	// timeout for ready to receive command
#define SPI_ACK_TIMEOUT				4003	// acknowledge timeout
#define	SPI_CMDRES_TIMEOUT			7004	// command result timeout
#define SPI_INTR_ENTER_TIMEOUT		1005	// enter interrupt state timeout (upon interrupt() command)
#define SPI_INTR_LEAVE_TIMEOUT		1006	// leave interrupt state timeout (upon interruptAck() command)

// those replace SPI_CMDRES_TIMEOUT for lengthy operations
#define SPI_SOCKET_RW_TIMEOUT		10007	// timeout for read/write on sockets
#define SPI_SOCKET_NETSCAN_TIMEOUT	20008	// timeout for network scan
#define SPI_JOIN_TIMEOUT			11009	// timeout for join access points
#define SPI_SOCKET_CONN_TIMEOUT		10010	// timeout for socket connection (esp gives up after approx 7 seconds)

// timeout for acknowledge wait
#define		SLV_WAIT_LONG_TIMEOUT			2011
#define		SLV_RESYNC_TIMEOUT				512

// status values from slave to master
#define		SLV_WAITING_COMMAND				0b0000000000000001
#define		SLV_COMMAND_ACK					0b0000000000000010
#define		SLV_COMMAND_OK					0b0000000000000100
#define		SLV_COMMAND_ERROR				0b0000000000001000

#define		SLV_RECEIVING_DATA				0b0000000000010000
#define		SLV_RECEIVE_OK					0b0000000000100000
#define		SLV_RECEIVE_ERROR				0b0000000001000000
#define		SLV_SENDING_DATA				0b0000000010000000
#define		SLV_SEND_OK						0b0000000100000000

// status values from master to slave
#define		MST_SENDING_COMMAND				0b0000001000000000
#define		MST_COMMAND_ACK					0b0000010000000000
#define		MST_RECEIVE_DONE				0b0000100000000000
#define		MST_RECEIVE_OK					0b0001000000000000
#define		MST_SEND_DONE					0b0010000000000000
#define		MST_SEND_OK						0b0100000000000000

#define		OUT_OF_SYNC						0b1000000000000000

class FishinoSPIMasterClass
{
	private:
	
		// last sent status
		static uint16_t _lastSentStatus;
	
		// interrupted status flag
		static bool _interrupted;
	
		// last error code,if any
		static uint16_t _lastError;
	
		// read a chunk of data from slave
		static bool readChunk(uint8_t *buf, uint8_t len, uint32_t timeout);
		
		// write a chunk of data to slave
		static bool writeChunk(uint8_t const *buf, uint8_t len, uint32_t timeout);
		
		// read slave's status dword
		static uint16_t readStatus(void);
		
		// send a status word to slave
		static void writeStatus(uint16_t stat);
		static inline void writeStatus(void) { writeStatus(_lastSentStatus); }
		
		// send and out of sync condition and wait till slave status become SLV_WAITING_COMMAND
		// signaling that previous command has been aborted and it's ready to process
		// a new one
		static void sendOutOfSync(void);
		
		// acknowledge an out-of-sync condition pushing MST_SEND_COMMAND and waiting for SLV_WAITING_COMMAND
		static void acknowledgeOutOfSync(void);
		
		// wait for a status from slave
		// return the got one
		static uint16_t waitStatus(uint16_t recvStat, uint32_t timeout);

	protected:

	public:
	
		// read 8 bit data from slave
		static bool read8(uint8_t &b);
		
		// write 8 bit data to slave
		static bool write8(uint8_t b);

		// read 16 bit data from slave
		static bool read16(uint16_t &w);
		
		// write 16 bit data to slave
		static bool write16(uint16_t w);

		// read 32 bit data from slave
		static bool read32(uint32_t &dw);
		
		// write 32 bit data to slave
		static bool write32(uint32_t dw);

		// read a buffer of data from slave
		static bool read(uint8_t *buf, uint32_t len);
		
		// write a buffer of data to slave
		static bool write(uint8_t const *buf, uint32_t len);
		
		// read a string from slave
		// it allocates dynamically the buffer which must be freed by caller
		static bool readString(char *&s);
		
		// writes a string to slave
		static bool writeString(const char *s);
		static bool writeString(const __FlashStringHelper *s);
		
		// read an IP from slave
		static bool readIP(IPAddress &ip);
		
		// write an IP to slave
		static bool writeIP(IPAddress const &ip);
		
		// write an 8 bit command to master
		static bool writeCommand(uint8_t cmd);
		
		// read command result from master
		static bool readCommandResult(uint32_t timeout = SPI_CMDRES_TIMEOUT);
		
		// write e reset command to master
		static bool writeReset(void);
		
/*
		// check command execution result
		static bool checkCommandResult(void);
*/

		static uint16_t getLastError(void);
		static void printLastErrorMessage(void);

		// constructor
		FishinoSPIMasterClass();

		// destructor
		~FishinoSPIMasterClass();
};

extern FishinoSPIMasterClass SPIMaster;

#endif
