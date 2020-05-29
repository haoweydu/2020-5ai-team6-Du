//////////////////////////////////////////////////////////////////////////////////////
//																					//
//							FishinoHomeAuto											//
//					An home automation demo with Fishino							//
//					Created by Massimo Del Fedele, 2016								//
//																					//
//  Copyright (c) 2016 and 2017 Massimo Del Fedele.  All rights reserved.			//
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
//	VERSION 1.0.0 - INITIAL VERSION - BASED ON TinyWebServer						//
//	VERSION 2.0.0 - 18/12/2016 - COMPLETE REWRITE FOR RAM OPTIMIZATION				//
//  Version 6.0.0 -- June 2017 - USE NEW DEBUG LIBRARY								//
//	VERSION 7.0.0 - 18/12/2017 - DISCARD 'GET' PART OF URL							//
//																					//
//////////////////////////////////////////////////////////////////////////////////////
#include <FishinoWebServer.h>
#include "FishinoHomeAuto.h"
#include <SD.h>

//#define DEBUG_MEMORY_ALLOC
//#define DEBUG_LEVEL_INFO
#include <FishinoDebug.h>
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CONFIGURATION DATA		-- ADAPT TO YOUR NETWORK !!!
// DATI DI CONFIGURAZIONE	-- ADATTARE ALLA PROPRIA RETE WiFi !!!
#ifndef __MY_NETWORK_H

// OPERATION MODE :
// NORMAL (STATION)	-- NEEDS AN ACCESS POINT/ROUTER
// STANDALONE (AP)	-- BUILDS THE WIFI INFRASTRUCTURE ON FISHINO
// COMMENT OR UNCOMMENT FOLLOWING #define DEPENDING ON MODE YOU WANT
// MODO DI OPERAZIONE :
// NORMAL (STATION)	-- HA BISOGNO DI UNA RETE WIFI ESISTENTE A CUI CONNETTERSI
// STANDALONE (AP)	-- REALIZZA UNA RETE WIFI SUL FISHINO
// COMMENTARE O DE-COMMENTARE LA #define SEGUENTE A SECONDA DELLA MODALITÀ RICHIESTA
// #define STANDALONE_MODE

// here pur SSID of your network
// inserire qui lo SSID della rete WiFi
#define MY_SSID	""

// here put PASSWORD of your network. Use "" if none
// inserire qui la PASSWORD della rete WiFi -- Usare "" se la rete non ￨ protetta
#define MY_PASS	""

// here put required IP address of your Fishino
// comment out this line if you want AUTO IP (dhcp)
// NOTES :
//		for STATION_MODE if you use auto IP you must find it somehow !
//		for AP_MODE you MUST choose a free address range
// 
// here put required IP address (and maybe gateway and netmask!) of your Fishino
// comment out this lines if you want AUTO IP (dhcp)
// NOTE : if you use auto IP you must find it somehow !
// inserire qui l'IP desiderato ed eventualmente gateway e netmask per il fishino
// commentare le linee sotto se si vuole l'IP automatico
// nota : se si utilizza l'IP automatico, occorre un metodo per trovarlo !
#define IPADDR	192, 168,   1, 251
#define GATEWAY	192, 168,   1, 1
#define NETMASK	255, 255, 255, 0

#endif
// 
// NOTE : for prototype green version owners, set SD_CS to 3 !!!
// NOTA : per i possessori del prototipo verde di Fishino, impostare SD_CS a 3 !!!
#ifdef SDCS
const int SD_CS = SDCS;
#else
const int SD_CS = 4;
#endif
//
// END OF CONFIGURATION DATA
// FINE DATI DI CONFIGURAZIONE
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

// define ip address if required
// NOTE : if your network is not of type 255.255.255.0 or your gateway is not xx.xx.xx.1
// you should set also both netmask and gateway
#ifdef IPADDR
	IPAddress ip(IPADDR);
	#ifdef GATEWAY
		IPAddress gw(GATEWAY);
	#else
		IPAddress gw(ip[0], ip[1], ip[2], 1);
	#endif
	#ifdef NETMASK
		IPAddress nm(NETMASK);
	#else
		IPAddress nm(255, 255, 255, 0);
	#endif
#endif

// the web server object
FishinoWebServer web(80);

// sd card stuffs
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

uint8_t nAppliances = 0;
Appliance *appliances;

// read next non-blank char from file
// return 0 on EOF
char readNB(void)
{
	int c;
	do
	{
		c = file.read();
		if(c == -1 || !c)
			return 0;
	}
	while(isspace(c));
	return c;
}

