//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								ReadBuf.h											//
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
#ifndef __READBUF_H
#define __READBUF_H

//#include "FishinoDbg.h"

#include "RefCounted.h"

#ifdef _FISHINO_PIC32_
#define FISHINO_RDBUF_SIZE	512
#elif ((RAMEND - RAMSTART) < 4096)
#define FISHINO_RDBUF_SIZE	16
#else
#define FISHINO_RDBUF_SIZE	250
#endif

template<class S, class C> class ReadBuf : public RefCounted<S, C>
{
	private:
	
		uint8_t _buf[FISHINO_RDBUF_SIZE];
		uint16_t _bufStart;
		uint16_t _bufCount;
		
		bool _availTwice;
		
		// refill buffer
		void refillBuffer(void)
		{
			_bufCount = C::readSocket(RefCounted<S, C>::getSocket(), _buf, FISHINO_RDBUF_SIZE);
			_bufStart = 0;
//			FISHINO_RDBUF_DEBUG_PRINT("REFILL - GOT %u BYTES", _bufCount);
		}
		
		uint16_t remoteAvail(void) { return C::availSocket(RefCounted<S, C>::getSocket()); }
		uint16_t remoteRead(uint8_t *buf, uint16_t reqSize) { return C::readSocket(RefCounted<S, C>::getSocket(), buf, reqSize); }
	
	protected:
	
	public:
	
		// constructor
		ReadBuf(S sock) : RefCounted<S, C>(sock)
		{
			_availTwice = false;
			_bufStart = 0;
			_bufCount = 0;
		}
	
		// check available data avoiding to call remote avail
		// if possible
		uint16_t avail(void)
		{
			if(!_availTwice && _bufCount)
			{
				_availTwice = true;
				return _bufCount;
			}
			_availTwice = false;
			return remoteAvail() + _bufCount;
		}
		
		// buffered data read
		uint16_t read(uint8_t *buf, uint16_t reqSize)
		{
//			FISHINO_RDBUF_DEBUG_PRINT("RDBUFREAD - REQUESTED %u BYTES", reqSize);
			_availTwice = false;
			uint16_t missing = reqSize;
			uint8_t *pBuf = buf;
			if(_bufCount)
			{
				if(missing <= _bufCount)
				{
//					FISHINO_RDBUF_DEBUG_PRINT("USING %u BYTES FROM BUFFER", missing);
					memcpy(pBuf, _buf + _bufStart, missing);
					_bufCount -= missing;
					_bufStart += missing;
					return missing;
				}
//				FISHINO_RDBUF_DEBUG_PRINT("STARTING WITH %u BYTES FROM BUFFER", _bufCount);
				memcpy(pBuf, _buf + _bufStart, _bufCount);
				pBuf += _bufCount;
				missing -= _bufCount;
				_bufCount = 0;
			}
			if(missing > FISHINO_RDBUF_SIZE)
			{
				uint16_t got = remoteRead(pBuf, missing);
//				FISHINO_RDBUF_DEBUG_PRINT("BLOCKREAD - REQUESTED %u GOT %u BYTES", missing, got);
				return reqSize - (missing - got);
			}
			
//			FISHINO_RDBUF_DEBUG_PRINT("READING LAST %u BYTES USING BUFFER", missing);
			refillBuffer();
			if(!_bufCount)
			{
//				FISHINO_RDBUF_DEBUG_PRINT("FAILED");
				return reqSize - missing;
			}
			uint16_t last = missing;
			if(_bufCount < last)
				last = _bufCount;
			memcpy(pBuf, _buf, last);
			_bufCount -= last;
			_bufStart += last;
			missing -= last;
//			FISHINO_RDBUF_DEBUG_PRINT("DONE - MISSING %u BYTES", reqSize - missing);
			return reqSize - missing;
		}
		
		// peeks next byte
		bool peek(uint8_t &b)
		{
			if(_bufCount)
			{
				b = _buf[_bufStart];
				return true;
			}
			refillBuffer();
			if(!_bufCount)
				return false;
			b = _buf[0];
			return true;
		}
};


#endif
