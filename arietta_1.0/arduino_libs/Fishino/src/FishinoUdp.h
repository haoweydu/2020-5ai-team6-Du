//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								FishinoUdp.h										//
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
#ifndef __FISHINOUDP_H
#define __FISHINOUDP_H

#include <Udp.h>

#define UDP_TX_PACKET_MAX_SIZE 24

class FishinoUDP : public UDP
{
//	friend class FishinoClass;

	template<class S, class C> friend class RefCounted;
	template<class S, class C> friend class ReadBuf;

	protected:

		// reference counting for copied objects
		ReadBuf<uint8_t, FishinoUDP> *ref;
		
		// remove socket on last reference drop
		inline static void killSocket(uint8_t sock) { Fishino.udpEnd(sock); }
		
		// remote read
		inline static uint16_t readSocket(uint8_t sock, uint8_t *buf, uint16_t reqSize)
		{
			uint16_t ret;
			if(Fishino.udpRead(sock, reqSize, buf, ret))
				return ret;
			return 0;
		}
		
		// remote avail
		inline static uint16_t availSocket(uint8_t sock) { return Fishino.udpAvail(sock); }
		
	public:
		// Constructor
		FishinoUDP();
		
		// copy constructor
		FishinoUDP(FishinoUDP const &c);

		// Destructor
		~FishinoUDP();
		
		// copy operator
		FishinoUDP const &operator=(FishinoUDP const &c);

		// initialize, start listening on specified port. Returns 1 if successful, 0 if there are no sockets available to use
		virtual uint8_t begin(uint16_t);

		// Finish with the UDP socket
		virtual void stop();

		// Sending UDP packets

		// Start building up a packet to send to the remote host specific in ip and port
		// Returns 1 if successful, 0 if there was a problem with the supplied IP address or port
		virtual int beginPacket(IPAddress ip, uint16_t port);

		// Start building up a packet to send to the remote host specific in host and port
		// Returns 1 if successful, 0 if there was a problem resolving the hostname or port
		virtual int beginPacket(const char *host, uint16_t port);

		// Finish off this packet and send it
		// Returns 1 if the packet was sent successfully, 0 if there was an error
		virtual int endPacket();

		// Write a single byte into the packet
		virtual size_t write(uint8_t);

		// Write size bytes from buffer into the packet
		virtual size_t write(const uint8_t *buffer, size_t size);

		using Print::write;

		// Start processing the next available incoming packet
		// Returns the size of the packet in bytes, or 0 if no packets are available
		virtual int parsePacket();

		// Number of bytes remaining in the current packet
		virtual int available();

		// Read a single byte from the current packet
		virtual int read();

		// Read up to len bytes from the current packet and place them into buffer
		// Returns the number of bytes read, or 0 if none are available
		virtual int read(unsigned char* buffer, size_t len);

		// Read up to len characters from the current packet and place them into buffer
		// Returns the number of characters read, or 0 if none are available
		virtual int read(char* buffer, size_t len)
		{
			return read((unsigned char*)buffer, len);
		};

		// Return the next byte from the current packet without moving on to the next byte
		virtual int peek();

		// Finish reading the current packet
		virtual void flush();

		// Return the IP address of the host who sent the current incoming packet
		virtual IPAddress remoteIP();

		// Return the port of the host who sent the current incoming packet
		virtual uint16_t remotePort();
};

#endif