// read an appliance from current point in map.txt file
// (use global sdfile variable)
bool readAppliance(Appliance *app)
{
	// max 15 chars for keys and values
	char buf[16];
	char c;
	uint8_t iBuf;
	
	// find id's opening quote
	while( (c = readNB()) != 0 && c != '"')
		;
	if(!c)
		return false;
	
	// get id
	iBuf = 0;
	while( (c = readNB()) != 0 && c != '"' && iBuf < 16)
		buf[iBuf++] = c;
	if(iBuf > 16 || !c)
		return false;
	
	// store id
	char *id = (char *)DEBUG_MALLOC(iBuf + 1);
	strncpy(id, buf, iBuf);
	id[iBuf] = 0;
	
	uint8_t kind = 0xff;
	uint8_t io = 0xff;
	
	// get kind and io, in any order
	while(kind == 0xff || io == 0xff)
	{
		while( (c = readNB()) != 0 && c != '"')
			;
		if(!c)
			break;
		c = readNB();
		if(!c)
			break;
		if(c == 'k')
		{
			if(
				((c = readNB()) == 'i') &&
				((c = readNB()) == 'n') &&
				((c = readNB()) == 'd') &&
				((c = readNB()) == '"')
			)
			{
				// found 'kind' keyword, get the value
				while( (c = readNB()) != 0 && c != '"')
					;
				if(!c)
					break;
				c = readNB();
				if(c == 'd')
				{
					c = readNB();
					if(readNB() != '"')
						break;
					if(c == 'o')
						kind = 0x00;
					else if(c == 'i')
						kind = 0x01;
					else
						break;
				}
				else if(c == 'a')
				{
					c = readNB();
					if(readNB() != '"')
						break;
					if(c == 'o')
						kind = 0x02;
					else if(c == 'i')
						kind = 0x03;
					else
						break;
				}
				else
					break;
			}
		}
		else if(c == 'i')
		{
			if(readNB() == 'o' && readNB() == '"')
			{
				while( (c = readNB()) != 0 && c != '"')
					;
				if(!c)
					break;
				io = 0;
				while(isDigit(c = readNB()))
					io = 10 * io + c - '0';
				if(c != '"')
				{
					io = 0xff;
					break;
				}
			}
		}
	}

	// here we shall have kind and io setup
	// if not, free id and return false
	if(kind == 0xff || io == 0xff)
	{
		Serial << F("ERROR IN maps.txt\n");
		free(id);
		return false;
	}
	app->id = id;
	app->kind = kind;
	app->io = io;

	// skip to closing bracket
	while( (c = readNB()) != 0 && c != '}')
		;
	return true;
}

// read appliances from map.txt file
// (use global sdfile variable)
bool readAppliances(void)
{
	const char *fname = "MAP.TXT";
	char c;
	
	// open map file
	if(!file.open(&root, fname, O_READ))
		return false;
	
	// count number of appliances, needed to allocate data
	// (we're tight of space...)
	// simply counts opening brackets, no error check
	nAppliances = 0;
	appliances = NULL;
	while( (c = readNB()) != 0)
		if(c == '{')
			nAppliances++;
	// we should have the opening bracket to skip
	if(nAppliances)
		nAppliances--;
	else
	{
		file.close();
		return false;
	}

	// allocate data for appliances
	appliances = (Appliance *)DEBUG_MALLOC(nAppliances * sizeof(Appliance));
	
	// restart from beginning
	file.seekSet(0);
	
	// skip first opening bracket
	while( (c = readNB()) != 0 && c != '{')
		;
	Appliance *appliance = appliances;
	for(uint8_t i = 0; i < nAppliances; i++)
	{
		if(!readAppliance(appliance++))
		{
			file.close();
			nAppliances = 0;
			free(appliances);
			appliances = NULL;
			return false;
		}
	}
	file.close();
	
	Serial << F("Found ") << nAppliances << F(" appliances\n");
	Appliance *app = appliances;
	for(uint8_t i = 0; i < nAppliances; i++, app++)
		Serial << app->id << ":" << app->kind << ":" << app->io << "\n";

	return true;
}

// based on appliances, setup I/O pins
void setupIO(void)
{
	Appliance *app = appliances;
	for(uint8_t i = 0; i < nAppliances; i++, app++)
	{
		switch(app->kind)
		{
			// digital out
			case 0x00:
				pinMode(app->io, OUTPUT);
				digitalWrite(app->io, 0);
				break;

			// digital in
			case 0x01:
				pinMode(app->io, INPUT);
				break;

			// analog out
			case 0x02:
				pinMode(app->io, OUTPUT);
				analogWrite(app->io, 0);
				break;

			// analog in
			case 0x03:
				break;
		}
	}
}

