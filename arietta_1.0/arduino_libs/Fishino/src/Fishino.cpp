//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								Fishino.cpp											//
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

//#define WIFI_TIMINGS

#define DEBUG_LEVEL_ERROR
#define DEBUG
#include "FishinoDebug.h"

#include "Fishino.h"
#include "Arduino.h"
#include "FishinoCommands.h"


#ifdef WIFI_TIMINGS
	#define DEBUG
	
	#define TIM_START \
		uint32_t __tim = millis()
	
	#define TIM_END \
		__tim = millis() - __tim;\
		DEBUG_PRINT("Time : %" PRIu32 " mSec\n", __tim)
		
	#define TIM_END_SIZE(__siz)\
		__tim = millis() - __tim;\
		DEBUG_PRINT("Transferred %" PRIu32" bytes in %" PRIu32 " mSec\n", (uint32_t)__siz, __tim)
	
#else
	#define TIM_START
	#define TIM_END
	#define TIM_END_SIZE(__siz)
#endif


// some shortcuts (and possible debug hooks)
static inline bool __attribute((always_inline)) writeCommand(uint8_t cmd)
{
#ifdef DEBUG
	Fishino.setLastCommand(cmd);
#endif
	return SPIMaster.writeCommand(cmd);
}

static inline bool __attribute((always_inline)) readCommandResult(uint32_t timeout = SPI_CMDRES_TIMEOUT)
{
	return SPIMaster.readCommandResult(timeout);
}

static inline bool __attribute((always_inline)) read8(uint8_t &b)							{ return SPIMaster.read8(b);		}
static inline bool __attribute((always_inline)) read16(uint16_t &w)							{ return SPIMaster.read16(w);		}
static inline bool __attribute((always_inline)) read32(uint32_t &dw)						{ return SPIMaster.read32(dw);		}
static inline bool __attribute((always_inline)) readBuf(uint8_t *buf, uint32_t len)			{ return SPIMaster.read(buf, len);	}
static inline bool __attribute((always_inline)) readString(char *&s)						{ return SPIMaster.readString(s);	}
static inline bool __attribute((always_inline)) readIP(IPAddress &ip)						{ return SPIMaster.readIP(ip);	}
static inline bool __attribute((always_inline)) write8(uint8_t const &b)					{ return SPIMaster.write8(b);		}
static inline bool __attribute((always_inline)) write16(uint16_t const &w)					{ return SPIMaster.write16(w);		}
static inline bool __attribute((always_inline)) write32(uint32_t const &dw)					{ return SPIMaster.write32(dw);		}
static inline bool __attribute((always_inline)) writeBuf(uint8_t const *buf, uint32_t len)	{ return SPIMaster.write(buf, len);	}
static inline bool __attribute((always_inline)) writeString(const char *&s)					{ return SPIMaster.writeString(s);	}
static inline bool __attribute((always_inline)) writeString(const __FlashStringHelper *&s)	{ return SPIMaster.writeString(s);	}
static inline bool __attribute((always_inline)) writeIP(IPAddress const &ip)				{ return SPIMaster.writeIP(ip);		}
static inline bool __attribute((always_inline)) writeReset(void)							{ return SPIMaster.writeReset();	}

// helper for flash strings
char *strdup_P(const __FlashStringHelper *f)
{
	char *res = (char *)DEBUG_MALLOC(strlen_P((PGM_P)f) + 1);
	if(!res)
		return 0;
	strcpy_P(res, (PGM_P)f);
	return res;
}

// constructor
FishinoClass::FishinoClass()
{
	_lastCommand = 0;
	
	// initialize firmware version
	_fwVersion = -1;
	
	::digitalWrite(WIFICS, HIGH);
	::pinMode(WIFICS, OUTPUT);
	WIFISPI.begin();
}

// destructor
FishinoClass::~FishinoClass()
{
}

// reset ESP and wait for it to be ready
// return true if ok, false if not ready
bool FishinoClass::reset(void)
{
	return writeReset();
}

// Get firmware version
uint32_t FishinoClass::firmwareVersion()
{
	// no need to ask twice
	if(_fwVersion != (uint32_t)-1)
		return _fwVersion;
	
	uint8_t devel, minor;
	uint16_t major;
	
	if(
		!writeCommand(CMD_SYSGETFIRMWAREVERSION)	||
		!readCommandResult()						||
		!read8(devel)								||
		!read8(minor)								||
		!read16(major)
	)
		return -1;

	_fwVersion = (uint32_t)major << 16 | (uint32_t)minor << 8 | devel;
	return _fwVersion;
}

char *FishinoClass::firmwareVersionStr()
{
	static char ver[] = "00000.000.000";
	
	uint32_t fv = firmwareVersion();
	if(fv != (uint32_t)-1)
		sprintf(ver, "%u.%u.%u", (uint16_t)((fv >> 16) & 0xffff), (uint16_t)((fv >> 8) & 0xff), (uint16_t)(fv & 0xff));
	else
		strcpy_P(ver, (const char *)F("UNKNOWN"));
	return ver;
}


// set operation mode (one of WIFI_MODE)
bool FishinoClass::setMode(uint8_t mode)
{
	DEBUG_INFO("Setting mode %" PRIu8 "\n", mode);
	if(
		!writeCommand(CMD_WIFISETMODE)	||
		!write8(mode)						||
		!readCommandResult()
	)
	{
		DEBUG_ERROR("Error setting mode\n");
		return false;
	}

	DEBUG_INFO("Setmode OK\n");
	return true;
}

// get operation mode
uint8_t FishinoClass::getMode(void)
{
	uint8_t mode;

	if(
		!writeCommand(CMD_WIFIQUERYMODE)	||
		!readCommandResult()				||
		!read8(mode)
	)
		return NULL_MODE;

	return mode;
}

// Start Wifi connection for OPEN networks
// param ssid: Pointer to the SSID string.
// return one of JOIN_STATUS result
uint8_t FishinoClass::begin(const char* ssid)
{
	return begin(ssid, "");
}

uint8_t FishinoClass::begin(const __FlashStringHelper *ssid)
{
	return begin(ssid, F(""));
}
		
// Start Wifi connection with encryption.
// return boolean
uint8_t FishinoClass::begin(const char* ssid, const char *passphrase)
{
	TIM_START;
	
	if(
		!writeCommand(CMD_WIFIJOIN)			||
		!writeString(ssid)					||
		!writeString(passphrase)			||
		!readCommandResult(SPI_JOIN_TIMEOUT)
	)
		return false;
	TIM_END;
	return true;
}

