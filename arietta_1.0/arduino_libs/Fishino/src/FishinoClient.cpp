//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								FishinoClient.cpp									//
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

#define FISHINO_MODULE "FishinoClient"
#include "Fishino.h"

//uint16_t EthernetClient::_srcport = 49152;      //Use IANA recommended ephemeral port range 49152-65535

FishinoClient::FishinoClient()
{
	ref = NULL;
	ssl = false;
}

// copy constructor
FishinoClient::FishinoClient(FishinoClient const &c)
{
	ref = c.ref;
	if(ref)
		ref->addRef();
	ssl = c.ssl;
}

// socket constructor
FishinoClient::FishinoClient(uint8_t sock)
{
	ref = NULL;
	ssl = false;
	attach(sock);
}

FishinoClient::~FishinoClient()
{
	if(ref)
		ref->remRef();
}

// copy operator
FishinoClient const &FishinoClient::operator=(FishinoClient const &c)
{
	if(ref)
		ref->remRef();
	ref = c.ref;
	if(ref)
		ref->addRef();
	ssl = c.ssl;
	return *this;
}

// attach to an already opene socket
void FishinoClient::attach(uint8_t sock)
{
	// disconnect from any previous socket
	if(ref)
		ref->remRef();
	ref = NULL;
	
	// create reference object
	ref = new ReadBuf<uint8_t, FishinoClient>(sock);
	
	return;
}

int FishinoClient::connect(const char* host, uint16_t port)
{
	// disconnect from any previous socket
	if(ref)
		ref->remRef();
	ref = NULL;
	
	// connect
	uint8_t sock = Fishino.connect(host, port, ssl);
	if (sock == 0xff)
		return false;

	// create reference object
	ref = new ReadBuf<uint8_t, FishinoClient>(sock);
	
	return true;
}

int FishinoClient::connect(IPAddress ip, uint16_t port)
{
	// disconnect from any previous socket
	if(ref)
		ref->remRef();
	ref = NULL;
	
	uint8_t sock = Fishino.connect(ip, port, ssl);
	if (sock == 0xff)
		return false;

	// create reference object
	ref = new ReadBuf<uint8_t, FishinoClient>(sock);
	
	return true;
}

size_t FishinoClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t FishinoClient::write(const uint8_t *buf, size_t size)
{
	if(!ref)
	{
		setWriteError();
		return 0;
	}
	bool res = Fishino.write(ref->getSocket(), buf, size);
	if(res)
		return size;

	setWriteError();
	return 0;
}

int FishinoClient::available()
{
	if(!ref)
		return 0;

	return ref->avail();
}

int FishinoClient::read()
{
	if(!ref)
		return -1;
	uint8_t b;
	if(ref->read(&b, 1) == 1)
		return b;
	return -1;
}

int FishinoClient::read(uint8_t *buf, size_t size)
{
	if(!ref)
		return 0;
	
	return ref->read(buf, size);
}

int FishinoClient::peek()
{
	if(!ref)
		return -1;
	
	uint8_t b;
	if(ref->peek(b))
		return b;
	return -1;
}

void FishinoClient::flush()
{
	if(!ref)
		return;
	
	Fishino.flush(ref->getSocket());
}

void FishinoClient::stop()
{
	if(!ref)
		return;

Fishino.disconnect(ref->getSocket());
	ref->remRef();
	ref = NULL;
}

uint8_t FishinoClient::connected()
{
	if(!ref)
		return false;
	
	return Fishino.connected(ref->getSocket()) || ref->avail();
}

bool FishinoClient::status()
{
	if(!ref)
		return 0;

	bool stat = (Fishino.status(ref->getSocket()) != CLOSED) || ref->avail();
	if(!stat)
	{
		ref->remRef();
		ref = NULL;
	}
	return stat;
}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.

FishinoClient::operator bool()
{
	return status();
}

// set buffered mode for tcp client
bool FishinoClient::setBufferedMode(bool b)
{
	if(!ref)
		return false;

	return Fishino.setBufferedMode(ref->getSocket(), b);
}

// get buffered mode for tcp client
bool FishinoClient::getBufferedMode(void)
{
	if(!ref)
		return false;

	return Fishino.getBufferedMode(ref->getSocket());
}

// disable/enable nagle for tcp client
bool FishinoClient::setNoDelay(bool b)
{
	if(!ref)
		return false;

	return Fishino.setNoDelay(ref->getSocket(), b);
}

// get nagle status for tcp client
bool FishinoClient::getNoDelay(void)
{
	if(!ref)
		return false;

	return Fishino.getNoDelay(ref->getSocket());
}

// set inactivity timeout for tcp client
bool FishinoClient::setForceCloseTime(uint32_t tim)
{
	if(!ref)
		return false;

	return Fishino.setClientForceCloseTime(ref->getSocket(), tim);
}

// get inactivity timeout for tcp client
uint32_t FishinoClient::getForceCloseTime(void)
{
	if(!ref)
		return false;

	return Fishino.getClientForceCloseTime(ref->getSocket());
}

bool FishinoClient::operator==(const FishinoClient& rhs)
{
	return ref && ref == rhs.ref;
}

#ifndef _FISHINO_PIC32_
size_t FishinoClient::print(const __FlashStringHelper *s)
{
	if(!ref)
		return 0;

	if(Fishino.write(ref->getSocket(), s))
		return strlen_P(reinterpret_cast<PGM_P>(s));
	return 0;
}

size_t FishinoClient::println(const __FlashStringHelper *s)
{
	size_t n =	print(s);
	n += print(F("\r\n"));
	return n;
}
#endif

// verify secure socket domain fingerprint
bool FishinoSecureClient::verifyFingerPrint(const char *domain, const char *fingerPrint)
{
	if(!ref)
		return false;
	return Fishino.verifyFingerPrint(ref->getSocket(), domain, fingerPrint);
}

bool FishinoSecureClient::verifyFingerPrint(const __FlashStringHelper *domain, const __FlashStringHelper *fingerPrint)
{
	if(!ref)
		return false;
	return Fishino.verifyFingerPrint(ref->getSocket(), domain, fingerPrint);
}

// set client certificate
bool FishinoSecureClient::setClientCertificate(const uint8_t *buf, uint16_t len)
{
	return Fishino.setClientCertificate(buf, len);
}

bool FishinoSecureClient::setClientCertificate()
{
	return Fishino.setClientCertificate();
}

// det client private key
bool FishinoSecureClient::setClientPrivateKey(const uint8_t *buf, uint16_t len)
{
	return Fishino.setClientPrivateKey(buf, len);
}

bool FishinoSecureClient::setClientPrivateKey()
{
	return Fishino.setClientPrivateKey();
}