// sends a file to client
void sendFile(FishinoWebServer& web, const char* filename)
{
	Serial << F("Free RAM: ") << Fishino.freeRam() << "\r\n";
	Serial << F("File: ") << filename << "\n";

	if (!filename)
	{
		web.sendErrorCode(404);
		web << F("Could not parse URL");
	}
	else
	{
		FishinoWebServer::MimeType mimeType = web.getMimeTypeFromFilename(filename);
		web.sendErrorCode(200);
		web.sendContentType(mimeType);
		
		// ask to cache image types
		if(
			mimeType == FishinoWebServer::MIMETYPE_GIF ||
			mimeType == FishinoWebServer::MIMETYPE_JPG ||
			mimeType == FishinoWebServer::MIMETYPE_PNG ||
			mimeType == FishinoWebServer::MIMETYPE_ICO
		)
			web << F("Cache-Control:public, max-age=900\r\n");
		web.endHeaders();
		if (file.open(&root, filename, O_READ))
		{
			Serial << F("Read file ");
			Serial.println(filename);
			web.sendFile(file);
			file.close();
		}
		else
		{
			web << F("Could not find file: ") << filename << "\n";
		}
	}
}

// handle file requests
bool fileHandler(FishinoWebServer& web)
{
	String filename = web.getFileFromPath(web.getPath());
	sendFile(web, filename.c_str());
	return true;
}


// read client and find out appliance pointer and data from ig
Appliance *getApplianceData(FishinoClient &client, uint16_t &data)
{
	char buf[16];
	uint8_t i = 0;
	int c;
	while(client.available())
	{
		if(i >= 16)
			return NULL;
		c = client.read();
		if(c == -1)
			return NULL;
		if(c != '=')
			buf[i++] = c;
		else
		{
			buf[i] = 0;
			break;
		}
	}
	data = 0;
	while(client.available())
	{
		c = client.read();
		if(c == -1)
			return NULL;
		if(!isdigit(c))
			break;
		data = data * 10 + c - '0';
	}
	
	for(uint8_t i = 0; i < nAppliances; i++)
		if(!strcmp(appliances[i].id, buf))
			return &appliances[i];
	return NULL;
}

// handle toggle digital requests
bool toggleDigitalHandler(FishinoWebServer& web)
{
	web.sendErrorCode(200);
	web.sendContentType(F("text/plain"));
	web.endHeaders();
	
	FishinoClient& client = web.getClient();
	uint16_t data;
	Appliance *app = getApplianceData(client, data);
	if(app)
	{
		Serial << F("TOGGLED APPLIANCE ") << app->id << "\n";
		digitalWrite(app->io, data ? 1 : 0);
		return true;
	}
//	return false;
	return true;
}

// handle status requests
bool statusHandler(FishinoWebServer& web)
{
	web.sendErrorCode(200);
	web.sendContentType(F("application/json"));
	web.endHeaders();

	FishinoClient& client = web.getClient();

	// send opening json bracket
	client << '{';
	for(uint8_t i = 0; i < nAppliances; i++)
	{
		Appliance &app = appliances[i];
		
		client << '"' << app.id << "\":";
		switch(app.kind)
		{
			case 0x00:
			case 0x01:
				client << '"' << digitalRead(app.io) << '"';
				break;
			default:
				client << '"' << analogRead(app.io) << '"';
				break;
		}
		if(i < nAppliances - 1)
		{
			client << ',';
		}
	}
	client << "}\n";
	return true;
}

// handle index.htm requests
bool indexHandler(FishinoWebServer& web)
{
	sendFile(web, "INDEX.HTM");
	return true;
}

// handle file uploads
bool fileUploaderHandler(FishinoWebServer& web, uint8_t action, char* buffer, int size)
{
	static uint32_t startTime;
	static uint32_t totalSize;

	switch (action)
	{
		case FishinoWebServer::START:
			startTime = millis();
			totalSize = 0;
			if (!file.isOpen())
			{
				// File is not opened, create it. First obtain the desired name
				// from the request path.
				String fname = web.getFileFromPath(web.getPath());
				if (fname != "")
				{
					Serial << F("Creating ") << fname << "\n";
					file.open(&root, fname.c_str(), O_CREAT | O_WRITE | O_TRUNC);
				}
			}
			break;

		case FishinoWebServer::WRITE:
			if (file.isOpen())
			{
				file.write(buffer, size);
				totalSize += size;
			}
			break;

		case FishinoWebServer::END:
			file.sync();
			Serial << F("Wrote ") << file.fileSize() << F(" bytes in ")
			<< millis() - startTime << F(" millis (received ")
			<< totalSize << F(" bytes)\n");
			file.close();
	}
	
	return true;
}

