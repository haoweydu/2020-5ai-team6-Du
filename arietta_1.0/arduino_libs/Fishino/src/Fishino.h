//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								Fishino.h											//
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

/////////////////////////////////////////////////////////////////////////////////////
// NOTE :
// ALL driver functions returns a boolean to show that command has been
// accepted. If return value is TRUE, the command was OK and more data is following
// If return value is FALSE, no more data is following
// This was changed since 2.0.0 to simplify error handling
// This library is *NOT* compatible with firmware prior to 5.1.0
// Next versions will be backwards compatible with previous firmware version
// *ONLY* if major version number is the same
/////////////////////////////////////////////////////////////////////////////////////

#ifndef FISHINO_H
#define FISHINO_H

#include <Arduino.h>
#include <FishinoFlash.h>

#include "FishinoSPIMaster.h"

#define TEST_BUF_SIZE	500

///////////////////////////////////////////////////////////////////////////////////
// this is for local testing purposes
// it allows to setup wifi network access without having to put
// it inside code. Just put a MYNET.h file somewhere and adjust the following
// define with its path.
// on which you define MY_SSID, MY_PASSWORD and other private stuffs
// and ALL Fishino samples and demos will have the network automatically setup
// without need of cleaning code from sensible data before deploying it
///////////////////////////////////////////////////////////////////////////////////
//#define MYNET_H	"/home/massimo/sketchbook/MYNET.h"
#ifdef MYNET_H
	#include MYNET_H
#endif

// uncomment this line to see detailed error messages on serial port
// (you must also enable them with Fishino.showErrors(true)
//#define __FISHINO_ERRORS_DETAILED__

// macro to build 32 bit version number from components
#define VERSION(maj, min, dev) ( ((uint32_t)maj << 16) | ((uint32_t)(min & 0xFF)) << 8 | ((uint32_t)(dev & 0xFF)) )

// minimal firmware version allowed for this library
#define FISHINO_FIRMWARE_VERSION_MIN VERSION(7,0,0)

#define Fishino __fishino()

#define WL_MAC_ADDR_LENGTH 6

enum WIFI_MODE
{
	NULL_MODE,
	STATION_MODE,
	SOFTAP_MODE,
	STATIONAP_MODE
};

enum JOIN_STATUS {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    STATION_IDLE		= 0,	// for compatibility with previous library version
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    STATION_GOT_IP		= 3,	// for compatibility with previous library version
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
};

enum PHY_MODE
{
	PHY_MODE_UNKNOWN	= 0,
	PHY_MODE_11B		= 1,
	PHY_MODE_11G		= 2,
	PHY_MODE_11N		= 3
};

enum AUTH_MODE
{
    AUTH_OPEN           = 0,
    AUTH_WEP,
    AUTH_WPA_PSK,
    AUTH_WPA2_PSK,
    AUTH_WPA_WPA2_PSK,
    AUTH_MAX
};

enum wl_tcp_state {
	CLOSED      = 0,
	LISTEN      = 1,
	SYN_SENT    = 2,
	SYN_RCVD    = 3,
	ESTABLISHED = 4,
	FIN_WAIT_1  = 5,
	FIN_WAIT_2  = 6,
	CLOSE_WAIT  = 7,
	CLOSING     = 8,
	LAST_ACK    = 9,
	TIME_WAIT   = 10
};

class FishinoClass
{
		friend class FishinoClient;
		friend class FishinoSecureClient;
		friend class FishinoServer;
		friend class FishinoUDP;
		friend class FishinoSerialClass;

	private:
	
		// last command sent to ESP
		// for debugging purposes
		uint8_t _lastCommand;
	
		// firmware version
		uint32_t _fwVersion;
		
		// current BSSID
		uint8_t _BSSID[6];
		
		// low level TCP stuffs -- used by client and server
		
		// connect to host -- return socket number, or 0xff if error
		uint8_t connect(const char *host, uint16_t port, bool ssl);
		uint8_t connect(IPAddress const &ip, uint16_t port, bool ssl);

