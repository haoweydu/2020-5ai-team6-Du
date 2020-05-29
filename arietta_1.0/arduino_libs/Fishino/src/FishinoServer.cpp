//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								FishinoServer.cpp									//
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

#define DEBUG_LEVEL_ERROR
#include "FishinoDebug.h"

#define FISHINO_MODULE "FishinoServer"
#include "Fishino.h"

FishinoServer::FishinoServer(uint16_t port)
{
	ref = NULL;
	_port = port;
	_accepting = false;
}

FishinoServer::~FishinoServer()
{
	if(ref)
		ref->remRef();
}

void FishinoServer::begin()
{
	// terminate the reference
	if(ref)
		ref->remRef();
	ref = NULL;
	_accepting = false;
	
	// start server with 100 seconds timeout
	uint8_t sock = Fishino.startServer(_port, 100);
	if(sock == 0xff)
		return;

	// create reference object
	ref = new RefCounted<uint8_t, FishinoServer>(sock);
	_accepting = true;
}

void FishinoServer::stop()
{
	if(ref)
	{
		Fishino.stopServer(_port);
		ref->remRef();
	}
	ref = NULL;
	_accepting = false;
}

bool FishinoServer::hasClients()
{
	if(!ref)
		return false;

	return Fishino.serverHasClient(ref->getSocket());
}

FishinoClient FishinoServer::available()
{
	if(!ref)
		return FishinoClient();

	uint8_t sock = Fishino.serverAvail(ref->getSocket());
	if(sock == 0xff)
		return FishinoClient();
	return FishinoClient(sock);
}

void FishinoServer::setNoDelay(bool n)
{
	if(!ref)
		return;

	Fishino.serverSetNoDelay(ref->getSocket(), n);
}

bool FishinoServer::getNoDelay(void)
{
	if(!ref)
		return false;

	return Fishino.serverGetNoDelay(ref->getSocket());
}

size_t FishinoServer::write(uint8_t b)
{
	return write(&b, 1);
}

size_t FishinoServer::write(const uint8_t *buffer, size_t size)
{
	if(!ref)
		return 0;

	uint16_t written;
	if(!Fishino.serverWrite(ref->getSocket(), buffer, size, written))
		return 0;
	return written;
}

// set server buffered mode
bool FishinoServer::setBufferedMode(bool b)
{
	if(!ref)
		return false;
	return Fishino.serverSetBufferedMode(ref->getSocket(), b);
}

// get server buffered mode
bool FishinoServer::getBufferedMode(void)
{
	if(!ref)
		return false;
	return Fishino.serverGetBufferedMode(ref->getSocket());
}

// set server clients timeout
bool FishinoServer::setClientsForceCloseTime(uint32_t tim)
{
	if(!ref)
		return false;
	return Fishino.serverSetClientsForceCloseTime(ref->getSocket(), tim);
}

// get server clients timeout
uint32_t FishinoServer::getClientsForceCloseTime(void)
{
	if(!ref)
		return 0;
	return Fishino.serverGetClientsForceCloseTime(ref->getSocket());
}

// set max number of server clients
bool FishinoServer::setMaxClients(uint8_t n)
{
	if(!ref)
		return false;
	return Fishino.serverSetMaxClients(ref->getSocket(), n);
}

// get max number of server clients
uint8_t FishinoServer::getMaxClients(void)
{
	if(!ref)
		return 0;
	return Fishino.serverGetMaxClients(ref->getSocket());
}