uint32_t tim;
void setup(void)
{
	Serial.begin(115200);
	while(!Serial)
		;
	Serial << F("Free RAM: ") << Fishino.freeRam() << "\r\n";
	
	// reset and test wifi module
	Serial << F("Resetting Fishino...");
	while(!Fishino.reset())
	{
		Serial << ".";
		delay(500);
	}
	Serial << F("OK\r\n");
	
	// set PHY mode 11G
	Fishino.setPhyMode(PHY_MODE_11G);

	// for AP MODE, setup the AP parameters
#ifdef STANDALONE_MODE
	// setup SOFT AP mode
	// imposta la modalitè SOFTAP
	Serial << F("Setting mode SOFTAP_MODE\r\n");
	Fishino.setMode(SOFTAP_MODE);

	// stop AP DHCP server
	Serial << F("Stopping DHCP server\r\n");
	Fishino.softApStopDHCPServer();
	
	// setup access point parameters
	// imposta i parametri dell'access point
	Serial << F("Setting AP IP info\r\n");
	Fishino.setApIPInfo(ip, ip, IPAddress(255, 255, 255, 0));

	Serial << F("Setting AP WiFi parameters\r\n");
	Fishino.softApConfig(F(MY_SSID), F(MY_PASS), 1, false);
	
	// restart DHCP server
	Serial << F("Starting DHCP server\r\n");
	Fishino.softApStartDHCPServer();
	
	// print current IP address
	Serial << F("IP Address :") << ip << "\r\n";

#else
	// setup STATION mode
	// imposta la modalitè STATION
	Fishino.setMode(STATION_MODE);

	// NOTE : INSERT HERE YOUR WIFI CONNECTION PARAMETERS !!!!!!
	Serial << F("Connecting AP...");
	while(!Fishino.begin(F(MY_SSID), F(MY_PASS)))
	{
		Serial << ".";
		delay(500);
	}
	Serial << F("OK\r\n");

	// setup IP or start DHCP server
#ifdef IPADDR
	Fishino.config(ip, gw, nm);
#else
	Fishino.staStartDHCP();
#endif

	// wait for connection completion
	Serial << F("Waiting for IP...");
	while(Fishino.status() != STATION_GOT_IP)
	{
		Serial << ".";
		delay(500);
	}
	Serial << F("OK\r\n");

	// print current IP address
	Serial << F("IP Address :") << Fishino.localIP() << "\r\n";
	
#endif

	// Set the SDcard CS pin as an output
	pinMode(SD_CS, OUTPUT);

	// Turn off the SD card! (wait for configuration)
	digitalWrite(SD_CS, HIGH);

	// initialize the SD card.
	Serial << F("Setting up SD card...\n");
	// Pass over the speed and Chip select for the SD card
	bool hasFilesystem = false;
	if (!card.init(SPI_FULL_SPEED, SD_CS))
		Serial << F("card failed\n");
	// initialize a FAT volume.
	else if (!volume.init(&card))
		Serial << F("vol.init failed!\n");
	else if (!root.openRoot(&volume))
		Serial << F("openRoot failed\n");
	if(!readAppliances())
		Serial << F("readAppliances failed\n");
	else
		hasFilesystem = true;
	
	// if no filesystem present, just signal it and stop
	if(!hasFilesystem)
	{
		Serial << F("NO FILESYSTEM FOUND -- HALTING\n");
		while(1)
			;
	}

	// setup the I/O ports
	setupIO();
	
	// setup accepted headers and handlers
	web.addHeader(F("Content-Length"));
	web
		.addHandler(F("/")				, FishinoWebServer::GET	, &indexHandler )
		.addHandler(F("/upload/" "*")	, FishinoWebServer::PUT	, &FishinoWebServer::putHandler)
		.addHandler(F("/toggleDigital")	, FishinoWebServer::POST, &toggleDigitalHandler)
		.addHandler(F("/status")		, FishinoWebServer::GET	, &statusHandler)
		.addHandler(F("/" "*")			, FishinoWebServer::GET	, &fileHandler)
	;
	
	web.setPutHandler(fileUploaderHandler);
	
	// setup the PUT handler
//		FishinoWebPutHandler::put_handler_fn = fileUploaderHandler;

	Serial << F("Init done\n");

	// Start the web server.
	Serial << F("Web server starting...\n");
	web.begin();

	Serial << F("Ready to accept HTTP requests.\n");

	tim = millis() + 2000;
}

// ciclo infinito
void loop(void)
{
	if(millis() > tim)
	{
		tim = millis() + 5000;
		Serial << F("Free RAM: ") << Fishino.freeRam() << "\r\n";
	}

	web.process();
}
