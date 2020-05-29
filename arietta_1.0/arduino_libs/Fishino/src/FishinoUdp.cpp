//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								FishinoUdp.cpp										//
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

//#define DEBUG_LEVEL_INFO
#include "FishinoDebug.h"

#define FISHINO_MODULE "FishinoUdp"
#include "Fishino.h"

// Constructor
FishinoUDP::FishinoUDP()
{
	ref = NULL;
}

// Destructor
FishinoUDP::~FishinoUDP()
{
	if(ref)
		ref->remRef();
}

// copy constructor
FishinoUDP::FishinoUDP(FishinoUDP const &c)
{
	ref = c.ref;
	if(ref)
		ref->addRef();
}

// copy operator
FishinoUDP const &FishinoUDP::operator=(FishinoUDP const &c)
{
	if(ref)
		ref->remRef();
	ref = c.ref;
	if(ref)
		ref->addRef();
	return *this;
}

// Start WiFiUDP socket, listening at local port PORT
uint8_t FishinoUDP::begin(uint16_t port)
{
	// disconnect from any previous socket
	if(ref)
		ref->remRef();
	ref = NULL;
	
	uint8_t sock = Fishino.udpBegin(port);
	if (sock == 0xff)
		return false;

	// create reference object
	ref = new ReadBuf<uint8_t, FishinoUDP>(sock);
	
	return true;
}

// return number of bytes available in the current packet,
// will return zero if parsePacket hasn't been called yet
int FishinoUDP::available()
{
	if(!ref)
		return 0;
	return Fishino.udpAvail(ref->getSocket());
}

// Release any resources being used by this WiFiUDP instance
void FishinoUDP::stop()
{
	if(!ref)
		return;

	ref->remRef();
	ref = NULL;
}

int FishinoUDP::beginPacket(const char *host, uint16_t port)
{
	if(!ref)
		return 0;
	return Fishino.udpBeginPacket(ref->getSocket(), host, port);
}

int FishinoUDP::beginPacket(IPAddress ip, uint16_t port)
{
	if(!ref)
		return 0;
	return Fishino.udpBeginPacket(ref->getSocket(), ip, port);
}

int FishinoUDP::endPacket()
{
	if(!ref)
		return 0;
	return Fishino.udpEndPacket(ref->getSocket());
}

size_t FishinoUDP::write(uint8_t b)
{
	if(!ref)
		return 0;
	uint8_t buf[1];
	buf[0] = b;
	uint16_t res;
	return Fishino.udpWrite(ref->getSocket(), buf, 1, res);
}

size_t FishinoUDP::write(const uint8_t *buffer, size_t size)
{
	if(!ref)
		return 0;
	uint16_t res;
	Fishino.udpWrite(ref->getSocket(), buffer, size, res);
	return res;
}

int FishinoUDP::parsePacket()
{
	if(!ref)
		return 0;
	return Fishino.udpParsePacket(ref->getSocket());
}

int FishinoUDP::read()
{
	if(!ref)
		return -1;
	
	uint8_t buf[1];
	uint16_t bufLen = 0;
	bool res = Fishino.udpRead(ref->getSocket(), 1, buf, bufLen);
	if(!res || bufLen != 1)
		return 0;
	else
		return buf[0];
}

int FishinoUDP::read(unsigned char* buffer, size_t len)
{
	if(!ref)
		return -1;
	
	uint16_t bufLen;
	bool res = Fishino.udpRead(ref->getSocket(), len, buffer, bufLen);
	if(res)
		return bufLen;
	else
		// hmmmm... not a nice result, indeed... would be better 0
		return -1;
}

int FishinoUDP::peek()
{
	if(!ref)
		return -1;
	
	uint8_t buf[1];
	uint16_t bufLen = 0;
	bool res = Fishino.udpPeek(ref->getSocket(), 1, buf, bufLen);
	if(!res || bufLen != 1)
		return 0;
	else
		return buf[0];
}

void FishinoUDP::flush()
{
	if(!ref)
		return;
	
	Fishino.udpFlush(ref->getSocket());
}

IPAddress  FishinoUDP::remoteIP()
{
	IPAddress ip;
	
	if(!ref)
		return ip;
	
	bool res = Fishino.udpRemoteIP(ref->getSocket(), ip);
	if(!res)
		return IPAddress(0, 0, 0, 0);
	return ip;
}

uint16_t  FishinoUDP::remotePort()
{
	uint16_t port;
	
	if(!ref)
		return 0;
	
	bool res = Fishino.udpRemotePort(ref->getSocket(), port);
	if(!res)
		return 0;
	else
		return port;
}
