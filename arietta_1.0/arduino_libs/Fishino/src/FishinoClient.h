//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								FishinoClient.h										//
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
#ifndef FISHINOCLIENT_H
#define FISHINOCLIENT_H

#include <Client.h>

#include "ReadBuf.h"

class FishinoClient : public Client
{
	template<class S, class C> friend class RefCounted;
	template<class S, class C> friend class ReadBuf;
	friend class FishinoServer;
	
	protected:

		// reference counting for copied objects
		ReadBuf<uint8_t, FishinoClient> *ref;
		
		// ssl flag
		bool ssl;
		
		// remove socket on last reference drop
		inline static void killSocket(uint8_t sock) { Fishino.disconnect(sock); }
		
		// remote read
		inline static uint16_t readSocket(uint8_t sock, uint8_t *buf, uint16_t reqSize)
		{
			uint16_t ret;
			if(Fishino.read(sock, reqSize, buf, ret))
				return ret;
			return 0;
		}
		
		// remote avail
		inline static uint16_t availSocket(uint8_t sock) { return Fishino.available(sock); }
		
		// attach to an already opened socket
		void attach(uint8_t sock);

		// socket constructor
		FishinoClient(uint8_t sock);

	public:
		FishinoClient();

		// copy constructor
		FishinoClient(FishinoClient const &c);

		// destructor, frees buffer
		virtual ~FishinoClient();

		// copy operator
		FishinoClient const &operator=(FishinoClient const &c);
		
		uint8_t getSocket(void) const { if(ref) return ref->getSocket(); else return 0xff; }

		bool status();

		virtual int connect(IPAddress ip, uint16_t port);
		virtual int connect(const char *host, uint16_t port);

		virtual size_t write(uint8_t);
		virtual size_t write(const uint8_t *buf, size_t size);

		virtual int available();

		virtual int read();
		virtual int read(uint8_t *buf, size_t size);

		virtual int peek();

		virtual void flush();

		virtual void stop();

		virtual uint8_t connected();
		virtual operator bool();
		
		// set buffered mode for tcp client
		bool setBufferedMode(bool b);
		
		// get buffered mode for tcp client
		bool getBufferedMode(void);
		
		// disable/enable nagle for tcp client
		bool setNoDelay(bool b);
		
		// get nagle status for tcp client
		bool getNoDelay(void);
		
		// set inactivity timeout for tcp client
		bool setForceCloseTime(uint32_t tim);

		// get inactivity timeout for tcp client
		uint32_t getForceCloseTime(void);

		virtual bool operator==(const FishinoClient&);
		virtual bool operator!=(const FishinoClient& rhs)
		{
			return !operator==(rhs);
		}

		using Print::write;
		using Print::print;
		using Print::println;
		
#ifndef _FISHINO_PIC32_
		size_t print(const __FlashStringHelper *s);
		size_t println(const __FlashStringHelper *s);
		FishinoClient &operator<<(const __FlashStringHelper *s)	{ print(s); return *this; }
		FishinoClient &operator<<(const FlashString &s) { print((const __FlashStringHelper *)s); return *this; }
		FishinoClient &operator<<(const char *s) { print(s); return *this;}
#endif
};


class FishinoSecureClient : public FishinoClient
{
	private:

	protected:

	public:

		FishinoSecureClient() : FishinoClient() { ssl = true; };
		FishinoSecureClient(uint8_t sock) : FishinoClient(sock) { ssl = true; };

		// copy constructor
		FishinoSecureClient(FishinoSecureClient const &c) : FishinoClient(c) { ssl = true; };

		// copy operator
		FishinoSecureClient const &operator=(FishinoSecureClient const &c)
		{
			FishinoSecureClient const &res = (FishinoSecureClient const &)FishinoClient::operator=(c);
			ssl = true;
			return res;
		}

		// verify secure socket domain fingerprint
		bool verifyFingerPrint(const char *domain, const char *fingerPrint);
		bool verifyFingerPrint(const __FlashStringHelper *domain, const __FlashStringHelper *fingerPrint);
		
		// set client certificate
		static bool setClientCertificate(const uint8_t *buf, uint16_t len);
		static bool setClientCertificate();
		
		// det client private key
		static bool setClientPrivateKey(const uint8_t *buf, uint16_t len);
		static bool setClientPrivateKey();
};

#endif