uint8_t FishinoClass::begin(const __FlashStringHelper *ssid, const __FlashStringHelper *passphrase)
{
	TIM_START;
	if(
		!writeCommand(CMD_WIFIJOIN)			||
		!writeString(ssid)					||
		!writeString(passphrase)			||
		!readCommandResult(SPI_JOIN_TIMEOUT)
	)
		return false;
	
	TIM_END;
	return true;
}

// Change Ip configuration settings disabling the dhcp client
//	param local_ip
// WARNING : you should also set gateway and netmask (see next function)
// if you set only the IP, the system will default with :
// IP      : a, b, c, d
// gateway : a, b, c, 1
// netmask : 255, 255, 255, 0
// (which is OK for most but not all situations)
bool FishinoClass::config(IPAddress local_ip)
{
	IPAddress gw(local_ip[0], local_ip[1], local_ip[2], 1);
	IPAddress nm(255, 255, 255, 0);
	return config(local_ip, gw, nm);
}

// Change Ip configuration settings disabling the dhcp client
// param local_ip:			Static ip configuration
// param gateway :			Static gateway configuration
// param subnet:			Static Subnet mask
// WARNING : you should also set netmask (see next function)
// if you set only the IP and gateway, the system will default with :
// IP      : a, b, c, d
// gateway : (your gateway)
// netmask : 255, 255, 255, 0
// (which is OK for most but not all situations)
bool FishinoClass::config(IPAddress local_ip, IPAddress gateway)
{
	IPAddress nm(255, 255, 255, 0);
	return config(local_ip, gateway, nm);
}

// Change Ip configuration settings disabling the dhcp client
// param local_ip:			Static ip configuration
// param gateway :			Static gateway configuration
// param subnet:			Static Subnet mask
bool FishinoClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet)
{
	if(
		!writeCommand(CMD_WIFISETSTAIPINFO)	||
		!writeIP(local_ip)					||
		!writeIP(gateway)					||
		!writeIP(subnet)					||
		!readCommandResult()
	)
		return false;
	return true;
}

// Change Ip configuration settings disabling the dhcp client
// param local_ip:			Static ip configuration
// param gateway:			Static gateway configuration
// param subnet:			Static Subnet mask
// param dns_server:		IP configuration for DNS server 1
bool FishinoClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns_server)
{
	if(!config(local_ip, gateway, subnet))
		return false;
	return setDNS(dns_server);
}