		// disconnect from host -- return true on success, false otherwise
		bool disconnect(uint8_t sock);
		
		// write to socket -- return true on success, false otherwise
		bool write(uint8_t sock, uint8_t const *buf, uint16_t bufLen);
		bool write(uint8_t sock, const __FlashStringHelper *str);
		
		// read from socket -- return true on success, false otherwise
		// buffer MUST be big enough to contain reqBytes
		// on exit, bufLen contains number of bytes actually read
		bool read(uint8_t sock, uint16_t reqBytes, uint8_t *buf, uint16_t &bufLen);

		// return number of available bytes on socket
		uint16_t available(uint8_t sock);

		// peek a data byte from socket, if any
		// returns true on success, false otherwise
		bool peek(uint8_t sock, uint8_t &b);
		
		// flush socket data
		bool flush(uint8_t sock);
		
		// check if socket is connected or has data
		bool connected(uint8_t sock);
		
		// connection status - true if connected, false if disconnected or invalid
		bool status(uint8_t sock);
		
		// set buffered mode for tcp client
		bool setBufferedMode(uint8_t sock, bool b);
		
		// get buffered mode for tcp client
		bool getBufferedMode(uint8_t sock);
		
		// disable/enable nagle for tcp client
		bool setNoDelay(uint8_t sock, bool b);
		
		// get nagle status for tcp client
		bool getNoDelay(uint8_t sock);
		
		// set inactivity timeout for tcp client
		bool setClientForceCloseTime(uint8_t sock, uint32_t tim);

		// get inactivity timeout for tcp client
		uint32_t getClientForceCloseTime(uint8_t sock);

		// verify secure socket domain fingerprint
		bool verifyFingerPrint(uint8_t sock, const char *domain, const char *fingerPrint);
		bool verifyFingerPrint(uint8_t sock, const __FlashStringHelper *domain, const __FlashStringHelper *fingerPrint);
		
		// set client certificate
		bool setClientCertificate(const uint8_t *buf, uint16_t len);
		bool setClientCertificate();
		
		// det client private key
		bool setClientPrivateKey(const uint8_t *buf, uint16_t len);
		bool setClientPrivateKey();

		// start a server on given port
		// return socket number, or 0xff if error
		uint8_t startServer(uint16_t port, uint16_t timeout);
		
		// stops server 
		bool stopServer(uint8_t sock);
		
		// get server status
		uint8_t serverStatus(uint8_t sock);
		
		// check if server has clients
		bool serverHasClient(uint8_t sock);
		
		// get first available client, if any  
		// return a client socket   
		uint8_t serverAvail(uint8_t sock);
		
		// enable/disable nagle      
		bool serverSetNoDelay(uint8_t sock, bool b);
		
		// query nagle state
		bool serverGetNoDelay(uint8_t sock);

		// write data to all connected clients
		bool serverWrite(uint8_t sock, uint8_t const *buf, uint16_t len, uint16_t &written);

		// set server buffered mode
		bool serverSetBufferedMode(uint8_t sock, bool b);
		
		// get server buffered mode
		bool serverGetBufferedMode(uint8_t sock);
		
		// set server clients timeout
		bool serverSetClientsForceCloseTime(uint8_t sock, uint32_t tim);
		
		// get server clients timeout
		uint32_t serverGetClientsForceCloseTime(uint8_t sock);
		
		// set max number of server clients
		bool serverSetMaxClients(uint8_t sock, uint8_t n);
		
		// get max number of server clients
		uint8_t serverGetMaxClients(uint8_t sock);

		// binds UDP socket to IP/Port
		// returns udp socket number
		uint8_t udpBegin(uint16_t localPort);
		
		// ends UDP socked binding
		bool udpEnd(uint8_t sock);
		
		// starts building a packet for sending to a receiver
		bool udpBeginPacket(uint8_t sock, const char *host, uint16_t remotePort);
		bool udpBeginPacket(uint8_t sock, const __FlashStringHelper *host, uint16_t remotePort);
		bool udpBeginPacket(uint8_t sock, const IPAddress &ip, uint16_t remotePort);
		
