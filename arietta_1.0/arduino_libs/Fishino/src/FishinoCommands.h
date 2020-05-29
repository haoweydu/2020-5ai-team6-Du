//////////////////////////////////////////////////////////////////////////////////////
//																					//
//								FishinoCommands.h									//
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
#ifndef __FISHINOCOMMANDS_H
#define __FISHINOCOMMANDS_H

enum Command
{
	CMD_SYSNULL,                // no-op command
	CMD_SYSRESET,               // chip reset
	CMD_SYSTEST,                // test - returns given strings parameters
	CMD_SYSGETFIRMWAREVERSION,  // get firmware version
	CMD_SYSGETERRORSTR,         // get error message for a given code
	CMD_WIFIGETPHYMODE,         // get WiFi physical connection mode
	CMD_WIFISETPHYMODE,         // set WiFi physical connection mode
	CMD_WIFIQUERYMODE,          // query chip mode (AP, STA or AP+STA)
	CMD_WIFISETMODE,            // set chip mode
	CMD_WIFILIST,               // get list of available AP
	CMD_WIFIGETNUMNETWORKS,     // get number of networks from last scan
	CMD_WIFIGETAUTHMODE,        // get auth mode for a wifi station
	CMD_WIFIGETSSID,            // get SSID for a wifi station
	CMD_WIFIGETRSSI,            // get RSSI for a wifi station
	CMD_WIFIGETBSSID,           // get BSSID for a wifi station
	CMD_WIFIGETCHANNEL,         // get channel for a wifi station
	CMD_WIFIJOIN,               // join an access point
	CMD_WIFIQUIT,               // quit access point
	CMD_WIFIJOINSTATUS,         // query status of wifi connection to AP
	CMD_WIFISETSTACONFIG,       // sets station configuration data
	CMD_WIFIGETSTACONFIG,       // get station configuration data
	CMD_WIFISTACONNECT,         // connect station using stored parameters
	CMD_WIFISTADISCONNECT,      // disconnect station (duplicate of CMD_QUITAP)
	CMD_WIFISETSTAIP,           // set station IP
	CMD_WIFIGETSTAIP,           // get station IP
	CMD_WIFISETSTAGW,           // set station gateway
	CMD_WIFIGETSTAGW,           // get station gatway
	CMD_WIFISETSTANM,           // set station Netmask
	CMD_WIFIGETSTANM,           // get station Netmask
	CMD_WIFISETSTADNS,          // set station DNS
	CMD_WIFIGETSTADNS,          // get station DNS
	CMD_WIFISETSTAMAC,          // set station MAC
	CMD_WIFIGETSTAMAC,          // get station MAC
	CMD_WIFISETSTAIPINFO,       // set all station IP info(IP, GW, NM)
	CMD_WIFIGETSTAIPINFO,       // set all station IP info(IP, GW, NM)
	CMD_WIFIENABLESTADHCP,      // enable station DHCP
	CMD_WIFIDISABLESTADHCP,     // disable station DHCP
	CMD_WIFIQUERYSTADHCP,       // queries status of station DHCP client
	CMD_WIFISETAPCONFIG,        // sets ap config (SSID,  PASS, CHANNEL,  HIDDEN)
	CMD_WIFIGETAPCONFIG,        // gets ap config
	CMD_WIFISETAPIP,            // set AP IP
	CMD_WIFIGETAPIP,            // get AP IP
	CMD_WIFISETAPGW,            // set AP gateway
	CMD_WIFIGETAPGW,            // get AP gatway
	CMD_WIFISETAPNM,            // set AP Netmask
	CMD_WIFIGETAPNM,            // get AP Netmask
	CMD_WIFISETAPDNS,           // set AP DNS
	CMD_WIFIGETAPDNS,           // get AP DNS
	CMD_WIFISETAPMAC,           // set AP MAC
	CMD_WIFIGETAPMAC,           // get AP MAC
	CMD_WIFISETAPIPINFO,        // sets full AP IP info (IP, GW, NM)
	CMD_WIFIGETAPIPINFO,        // sets full AP IP info (IP, GW, NM)
	CMD_WIFIENABLEAPDHCP,       // enable AP DHCP
	CMD_WIFIDISABLEAPDHCP,      // disable AP DHCP
	CMD_WIFIQUERYAPDHCPS,       // queries status of AP DHCP server
	CMD_WIFISETAPDHCPRANGE,     // sets AP DHCP range
	CMD_WIFISETHOSTNAME,        // set host name
	CMD_WIFIGETHOSTNAME,        // get host name
	CMD_TCPCONNSTATUS,          // get tcp connection status
	CMD_TCPCONNECT,             // starts a TCP connection
	CMD_TCPCONNECTED,           // check if socket is connected (or has data)
	CMD_TCPCLOSE,               // close connection
	CMD_TCPSEND,                // send data to socket
	CMD_TCPAVAIL,               // get number of available data on a socket
	CMD_TCPREAD,                // read data from socket
	CMD_TCPPEEK,                // peeks a byte of data from socket
	CMD_TCPFLUSH,               // flushes socket data if any
	CMD_TCPSETBUFFERED,         // set buffered mode for tcp client
	CMD_TCPGETBUFFERED,         // get buffered mode for tcp client
	CMD_TCPSETNODELAY,          // disable/enable nagle for tcp client
	CMD_TCPGETNODELAY,          // get nagle status for tcp client
	CMD_TCPSETTIMEOUT,          // set inactivity timeout for tcp client
	CMD_TCPGETTIMEOUT,          // get inactivity timeout for tcp client
	CMD_TCPSETMAXCONN,          // set max number of tcp connections
	CMD_TCPGETMAXCONN,          // get max number of tcp connections
	CMD_TCPVERIFY,                          // verify server certificate fingerprint
	CMD_TCPSETCERTIFICATE,      // set SSL certificate
	CMD_TCPSETPRIVATEKEY,       // set SSL private key
	CMD_TCPSERVERSTART,         // start server
	CMD_TCPSERVERSTOP,          // stop server
	CMD_TCPSERVERSTATUS,        // check tcp server status
	CMD_TCPSERVERHASCLIENT,     // check if server has clients
	CMD_TCPSERVERAVAIL,         // get first available client, if any
	CMD_TCPSERVERWRITE,         // write data to all connected clients
	CMD_TCPSERVERENUMCLIENTS,   // return a list of all connected clients
	CMD_TCPSERVERSETBUFFERED,   // set server buffered mode
	CMD_TCPSERVERGETBUFFERED,   // get server buffered mode
	CMD_TCPSERVERSETNODELAY,    // enable/disable nagle for server
	CMD_TCPSERVERGETNODELAY,    // query nagle state for server
	CMD_TCPSERVERSETTIMEOUT,    // set server clients timeout
	CMD_TCPSERVERGETTIMEOUT,    // get server clients timeout
	CMD_TCPSERVERSETMAXCONN,    // set max number of server clients
	CMD_TCPSERVERGETMAXCONN,    // get max number of server clients
	CMD_UDPBEGIN,               // starts UDP listening on given port
	CMD_UDPEND,                 // ends UDP listening on given socket
	CMD_UDPBEGINPACKET,         // starts an UDP packet
	CMD_UDPENDPACKET,           // ends and send an UDP packet
	CMD_UDPPARSEPACKET,         // parse a received UDP packet
	CMD_UDPWRITE,               // write data to current outgoing UDP packet
	CMD_UDPAVAIL,               // number of avail bytes on current UDP packet
	CMD_UDPREAD,                // read data from current incoming UDP packet
	CMD_UDPPEEK,                // peek a byte from current incoming UDP packet
	CMD_UDPFLUSH,               // flush data on current incoming UDP packet
	CMD_UDPFLUSHALL,            // flush all incoming UDP packets
	CMD_UDPREMOTEIP,            // get remote ip of current incoming UDP packet
	CMD_UDPREMOTEPORT,          // get remote port of current inc. UDP packet
	CMD_PORTSPINMODE,           // pinMode for extra I/O on ESP module
	CMD_PORTSDIGITALREAD,       // digital read for extra I/O on ESP module
	CMD_PORTSDIGITALWRITE,      // digital write for extra I/O on ESP module
	CMD_PORTSANALOGREAD,        // analog read
	CMD_PORTSANALOGWRITE,       // analog write
	CMD_PORTSSERIALBEGIN,       // starts built-in serial port
	CMD_PORTSSERIALEND,         // stops built-in serial port
	CMD_PORTSSERIALAVAIL,       // gets number of available bytes on port
	CMD_PORTSSERIALPEEK,        // reads data from serial port
	CMD_PORTSSERIALREAD,        // reads data from serial port
	CMD_PORTSSERIALWRITE,       // writes data to serial port
	CMD_PORTSSERIALFLUSH,       // flushes data on serial port
	CMD_NTPSETSERVER,           // set NTP server
	CMD_NTPGETSERVER,           // get for NTP server
	CMD_NTPQUERYTIME,           // query time from NTP server
	CMD_NTPQUERYEPOCH,          // equery epoch from NTP server
	CMD_SYSDEEPSLEEP,           // puts WiFi module in deep sleep mode for some time
};

#endif