// Change DNS Ip configuration
// param dns_server1:		ip configuration for DNS server 1
bool FishinoClass::setDNS(IPAddress dns_server1)
{
	if(
		!writeCommand(CMD_WIFISETSTADNS)	||
		!write8(0)							||
		!writeIP(dns_server1)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// Change DNS Ip configuration
// param dns_server1:		ip configuration for DNS server 1
// param dns_server2:		ip configuration for DNS server 2
bool FishinoClass::setDNS(IPAddress dns_server1, IPAddress dns_server2)
{
	if(!setDNS(dns_server1))
		return false;
	if(
		!writeCommand(CMD_WIFISETSTADNS)	||
		!write8(1)						||
		!writeIP(dns_server2)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// Disconnect from the network
// return: one value of wl_status_t enum
bool FishinoClass::disconnect(void)
{
	if(
		!writeCommand(CMD_WIFIQUIT)	||
		!readCommandResult()
	)
		return false;
	return true;
}

// Get the interface MAC address.
// return: pointer to A STATIC uint8_t array with length WL_MAC_ADDR_LENGTH
const uint8_t* FishinoClass::macAddress(void)
{
	static uint8_t MAC[6];
	const uint16_t macBufLen = sizeof(MAC);

	if(
		!writeCommand(CMD_WIFIGETSTAMAC)	||
		!readCommandResult()				||
		!readBuf(MAC, macBufLen)
	)
		return MAC;
	return MAC;
}

// issue an IP-Querying command
// return: Ip address value
IPAddress FishinoClass::getIP(uint8_t ipCmd)
{
	IPAddress ip;
	if(
		!writeCommand(ipCmd)			||
		!readCommandResult()			||
		!readIP(ip)
	)
		return ip;
	return ip;
}


// Get the interface IP address.
// return: Ip address value
IPAddress FishinoClass::localIP()
{
	return getIP(CMD_WIFIGETSTAIP);
}

// Get the interface subnet mask address.
// return: subnet mask address value
IPAddress FishinoClass::subnetMask()
{
	return getIP(CMD_WIFIGETSTANM);
}

// Get the gateway ip address.
// return: gateway ip address value
IPAddress FishinoClass::gatewayIP()
{
	return getIP(CMD_WIFIGETSTAGW);
}

// Return the current SSID associated with the network
// return: ssid string
String FishinoClass::SSID()
{
	char *ssid;
	if(
		!writeCommand(CMD_WIFIGETSSID)	||
		!write8(0xff)					||
		!readCommandResult()			||
		!readString(ssid)
	)
		return "";
	String ssidStr = ssid;
	free(ssid);
	return ssidStr;
}

// Return the current BSSID associated with the network.
// It is the MAC address of the Access Point
// return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
const uint8_t* FishinoClass::BSSID()
{
	if(
		!writeCommand(CMD_WIFIGETBSSID)	||
		!write8(0xff)					||
		!readCommandResult()			||
		!readBuf(_BSSID, 6)
	)
		memset(_BSSID, 0, 6);
	return _BSSID;
}

// Return the current RSSI /Received Signal Strength in dBm)
// associated with the network
// return: signed value
int32_t FishinoClass::RSSI()
{
	int8_t rssi = 0;
	if(
		!writeCommand(CMD_WIFIGETRSSI)	||
		!write8(0xff)					||
		!readCommandResult()			||
		!read8((uint8_t &)rssi)
	)
		return 0;
	return rssi;
}

// Return the Encryption Type associated with the network
// return: one value of wl_enc_type enum
uint8_t	FishinoClass::encryptionType()
{
	// @@ NOT IMPLEMENTED
	return 0;
}

// Start scan WiFi networks available
// return: Number of discovered networks
uint8_t FishinoClass::scanNetworks()
{
	TIM_START;

	uint8_t numAp;
	if(
		!writeCommand(CMD_WIFILIST)						||
		!readCommandResult(SPI_SOCKET_NETSCAN_TIMEOUT)	||
		!read8(numAp)
	)
	{
		return 0;
	}

	TIM_END;
	return numAp;
}

// get number of networks got on last scan without re-scanning
// (or re-scan if none found)
uint8_t FishinoClass::getNumNetworks()
{
	TIM_START;

	uint8_t numAp;
	if(
		!writeCommand(CMD_WIFIGETNUMNETWORKS)			||
		!readCommandResult(SPI_SOCKET_NETSCAN_TIMEOUT)	||
		!read8(numAp)
	)
	{
		return 0;
	}

	TIM_END;
	return numAp;
}

// Return the SSID discovered during the network scan.
// param networkItem: specify from which network item want to get the information
// return: ssid string of the specified item on the networks scanned list
String FishinoClass::SSID(uint8_t networkItem)
{
	char *ssid;
	if(
		!writeCommand(CMD_WIFIGETSSID)	||
		!write8(networkItem)			||
		!readCommandResult()			||
		!readString(ssid)
	)
		return "";
	String ssidStr = ssid;
	free(ssid);
	return ssidStr;
}

// return the BSSID of a scanned network
// buf must point to 6 byte buffer able to hold the SSID
uint8_t *FishinoClass::BSSID(uint8_t networkItem, uint8_t *buf)
{
	if(
		!writeCommand(CMD_WIFIGETBSSID)	||
		!write8(networkItem)			||
		!readCommandResult()			||
		!readBuf(buf, 6)
	)
		memset(buf, 0, 6);
	return buf;
}


// Return the encryption type of the networks discovered during the scanNetworks
// param networkItem: specify from which network item want to get the information
// return: encryption type (enum wl_enc_type) of the specified item on the networks scanned list
uint8_t FishinoClass::encryptionType(uint8_t networkItem)
{
	uint8_t encType = 0;
	if(
		!writeCommand(CMD_WIFIGETAUTHMODE)	||
		!write8(networkItem)				||
		!readCommandResult()				||
		!read8(encType)
	)
		return 0;
	return encType;
}

// Return the RSSI of the networks discovered during the scanNetworks
// param networkItem: specify from which network item want to get the information
// return: signed value of RSSI of the specified item on the networks scanned list
int32_t FishinoClass::RSSI(uint8_t networkItem)
{
	int8_t rssi = 0;
	if(
		!writeCommand(CMD_WIFIGETRSSI)	||
		!write8(networkItem)			||
		!readCommandResult()			||
		!read8((uint8_t &)rssi)
	)
		return 0;
	return rssi;
}

// Return Connection status.
// return: one of the value defined in JOIN_STATUS
uint8_t FishinoClass::status()
{
	uint8_t res;
	
	if(
		!writeCommand(CMD_WIFIJOINSTATUS)	||
		!readCommandResult()				||
		!read8(res)
	)
		return WL_IDLE_STATUS;
	return res;
}

// get/set host name for this device
bool FishinoClass::setHostName(const char *hostName)
{
	if(
		!writeCommand(CMD_WIFISETHOSTNAME)	||
		!writeString(hostName)				||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::setHostName(const __FlashStringHelper *hostName)
{
	if(
		!writeCommand(CMD_WIFISETHOSTNAME)	||
		!writeString(hostName)				||
		!readCommandResult()
	)
		return false;
	return true;
}

String FishinoClass::getHostName(void)
{
	char *hostName = NULL;
	if(
		!writeCommand(CMD_WIFIGETHOSTNAME)	||
		!readCommandResult()				||
		!readString(hostName)
	)
		return "";
	String res = hostName;
	free(hostName);
	return res;
}

// Resolve the given hostname to an IP address.
// param aHostname: Name to be resolved
// param aResult: IPAddress structure to store the returned IP address
// result: 1 if aIPAddrString was successfully converted to an IP address,
// else error code
bool FishinoClass::hostByName(const char* aHostname, IPAddress& aResult)
{
	// STILL NOT IMPLEMENTED
	return false;
}

// low level TCP stuffs -- used by client and server

// connect to host -- return socket number, or 0xff if error
// we use an high timeout here, because esp connect has over 6 seconds
uint8_t FishinoClass::connect(const char *host, uint16_t port, bool ssl)
{
	TIM_START;
	uint8_t res;
	
	if(
		!writeCommand(CMD_TCPCONNECT)				||
		!write8(1)									||
		!writeString(host)							||
		!write16(port)								||
		!write8(ssl)								||
		!readCommandResult(SPI_SOCKET_CONN_TIMEOUT)	||
		!read8(res)
	)
		return 0xff;
	DEBUG_INFO("SOCKET : %u\n", res);
	TIM_END;
	return res;
}

// connect to host -- return socket number, or 0xff if error
uint8_t FishinoClass::connect(IPAddress const &ip, uint16_t port, bool ssl)
{
	TIM_START;
	uint8_t res;
	
	if(
		!writeCommand(CMD_TCPCONNECT)				||
		!write8(0)									||
		!writeIP(ip)								||
		!write16(port)								||
		!write8(ssl)								||
		!readCommandResult(SPI_SOCKET_CONN_TIMEOUT)	||
		!read8(res)
	)
		res = 0xff;
	DEBUG_INFO("SOCKET : %u\n", res);
	TIM_END;
	return res;
}

// disconnect from host -- return true on success, false otherwise
bool FishinoClass::disconnect(uint8_t sock)
{
	DEBUG_INFO("SOCKET : %u\n", sock);
	if(
		!writeCommand(CMD_TCPCLOSE)	||
		!write8(sock)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// write to socket -- return true on success, false otherwise
bool FishinoClass::write(uint8_t sock, uint8_t const *buf, uint16_t bufLen)
{
	TIM_START;
	DEBUG_INFO("SOCKET : %u -- bufLen : %u\n", sock, bufLen);
	if(
		!writeCommand(CMD_TCPSEND)					||
		!write8(sock)								||
		!write32(bufLen)							||
		!writeBuf(buf, bufLen)						||
		!readCommandResult(SPI_SOCKET_RW_TIMEOUT)
	)
		return false;

	TIM_END_SIZE(bufLen);
	return true;
}

bool FishinoClass::write(uint8_t sock, const __FlashStringHelper *str)
{
	TIM_START;
	
	DEBUG_INFO("SOCKET : %u -- strlen : %" PRIu32 "\n", sock, (uint32_t)strlen_P((const char *)str));
	if(
		!writeCommand(CMD_TCPSEND)				||
		!write8(sock)							||
		!writeString(str)						||
		!readCommandResult(SPI_SOCKET_RW_TIMEOUT)
	)
		return false;

	TIM_END_SIZE(strlen_P((const char *)str));
	return true;
}

// read from socket -- return true on success, false otherwise
// buffer MUST be big enough to contain reqBytes
// on exit, bufLen contains number of bytes actually read
bool FishinoClass::read(uint8_t sock, uint16_t reqBytes, uint8_t *buf, uint16_t &bufLen)
{
	TIM_START;
	
	uint32_t bufLen32;
	DEBUG_INFO("SOCKET : %u -- reqBytes : %u\n", sock, reqBytes);
	if(
		!writeCommand(CMD_TCPREAD)					||
		!write8(sock)								||
		!write32(reqBytes)							||
		!readCommandResult(SPI_SOCKET_RW_TIMEOUT)	||
		!read32(bufLen32)							||
		!readBuf(buf, bufLen32)
	)
	{
		bufLen = 0;
		DEBUG_ERROR("ERROR RECEIVING\n");
		return false;
	}
	bufLen = (uint16_t)bufLen32;
	DEBUG_INFO("GOT %u BYTES\n", bufLen);

	TIM_END_SIZE(bufLen);
	return true;
}

// return number of available bytes on socket
uint16_t FishinoClass::available(uint8_t sock)
{
	TIM_START;
	
	uint32_t size;
	if(
		!writeCommand(CMD_TCPAVAIL)	||
		!write8(sock)					||
		!readCommandResult()			||
		!read32(size)
	)
		size = 0;
	if(size)
	{
		DEBUG_INFO("%lu BYTES AVAIL\n", size);
	}

	TIM_END;
	return (uint16_t)size;
}

// peek a data byte from socket, if any
// returns true on success, false otherwise
bool FishinoClass::peek(uint8_t sock, uint8_t &b)
{
	TIM_START;
	
	uint32_t dummy;
	DEBUG_INFO("SOCKET : %u\n", sock);
	if(
		!writeCommand(CMD_TCPPEEK)					||
		!write8(sock)								||
		!write32(1)									||
		!readCommandResult(SPI_SOCKET_RW_TIMEOUT)	||
		!read32(dummy)								||
		dummy != 1									||
		!readBuf(&b, 1)
	)
		return false;

	TIM_END_SIZE(1);
	return true;
}

// flush socket data
bool FishinoClass::flush(uint8_t sock)
{
	TIM_START;
	
	DEBUG_INFO("SOCKET : %u\n", sock);
	if(
		!writeCommand(CMD_TCPFLUSH)	||
		!write8(sock)					||
		!readCommandResult()
	)
		return false;

	TIM_END;
	return true;
}

// check if socket is connected or has data
bool FishinoClass::connected(uint8_t sock)
{
	DEBUG_INFO("SOCKET : %u\n", sock);
	uint8_t res;

	if(
		!writeCommand(CMD_TCPCONNECTED)		||
		!write8(sock)						||
		!readCommandResult()				||
		!read8(res)
	)
		return false;

	return res;
}

// connection status - true if connected, false if disconnected or invalid
bool FishinoClass::status(uint8_t sock)
{
	TIM_START;
	DEBUG_INFO("SOCKET : %u\n", sock);
	uint8_t res;

	if(
		!writeCommand(CMD_TCPCONNSTATUS)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read8(res)
	)
		return false;

	TIM_END;
	return res;
}

// set buffered mode for tcp client
bool FishinoClass::setBufferedMode(uint8_t sock, bool b)
{
	if(
		!writeCommand(CMD_TCPSETBUFFERED)	||
		!write8(sock)						||
		!write8(b)							||
		!readCommandResult()
	)
		return false;
	return true;
}

// get buffered mode for tcp client
bool FishinoClass::getBufferedMode(uint8_t sock)
{
	uint8_t res;
	if(
		!writeCommand(CMD_TCPGETBUFFERED)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read8(res)
	)
		return false;
	return res;
}

// disable/enable nagle for tcp client
bool FishinoClass::setNoDelay(uint8_t sock, bool b)
{
	if(
		!writeCommand(CMD_TCPSETNODELAY)	||
		!write8(sock)						||
		!write8(b)							||
		!readCommandResult()
	)
		return false;
	return true;
}

// get nagle status for tcp client
bool FishinoClass::getNoDelay(uint8_t sock)
{
	uint8_t res;

	if(
		!writeCommand(CMD_TCPGETNODELAY)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read8(res)
	)
		return false;
	return res;
}

// set inactivity timeout for tcp client
bool FishinoClass::setClientForceCloseTime(uint8_t sock, uint32_t tim)
{
	if(
		!writeCommand(CMD_TCPSETTIMEOUT)	||
		!write8(sock)						||
		!write32(tim)						||
		!readCommandResult()
	)
		return false;
	return true;
}

// get inactivity timeout for tcp client
uint32_t FishinoClass::getClientForceCloseTime(uint8_t sock)
{
	uint32_t res;
	if(
		!writeCommand(CMD_TCPGETTIMEOUT)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read32(res)
	)
		return 0;
	return res;
}

// set max number of tcp connections
bool FishinoClass::setMaxTcpConnections(uint8_t n)
{
	if(
		!writeCommand(CMD_TCPSETMAXCONN)	||
		!write8(n)							||
		!readCommandResult()
	)
		return false;
	return true;
}

// get max number of tcp connections
uint8_t FishinoClass::getMaxTcpConnections(void)
{
	uint8_t res;
	if(
		!writeCommand(CMD_TCPGETMAXCONN)	||
		!readCommandResult()				||
		!read8(res)
	)
		return false;
	return res;
}

// start a server on given port
// return socket number, or 0xff if error
uint8_t FishinoClass::startServer(uint16_t port, uint16_t timeout)
{
	uint8_t sock;
	if(
		!writeCommand(CMD_TCPSERVERSTART)	||
		!write16(port)						||
		!write16(timeout)					||
		!readCommandResult()				||
		!read8(sock)
	)
	{
		DEBUG_ERROR("Failed to start server\n");
		return 0xff;
	}
	
	DEBUG_INFO("Server started with socket %" PRIu16 "\n", sock);
	return sock;
}

// stops server 
bool FishinoClass::stopServer(uint8_t sock)
{
	if(
		!writeCommand(CMD_TCPSERVERSTOP)	||
		!write8(sock)						||
		!readCommandResult()
	)
	{
		DEBUG_ERROR("Failed to stop server %" PRIu16 "\n", sock);
		return false;
	}
	DEBUG_INFO("Server %" PRIu16 " stopped\n", sock);
	return true;
}

// get server status
uint8_t FishinoClass::serverStatus(uint8_t sock)
{
	uint8_t res;

	if(
		!writeCommand(CMD_TCPSERVERSTATUS)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read8(res)
	)
	{
		DEBUG_ERROR("Failed to get status for server %" PRIu16 "\n", sock);
		return CLOSED;
	}
	DEBUG_INFO("Server %" PRIu16 " status is %" PRIu8 "\n", sock, res);
	return res;
}

// check if server has clients
bool FishinoClass::serverHasClient(uint8_t sock)
{
	uint8_t res;

	if(
		!writeCommand(CMD_TCPSERVERHASCLIENT)	||
		!write8(sock)							||
		!readCommandResult()					||
		!read8(res)
	)
	{
		DEBUG_ERROR("Failed to check if server %" PRIu16 " has clients\n", sock);
		return false;
	}
//	DEBUG_INFO("Server %" PRIu16 " %s clients\n", sock, res ? "has" : "has not");
	return res;
}

// get first available client, if any     
uint8_t FishinoClass::serverAvail(uint8_t sock)
{
	uint8_t clientSock;
	if(
		!writeCommand(CMD_TCPSERVERAVAIL)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read8(clientSock)
	)
	{
		DEBUG_ERROR("Failed to get clients for server %" PRIu16 "\n", sock);
		return 0xff;
	}
//	DEBUG_INFO("Got client %" PRIu16 " for server %" PRIu16 "\n", clientSock, sock);
	return clientSock;
}

// enable/disable nagle      
bool FishinoClass::serverSetNoDelay(uint8_t sock, bool b)
{
	if(
		!writeCommand(CMD_TCPSERVERSETNODELAY)	||
		!write8(sock)							||
		!write8(b)								||
		!readCommandResult()
	)
		return false;
	return true;
}

// query nagle state
bool FishinoClass::serverGetNoDelay(uint8_t sock)
{
	uint8_t res;

	if(
		!writeCommand(CMD_TCPSERVERGETNODELAY)		||
		!write8(sock)								||
		!readCommandResult()						||
		!read8(res)
	)
		return false;
	return res;
}

// write data to all connected clients
bool FishinoClass::serverWrite(uint8_t sock, uint8_t const *buf, uint16_t len, uint16_t &written)
{
	uint32_t written32;
	if(
		!writeCommand(CMD_TCPSERVERWRITE)	||
		!write8(sock)						||
		!write32(len)						||
		!writeBuf(buf, len)					||
		!readCommandResult()				||
		!read32(written32)
	)
		return false;
	written = (uint16_t)written32;
	return true;
}

// set server buffered mode
bool FishinoClass::serverSetBufferedMode(uint8_t sock, bool b)
{
	if(
		!writeCommand(CMD_TCPSERVERSETBUFFERED)	||
		!write8(sock)							||
		!write8(b)								||
		!readCommandResult()
	)
		return false;
	return true;
}

// get server buffered mode
bool FishinoClass::serverGetBufferedMode(uint8_t sock)
{
	uint8_t res;
	if(
		!writeCommand(CMD_TCPSERVERGETBUFFERED)	||
		!write8(sock)							||
		!readCommandResult()					||
		!read8(res)
	)
		return false;
	return res;
}

// set server clients timeout
bool FishinoClass::serverSetClientsForceCloseTime(uint8_t sock, uint32_t tim)
{
	if(
		!writeCommand(CMD_TCPSERVERSETTIMEOUT)	||
		!write8(sock)							||
		!write32(tim)							||
		!readCommandResult()
	)
		return false;
	return true;
}

// get server clients timeout
uint32_t FishinoClass::serverGetClientsForceCloseTime(uint8_t sock)
{
	uint32_t res;
	if(
		!writeCommand(CMD_TCPSERVERGETTIMEOUT)	||
		!write8(sock)							||
		!readCommandResult()					||
		!read32(res)
	)
		return 0;
	return res;
}

// set max number of server clients
bool FishinoClass::serverSetMaxClients(uint8_t sock, uint8_t n)
{
	if(
		!writeCommand(CMD_TCPSERVERSETMAXCONN)	||
		!write8(sock)							||
		!write8(n)								||
		!readCommandResult()
	)
		return false;
	return true;
}

// get max number of server clients
uint8_t FishinoClass::serverGetMaxClients(uint8_t sock)
{
	uint8_t res;
	if(
		!writeCommand(CMD_TCPSERVERGETMAXCONN)	||
		!write8(sock)							||
		!readCommandResult()					||
		!read8(res)
	)
		return false;
	return res;
}

// "uniform" functions to set station and AP parameters
// station ones replicates the above ones, jus to have meaningful names
bool FishinoClass::setStaIP(IPAddress ip)
{
	return config(ip);
}

bool FishinoClass::setStaMAC(uint8_t const *mac)
{
	if(
		!writeCommand(CMD_WIFISETSTAMAC)	||
		!writeBuf(mac, 6)					||
		!readCommandResult()
	)
		return false;
	return true;
}

// issue an IP-Setting command
bool FishinoClass::setIP(uint8_t ipCmd, IPAddress const &ip)
{
	if(
		!writeCommand(ipCmd)		||
		!writeIP(ip)				||
		!readCommandResult()
	)
		return false;
	return true;
}


bool FishinoClass::setStaGateway(IPAddress gw)
{
	return setIP(CMD_WIFISETSTAGW, gw);
}

bool FishinoClass::setStaNetMask(IPAddress nm)
{
	return setIP(CMD_WIFISETSTANM, nm);
}

bool FishinoClass::staStartDHCP(void)
{
	if(
		!writeCommand(CMD_WIFIENABLESTADHCP)	||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::staStopDHCP(void)
{
	if(
		!writeCommand(CMD_WIFIDISABLESTADHCP)	||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::getStaDHCPStatus(void)
{
	uint8_t res;
	
	if(
		!writeCommand(CMD_WIFIQUERYSTADHCP)	||
		!readCommandResult()					||
		!read8(res)
	)
		return false;
	return res;
}

bool FishinoClass::setApIP(IPAddress ip)
{
	return setIP(CMD_WIFISETAPIP, ip);
}

bool FishinoClass::setApMAC(uint8_t const *mac)
{
	if(
		!writeCommand(CMD_WIFISETAPMAC)	||
		!writeBuf(mac, 6)				||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::setApGateway(IPAddress gw)
{
	return setIP(CMD_WIFISETAPGW, gw);
}

bool FishinoClass::setApNetMask(IPAddress nm)
{
	return setIP(CMD_WIFISETAPNM, nm);
}

// full AP IP info
bool FishinoClass::setApIPInfo(IPAddress ip, IPAddress gateway, IPAddress netmask)
{
	if(
		!writeCommand(CMD_WIFISETAPIPINFO)	||
		!writeIP(ip)							||
		!writeIP(gateway)						||
		!writeIP(netmask)						||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::getApIPInfo(IPAddress &ip, IPAddress &gateway, IPAddress &netmask)
{
	if(
		!writeCommand(CMD_WIFIGETAPIPINFO)	||
		!readCommandResult()						||
		!readIP(ip)						||
		!readIP(gateway)					||
		!readIP(netmask)
	)
		return false;
	return true;
}

// softAp DHCP
bool FishinoClass::softApStartDHCPServer(void)
{
	if(
		!writeCommand(CMD_WIFIENABLEAPDHCP)	||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::softApStartDHCPServer(IPAddress startIP, IPAddress endIP)
{
	if(
		!writeCommand(CMD_WIFISETAPDHCPRANGE)	||
		!writeIP(startIP)						||
		!writeIP(endIP)						||
		!readCommandResult()
	)
		return false;
	return softApStartDHCPServer();
}

bool FishinoClass::softApStopDHCPServer(void)
{
	if(
		!writeCommand(CMD_WIFIDISABLEAPDHCP)	||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::getSoftApDHCPServerStatus(void)
{
	uint8_t res;
	
	if(
		!writeCommand(CMD_WIFIQUERYAPDHCPS)	||
		!readCommandResult()					||
		!read8(res)
	)
		return false;
	return res;
}

// softAp configuration

// get ap SSID - return a dynamic buffer that MUST be freed by free()
// return NULL on error
char *FishinoClass::softApGetSSID(void)
{
	char *SSID, *pass;
	uint8_t channel;
	bool hidden;
	if(!softApGetConfig(SSID, pass, channel, hidden))
		return NULL;
	if(pass)
		DEBUG_FREE(pass);
	return SSID;
}

// get ap PASSWORD - return a dynamic buffer that MUST be freed by free()
// return NULL on error
char *FishinoClass::softApGetPassword(void)
{
	char *SSID, *pass;
	uint8_t channel;
	bool hidden;
	if(!softApGetConfig(SSID, pass, channel, hidden))
		return NULL;
	if(SSID)
		DEBUG_FREE(SSID);
	return pass;
}

// return ap channel
// return 0 on error
uint8_t FishinoClass::softApGetChannel(void)
{
	char *SSID, *pass;
	uint8_t channel;
	bool hidden;
	if(!softApGetConfig(SSID, pass, channel, hidden))
		return 0;
	if(SSID)
		DEBUG_FREE(SSID);
	if(pass)
		DEBUG_FREE(pass);
	return channel;
}

// return ap hidden state
bool FishinoClass::softApGetHidden(void)
{
	char *SSID, *pass;
	uint8_t channel;
	bool hidden;
	if(!softApGetConfig(SSID, pass, channel, hidden))
		return false;
	if(SSID)
		DEBUG_FREE(SSID);
	if(pass)
		DEBUG_FREE(pass);
	return hidden;
}

// get all softAp config data
// warning - all returned strings MUST be freed by free()
bool FishinoClass::softApGetConfig(char *&SSID, char *&pass, uint8_t &channel, bool &hidden)
{
	SSID = NULL;
	pass = NULL;
	uint8_t chan, hid;

	if(
		!writeCommand(CMD_WIFIGETAPCONFIG)	||
		!readCommandResult()					||
		!readString(SSID)						||
		!readString(pass)						||
		!read8(chan)							||
		!read8(hid)
	)
	{
		// cleanup on errors
		if(SSID)
			DEBUG_FREE(SSID);
		if(pass)
			DEBUG_FREE(pass);
		SSID = pass = NULL;
		return false;
	}

	channel = chan;
	hidden = hid;
	return true;
}

// set softAp parameters
bool FishinoClass::softApConfig(const char *SSID, const char *pass, uint8_t channel, bool hidden)
{
	if(
		!writeCommand(CMD_WIFISETAPCONFIG)	||
		!writeString(SSID)					||
		!writeString(pass)					||
		!write8(channel)						||
		!write8(hidden)						||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::softApConfig(const __FlashStringHelper *SSID, const __FlashStringHelper *pass, uint8_t channel, bool hidden)
{
	if(
		!writeCommand(CMD_WIFISETAPCONFIG)	||
		!writeString(SSID)					||
		!writeString(pass)					||
		!write8(channel)						||
		!write8(hidden)						||
		!readCommandResult()
	)
		return false;
	return true;
}

// setup station AP parameters without joining
bool FishinoClass::setStaConfig(const char *ssid)
{
	return setStaConfig(ssid, "");
}

// setup station AP parameters without joining
bool FishinoClass::setStaConfig(const __FlashStringHelper *ssid)
{
	return setStaConfig(ssid, F(""));
}

bool FishinoClass::setStaConfig(const char *ssid, const char *passphrase)
{
	if(
		!writeCommand(CMD_WIFISETSTACONFIG)	||
		!writeString(ssid)					||
		!writeString(passphrase)				||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::setStaConfig(const __FlashStringHelper *ssid, const __FlashStringHelper *passphrase)
{
	if(
		!writeCommand(CMD_WIFISETSTACONFIG)	||
		!writeString(ssid)					||
		!writeString(passphrase)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// get current station AP parameters
// WARNING : returns dynamic buffers wich MUST be freed
bool FishinoClass::getStaConfig(char *&ssid, char *&pass)
{
	ssid = NULL;
	pass = NULL;

	if(
		!writeCommand(CMD_WIFIGETSTACONFIG)	||
		!readCommandResult()					||
		!readString(ssid)						||
		!readString(pass)
	)
	{
		// cleanup
		if(ssid)
			DEBUG_FREE(ssid);
		if(pass)
			DEBUG_FREE(pass);
		return false;
	}
	return true;
}

// join AP setup by setStaConfig call (or previous AP set by begin)
bool FishinoClass::joinAp(void)
{
	if(
		!writeCommand(CMD_WIFISTACONNECT)		||
		!readCommandResult(SPI_JOIN_TIMEOUT)
	)
		return false;

	return true;
}

// quits AP
bool FishinoClass::quitAp(void)
{
	if(
		!writeCommand(CMD_WIFISTADISCONNECT)	||
		!readCommandResult()
	)
		return false;
	return true;
}

#ifndef _FISHINO_PIC32_
////////////////////////////////////////////////////////////////////////
// extra I/O pins on ESP module

// pinMode for ESP I/O
bool FishinoClass::pinMode(uint8_t pin, uint8_t mode)
{
	if(
		!writeCommand(CMD_PORTSPINMODE)	||
		!write8(pin)						||
		!write8(mode)						||
		!readCommandResult()
	)
		return false;
	return true;
}

// digital I/O for ESP I/O
uint8_t FishinoClass::digitalRead(uint8_t pin)
{
	uint8_t res;
	
	if(
		!writeCommand(CMD_PORTSDIGITALREAD)	||
		!write8(pin)							||
		!readCommandResult()						||
		!read8(res)
	)
		return false;
	return res;
}

bool FishinoClass::digitalWrite(uint8_t pin, uint8_t val)
{
	if(
		!writeCommand(CMD_PORTSDIGITALWRITE)	||
		!write8(pin)							||
		!write8(val)							||
		!readCommandResult()
	)
		return false;
	return true;
}

// analog read
uint16_t FishinoClass::analogRead(void)
{
	uint16_t val;
	
	if(
		!writeCommand(CMD_PORTSANALOGREAD)	||
		!readCommandResult()						||
		!read16(val)
	)
		return 0;
	return val;
}
#endif

// physical mode handling
uint8_t FishinoClass::getPhyMode(void)
{
	uint8_t mode ;
	
	if(
		!writeCommand(CMD_WIFIGETPHYMODE)	||
		!readCommandResult()						||
		!read8(mode)
	)
		return PHY_MODE_UNKNOWN;
	return mode;
}

bool FishinoClass::setPhyMode(uint8_t mode)
{
	if(
		!writeCommand(CMD_WIFISETPHYMODE)	||
		!write8(mode)							||
		!readCommandResult()
	)
		return false;
	return true;
}

// UDP STUFFS

// binds UDP socket to IP/Port
// returns udp socket number
uint8_t FishinoClass::udpBegin(uint16_t localPort)
{
	uint8_t socket;
	
	if(
		!writeCommand(CMD_UDPBEGIN)	||
		!write16(localPort)			||
		!readCommandResult()			||
		!read8(socket)
	)
		return 0xff;
	return socket;
}

// ends UDP socked binding
bool FishinoClass::udpEnd(uint8_t sock)
{
	if(
		!writeCommand(CMD_UDPEND)	||
		!write8(sock)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// starts building a packet for sending to a receiver
bool FishinoClass::udpBeginPacket(uint8_t sock, const char *host, uint16_t localPort)
{
	if(
		!writeCommand(CMD_UDPBEGINPACKET)	||
		!write8(sock)						||
		!write8(1)							||
		!writeString(host)					||
		!write16(localPort)					||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::udpBeginPacket(uint8_t sock, IPAddress const &hostIP, uint16_t localPort)
{
	if(
		!writeCommand(CMD_UDPBEGINPACKET)	||
		!write8(sock)						||
		!write8(0)							||
		!writeIP(hostIP)					||
		!write16(localPort)					||
		!readCommandResult()
	)
		return false;
	return true;
}

#ifndef _FISHINO_PIC32_
bool FishinoClass::udpBeginPacket(uint8_t sock, const __FlashStringHelper *host, uint16_t localPort)
{
	if(
		!writeCommand(CMD_UDPBEGINPACKET)	||
		!write8(sock)						||
		!write8(1)							||
		!writeString(host)					||
		!write16(localPort)					||
		!readCommandResult()
	)
		return false;
	return true;
}
#endif

// ends and send current packet
bool FishinoClass::FishinoClass::udpEndPacket(uint8_t sock)
{
	if(
		!writeCommand(CMD_UDPENDPACKET)	||
		!write8(sock)					||
		!readCommandResult()
	)
		return false;
	return true;
}

// parse currently received UDP packet and returns number of available bytes
uint16_t FishinoClass::FishinoClass::udpParsePacket(uint8_t sock)
{
	uint16_t len;
	
	if(
		!writeCommand(CMD_UDPPARSEPACKET)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read16(len)
	)
		return 0;
	return len;
}

// write data to current packet
bool FishinoClass::udpWrite(uint8_t sock, const uint8_t *buf, uint16_t len, uint16_t &written)
{
	if(
		!writeCommand(CMD_UDPWRITE)	||
		!write8(sock)				||
		!write16(len)				||
		!writeBuf(buf, len)			||
		!readCommandResult()		||
		!read16(written)
	)
		return false;
	return true;
}

// check if data is available in current packet
uint16_t FishinoClass::udpAvail(uint8_t sock)
{
	uint16_t len;
	
	if(
		!writeCommand(CMD_UDPAVAIL)	||
		!write8(sock)				||
		!readCommandResult()		||
		!read16(len)
	)
		return 0;
	return len;
}

// read data from current packet
// buffer MUST be big enough to contain reqBytes
// on exit, bufLen contains number of bytes actually read
bool FishinoClass::udpRead(uint8_t sock, uint16_t reqBytes, uint8_t *buf, uint16_t &bufLen)
{
	if(
		!writeCommand(CMD_UDPREAD)	||
		!write8(sock)				||
		!write16(reqBytes)			||
		!readCommandResult()		||
		!read16(bufLen)				||
		!readBuf(buf, bufLen)
	)
	{
		bufLen = 0;
		return false;
	}
	return true;
}

// peek next byte in current packet
bool FishinoClass::udpPeek(uint8_t sock, uint16_t reqBytes, uint8_t *buf, uint16_t &bufLen)
{
	if(
		!writeCommand(CMD_UDPPEEK)	||
		!write8(sock)				||
		!write16(reqBytes)			||
		!readCommandResult()		||
		!read16(bufLen)				||
		!readBuf(buf, bufLen)
	)
	{
		bufLen = 0;
		return false;
	}
	return true;
}

// Finish reading the current packet
bool FishinoClass::udpFlush(uint8_t sock)
{
	if(
		!writeCommand(CMD_UDPFLUSH)	||
		!write8(sock)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// flush all stored UDP packets
bool FishinoClass::udpFlushAll(uint8_t sock)
{
	if(
		!writeCommand(CMD_UDPFLUSHALL)	||
		!write8(sock)					||
		!readCommandResult()
	)
		return false;
	return true;
}

// Return the IP address of the host who sent the current incoming packet
bool FishinoClass::udpRemoteIP(uint8_t sock, IPAddress &ip)
{
	if(
		!writeCommand(CMD_UDPREMOTEIP)	||
		!write8(sock)					||
		!readCommandResult()			||
		!readIP(ip)
	)
		return false;
	return true;
}

// Return the port of the host who sent the current incoming packet
bool FishinoClass::udpRemotePort(uint8_t sock, uint16_t &port)
{
	if(
		!writeCommand(CMD_UDPREMOTEPORT)	||
		!write8(sock)						||
		!readCommandResult()				||
		!read16(port)
	)
	{
		port = 0;
		return false;
	}
	return true;
}

// serial port handling

// port open
bool FishinoClass::serialBegin(uint32_t speed)
{
	if(
		!writeCommand(CMD_PORTSSERIALBEGIN)	||
		!write32(speed)						||
		!readCommandResult()
	)
		return false;
	return true;
}

// port close
bool FishinoClass::serialEnd(void)
{
	if(
		!writeCommand(CMD_PORTSSERIALEND)	||
		!readCommandResult()
	)
		return false;
	return true;
}

// data available on port
uint16_t FishinoClass::serialAvail(void)
{
	uint16_t count;
	if(
		!writeCommand(CMD_PORTSSERIALAVAIL)	||
		!readCommandResult()				||
		!read16(count)
	)
		return 0;
	return count;
}

int FishinoClass::serialPeek(void)
{
	uint16_t res;
	if(
		!writeCommand(CMD_PORTSSERIALPEEK)	||
		!readCommandResult()						||
		!read16(res)
	)
		return -1;
	return (int)res;
}

// read a data buffer -- buffer MUST be pre-allocated
uint16_t FishinoClass::serialRead(uint8_t *buf, uint16_t reqSize)
{
	uint16_t size;
	if(
		!writeCommand(CMD_PORTSSERIALREAD)	||
		!write16(reqSize)					||
		!readCommandResult()				||
		!read16(size)						||
		!readBuf(buf, size)
	)
		return 0;
	return size;
}

// write a data buffer to serial port
uint16_t FishinoClass::serialWrite(uint8_t *buf, uint16_t size)
{
	uint16_t recSize;
	if(
		!writeCommand(CMD_PORTSSERIALWRITE)	||
		!write16(size)						||
		!writeBuf(buf, size)				||
		!readCommandResult()				||
		!read16(recSize)
	)
		return 0;
	return recSize;
}

void FishinoClass::serialFlush(void)
{
	if(
		!writeCommand(CMD_PORTSSERIALFLUSH)	||
		!readCommandResult()
	)
		return ;
}

// sets NTP server
bool FishinoClass::ntpSetServer(IPAddress const &ip)
{
	if(
		!writeCommand(CMD_NTPSETSERVER)	||
		!write8(0)						||
		!writeIP(ip)						||
		!readCommandResult()
	)
		return false;
	return true;
}

bool FishinoClass::ntpSetServer(const char *server)
{
	if(
		!writeCommand(CMD_NTPSETSERVER)	||
		!write8(1)						||
		!writeString(server)				||
		!readCommandResult()
	)
		return false;
	return true;
}

// get NTP server
bool FishinoClass::ntpGetServer(IPAddress &ip)
{
	if(
		!writeCommand(CMD_NTPGETSERVER)	||
		!readCommandResult()					||
		!readIP(ip)
	)
		return false;
	return true;
}

// get Epoch (seconds since 1 january 1900
uint32_t FishinoClass::ntpEpoch(void)
{
	uint32_t epoch;
	if(
		!writeCommand(CMD_NTPQUERYEPOCH)	||
		!readCommandResult(3000)				||
		!read32(epoch)
	)
		return 0;
	return epoch;
}

// get current time (hour, minute and second)
bool FishinoClass::ntpTime(uint8_t &hour, uint8_t &minute, uint8_t &second)
{
	if(
		!writeCommand(CMD_NTPQUERYTIME)	||
		!readCommandResult(3000)				||
		!read8(hour)					||
		!read8(minute)				||
		!read8(second)
	)
		return false;
	return true;
}

// put ESP module in deep sleep mode for requested time (uSeconds)
bool FishinoClass::deepSleep(uint32_t uSec)
{
	if(
		!writeCommand(CMD_SYSDEEPSLEEP)	||
		!write32(uSec)					||
		!readCommandResult(3000)
	)
		return false;
	return true;
}

// TEST
// read and write a buffer to slave
// returns a 32 bit word with upper part as write time, lower as read time
uint32_t FishinoClass::test(void)
{
	uint32_t res = 0;
	
	uint8_t sendBuf[TEST_BUF_SIZE], recvBuf[TEST_BUF_SIZE];

	// fill send buffer with random data
	for(int i = 0; i < TEST_BUF_SIZE; i++)
		sendBuf[i] = random(255);
	
	// send command
//	DEBUG_INFO("Writing command\n");
	if(!writeCommand(2))
	{
		DEBUG_ERROR("writeCommand() ERROR\n");
		return 0;
	}
	
	// send buffer size
//	DEBUG_INFO("Writing size\n");
	if(!write32(TEST_BUF_SIZE))
	{
		DEBUG_ERROR("write32() ERROR\n");
		return 0;
	}
	
	uint64_t tim = micros();

	// send buffer
//	DEBUG_INFO("Writing buffer\n");
	if(!writeBuf(sendBuf, TEST_BUF_SIZE))
	{
		DEBUG_ERROR("write() ERROR\n");
		return 0;
	}
	
	// get elapsed time
	tim = micros() - tim;
	res = (tim << 16) & 0xffff0000;
	
	// send command completion
	if(!readCommandResult())
	{
		DEBUG_ERROR("Command execution error\n");
		return 0;
	}
	
	memset(recvBuf, 0, sizeof(recvBuf));
	
	// reverse source buffer to check with dest
	for(int i = 0; i < TEST_BUF_SIZE; i++)
		sendBuf[i] ^= 0xff;

	// give some time to slave to elaborate the buffer
	delay(10);

	tim = micros();

	// receive buffer
//	DEBUG_INFO("Reading buffer\n");
	if(!readBuf(recvBuf, TEST_BUF_SIZE))
	{
		DEBUG_ERROR("read() ERROR\n");
		return 0;
	}
	
	tim = micros() - tim;
	res |= tim & 0xffff;
	
	// check buffer content
//	DEBUG_INFO("Checking buffer\n");
	for(int i = 0; i < TEST_BUF_SIZE; i++)
		if(recvBuf[i] != sendBuf[i])
		{
			DEBUG_ERROR("data ERROR ad byte %d\n", i);
			int j = i - 2;
			if(j < 0)
				j = 0;
			if(j + 16 > TEST_BUF_SIZE)
				j = TEST_BUF_SIZE - 16;
				
			DEBUG_BINDUMP(sendBuf + j, 16);
			DEBUG_PRINT("\n");
			DEBUG_BINDUMP(recvBuf + j, 16);
			DEBUG_PRINT("\n");
			return 0;
		}
	return res;
}

FishinoClass &__fishino()
{
	static FishinoClass fish;
	return fish;
}

// get available free ram
// for backward compatibility
uint32_t FishinoClass::freeRam(void)
{
	return __debug__freeram__();
}