		// ends and send current packet
		bool udpEndPacket(uint8_t sock);
		
		// parse currently received UDP packet and returns number of available bytes
		uint16_t udpParsePacket(uint8_t sock);
		
		// write data to current packet
		bool udpWrite(uint8_t sock, const uint8_t *buf, uint16_t len, uint16_t &written);
		
		// check if data is available in current packet
		uint16_t udpAvail(uint8_t sock);
		
		// read data from current packet
		// buffer MUST be big enough to contain reqBytes
		// on exit, bufLen contains number of bytes actually read
		bool udpRead(uint8_t sock, uint16_t reqBytes, uint8_t *buf, uint16_t &bufLen);
		
		// peek next byte in current packet
		bool udpPeek(uint8_t sock, uint16_t reqBytes, uint8_t *buf, uint16_t &bufLen);
		
		// Finish reading the current packet
		bool udpFlush(uint8_t sock);
		
		// flush all stored UDP packets
		bool udpFlushAll(uint8_t sock);
		
		// Return the IP address of the host who sent the current incoming packet
		// PARAMETERS:
		//		socket		byte
		// RESULT:
		//		remote IP	IP
		bool udpRemoteIP(uint8_t sock, IPAddress &ip);
		
		// Return the port of the host who sent the current incoming packet
		// PARAMETERS:
		//		socket		byte
		// RESULT:
		//		remote port	uint32
		bool udpRemotePort(uint8_t sock, uint16_t &port);
		
		// issue an IP-Querying command
		// return: Ip address value
		IPAddress getIP(uint8_t ipCmd);
		
		// issue an IP-Setting command
		bool setIP(uint8_t ipCmd, IPAddress const &ip);

		// serial port handling
		bool serialBegin(uint32_t speed);
		bool serialEnd(void);
		uint16_t serialAvail(void);
		int serialPeek(void);
		uint16_t serialRead(uint8_t *buf, uint16_t reqSize);
		uint16_t serialWrite(uint8_t *buf, uint16_t size);
		void serialFlush(void);
		
	public:

		// constructor
		FishinoClass();
		
		// destructor
		~FishinoClass();
		
		// get last command
		uint8_t getLastCommand(void) { return _lastCommand; }
		void setLastCommand(uint8_t cmd) { _lastCommand = cmd; }
		
		// reset ESP and wait for it to be ready
		// return true if ok, false if not ready
		bool reset(void);
		
		// Get firmware version
		uint32_t firmwareVersion();
		char *firmwareVersionStr();
		
		// error handling; useful mostly on debugging code
		// forward the request to ESP8266 module
		
		// get last driver error code
		uint16_t getLastError(void);
		
		// get last driver error string
		// MUST be freed by free() upon usage
		const char *getLastErrorString(void);
		
		// get string for error code
		// MUST be freed by free() upon usage
		const char *getErrorString(uint16_t errCode);
		
		// clear last error
		void clearLastError(void);
		
		// set operation mode (one of WIFI_MODE)
		bool setMode(uint8_t mode);
		
		// get operation mode
		uint8_t getMode(void);

		// Start Wifi connection for OPEN networks
		// param ssid: Pointer to the SSID string.
		// return true on success
		uint8_t begin(const char *ssid);
		uint8_t begin(const __FlashStringHelper *ssid);
		
		// Start Wifi connection with encryption.
		// return true on success
		uint8_t begin(const char* ssid, const char *passphrase);
		uint8_t begin(const __FlashStringHelper *ssid, const __FlashStringHelper *passphrase);

		// setup station AP parameters without joining
		bool setStaConfig(const char *ssid);
		bool setStaConfig(const __FlashStringHelper *ssid);
		bool setStaConfig(const char *ssid, const char *passphrase);
		bool setStaConfig(const __FlashStringHelper *ssid, const __FlashStringHelper *passphrase);
		
		// get current station AP parameters
		// WARNING : returns dynamic buffers wich MUST be freed
		bool getStaConfig(char *&ssid, char *&pass);
		
		// join AP setup by setStaConfig call (or previous AP set by begin)
		bool joinAp(void);
		
		// quits AP
		bool quitAp(void);

		// Change Ip configuration settings disabling the dhcp client
		//	param local_ip
		// WARNING : you should also set gateway and netmask (see next function)
		// if you set only the IP, the system will default with :
		// IP      : a, b, c, d
		// gateway : a, b, c, 1
		// netmask : 255, 255, 255, 0
		// (which is OK for most but not all situations)
		bool config(IPAddress local_ip);

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
		bool config(IPAddress local_ip, IPAddress gateway);

		// Change Ip configuration settings disabling the dhcp client
		// param local_ip:			Static ip configuration
		// param gateway :			Static gateway configuration
		// param subnet:			Static Subnet mask
		bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet);

		// Change Ip configuration settings disabling the dhcp client
		// param local_ip:			Static ip configuration
		// param gateway:			Static gateway configuration
		// param subnet:			Static Subnet mask
		// param dns_server:		IP configuration for DNS server 1
		// DNS SERVER SETTING NOT SUPPORTED BY NOW -- DUMMY
		bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns_server);

		// Change DNS Ip configuration
		// param dns_server1:		ip configuration for DNS server 1
		// DNS SERVER SETTING NOT SUPPORTED BY NOW -- DUMMY
		bool setDNS(IPAddress dns_server1);

		// Change DNS Ip configuration
		// param dns_server1:		ip configuration for DNS server 1
		// param dns_server2:		ip configuration for DNS server 2
		// DNS SERVER SETTING NOT SUPPORTED BY NOW -- DUMMY
		bool setDNS(IPAddress dns_server1, IPAddress dns_server2);

		// Disconnect from the network
		// return: one value of wl_status_t enum
		bool disconnect(void);

		// Get the interface MAC address.
		// return: pointer to A STATIC uint8_t array with length WL_MAC_ADDR_LENGTH
		const uint8_t* macAddress(void);

		// Get the interface IP address.
		// return: Ip address value
		IPAddress localIP();

		// Get the interface subnet mask address.
		// return: subnet mask address value
		IPAddress subnetMask();

		// Get the gateway ip address.
		// return: gateway ip address value
		IPAddress gatewayIP();

		// Return the current SSID associated with the network
		// return: ssid string
		String SSID();

		// Return the current BSSID associated with the network.
		// It is the MAC address of the Access Point
		// return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
		const uint8_t* BSSID();

		// Return the current RSSI /Received Signal Strength in dBm)
		// associated with the network
		// return: signed value
		int32_t RSSI();

		// Return the Encryption Type associated with the network
		// return: one value of wl_enc_type enum
		uint8_t	encryptionType();

		// Start scan WiFi networks available
		// return: Number of discovered networks
		uint8_t scanNetworks();
		
		// get number of networks got on last scan without re-scanning
		// (or re-scan if none found)
		uint8_t getNumNetworks();

		// Return the SSID discovered during the network scan.
		// param networkItem: specify from which network item want to get the information
		// return: ssid string of the specified item on the networks scanned list
		String SSID(uint8_t networkItem);
		
		// return the BSSID of a scanned network
		// buf must point to 6 byte buffer able to hold the SSID
		uint8_t *BSSID(uint8_t networkItem, uint8_t *buf);

		// Return the encryption type of the networks discovered during the scanNetworks
		// param networkItem: specify from which network item want to get the information
		// return: encryption type (enum wl_enc_type) of the specified item on the networks scanned list
		uint8_t encryptionType(uint8_t networkItem);

		// Return the RSSI of the networks discovered during the scanNetworks
		// param networkItem: specify from which network item want to get the information
		// return: signed value of RSSI of the specified item on the networks scanned list
		int32_t RSSI(uint8_t networkItem);

		// Return Connection status.
		// return: true if connected to AP, false otherwise
		// if connected, refresh cached AP SSID
		uint8_t status();
		
		// get/set host name for this device
		bool setHostName(const char *hostName);
		bool setHostName(const __FlashStringHelper *hostName);
		String getHostName(void);
		
		// "uniform" functions to set station and AP parameters
		// station ones replicates the above ones, jus to have meaningful names
		
		bool setStaIP(IPAddress ip);
		bool setStaMAC(uint8_t const *mac);
		bool setStaGateway(IPAddress gw);
		bool setStaNetMask(IPAddress nm);
		
		// station DHCP client
		bool staStartDHCP(void);
		bool staStopDHCP(void);
		bool getStaDHCPStatus(void);

		bool setApIP(IPAddress ip);
		bool setApMAC(uint8_t const *mac);
		bool setApGateway(IPAddress gw);
		bool setApNetMask(IPAddress nm);

		bool setApIPInfo(IPAddress ip, IPAddress gateway, IPAddress netmask);
		bool getApIPInfo(IPAddress &ip, IPAddress &gateway, IPAddress &netmask);

		// Resolve the given hostname to an IP address.
		// param aHostname: Name to be resolved
		// param aResult: IPAddress structure to store the returned IP address
		// result: 1 if aIPAddrString was successfully converted to an IP address,
		// else error code
		bool hostByName(const char* aHostname, IPAddress& aResult);

		// softAp DHCP
		bool softApStartDHCPServer(void);
		bool softApStartDHCPServer(IPAddress startIP, IPAddress endIP);
		bool softApStopDHCPServer(void);
		bool getSoftApDHCPServerStatus(void);

		// softAp configuration
		
		// get ap SSID - return a dynamic buffer that MUST be freed by free()
		// return NULL on error
		char *softApGetSSID(void);
		
		// get ap PASSWORD - return a dynamic buffer that MUST be freed by free()
		// return NULL on error
		char *softApGetPassword(void);
		
		// return ap channel
		// return 0 on error
		uint8_t softApGetChannel(void);
		
		// return ap hidden state
		bool softApGetHidden(void);
		
		// get all softAp config data
		// warning - all returned strings MUST be freed by free()
		bool softApGetConfig(char *&SSID, char *&pass, uint8_t &channel, bool &hidden);
		
		// set softAp parameters
		bool softApConfig(const char *SSID, const char *pass, uint8_t channel, bool hidden = false);
		bool softApConfig(const __FlashStringHelper *SSID, const __FlashStringHelper *passphrase, uint8_t channel, bool hidden = false);

		// set max number of tcp connections
		bool setMaxTcpConnections(uint8_t n);
		
		// get max number of tcp connections
		uint8_t getMaxTcpConnections(void);
		
		// extra I/O pins on ESP module

		// pinMode for ESP I/O
		bool pinMode(uint8_t pin, uint8_t mode);

		// digital I/O for ESP I/O
#ifndef _FISHINO_PIC32_
		uint8_t digitalRead(uint8_t pin);
		bool digitalWrite(uint8_t pin, uint8_t val);
		
		// analog read
		uint16_t analogRead(void);
#endif
		
		// physical mode handling
		uint8_t getPhyMode(void);
		bool setPhyMode(uint8_t mode);
		
		// ntp stuffs
		
		// sets NTP server
		bool ntpSetServer(IPAddress const &ip);
		bool ntpSetServer(const char *server);
		
		// get NTP server
		bool ntpGetServer(IPAddress &ip);
		
		// get Epoch (seconds since 1 january 1900
		uint32_t ntpEpoch(void);
		
		// get current time (hour, minute and second)
		bool ntpTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
		
		// put ESP module in deep sleep mode for requested time (uSeconds)
		bool deepSleep(uint32_t uSec);
		
		// TEST
		uint32_t test(void);
		
		// get available free ram
		static uint32_t freeRam(void);
};

FishinoClass &__fishino();

#include "FishinoClient.h"
#include "FishinoServer.h"
#include "FishinoUdp.h"
#include "FishinoSerial.h"

#endif
