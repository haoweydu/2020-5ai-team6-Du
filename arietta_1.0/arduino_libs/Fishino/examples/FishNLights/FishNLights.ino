// Progetto : FishNLights
#include "Fishino.h"
#include "SPI.h"
#include "EEPROM.h"

// each colibrì device needs 4 PWM outputs
// this array is used to match these outputs with RGBW colors
// ogni colibrì richiede 4 uscite PWM
// questo array è utilizzato per connettere le uscite con i 4 colori
typedef uint8_t DEVICE_CONNECTIONS[4];

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CONFIGURATION DATA		-- ADAPT TO YOUR NETWORK !!!
// DATI DI CONFIGURAZIONE	-- ADATTARE ALLA PROPRIA RETE WiFi !!!
#ifndef __MY_NETWORK_H

// here pur SSID of your network
// inserire qui lo SSID della rete WiFi
#define MY_SSID	""

// here put PASSWORD of your network. Use "" if none
// inserire qui la PASSWORD della rete WiFi -- Usare "" se la rete non ￨ protetta
#define MY_PASS	""

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

// here put the UDP port that listens for packets
// inserire qui la porta UDP in attesa dei pacchetti
#define UDP_PORT		47777

// here put the application name, which will be sent on discovery requests
// inserire qui il nome dell'applicazione che sarà inviato al client dopo la richiesta di discovery
#define APP_NAME		"fishnlights"

// connections between outputs and colibry devices
// each colibrì needs 4 PWM outputs (R, G, B, W)
// as Fishino has only 6 outputs, we simply duplicate them
// to show 2 appliances connected with a single board
// connessioni tra gli output ed i colibrì
// ogni colibrì richiede 4 uscite PWM (R, G, B, W)
// siccome Fishino ha solo 6 PWM disponibili, li abbiamo semplicemente
// duplitati per mostrare nell'App 2 luci connesse
DEVICE_CONNECTIONS DEVICES[] =
{
	{3, 5, 6, 9},
	{3, 5, 6, 9},
};
//                    END OF CONFIGURATION DATA                      //
//                       FINE CONFIGURAZIONE                         //
///////////////////////////////////////////////////////////////////////

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

// number of connected colibrì devices
// numero dei colibrì connessi
const uint8_t numDevices = sizeof(DEVICES) / sizeof(DEVICE_CONNECTIONS);

// stored lights components values
uint8_t lightComponents[numDevices][4];

// max device name length -- warning, MUST be 18 bytes minimum
// lunghezza massima del nome device -- attenzione, DEVE essere di almeno 18 bytes
#define DEVICE_NAME_MAX 20

// position of device name into EEPROM
// posizione del nome device nella EEPROM
const int eepromNameIndex = 0;

// the UDP client/server
// il client/server UDP
FishinoUDP udp;

// udp packet codes
// codici nei pacchetti UDP
typedef enum
{
	UDP_EMPTY		= 0,
	UDP_SETLIGHT	= 1,
	UDP_GETLIGHT	= 2,
	UDP_SETNAME		= 3,
	UDP_GETNAME		= 4,
	UDP_FIND		= 0x55

} UDP_PACKETS_CODES;

// get device name from EEPROM - if invalid, use IP name and device number
// legge il nome del device dalla EEPROM
bool getDeviceName(uint8_t devNum, char *devName, uint16_t maxSize)
{
	if(devNum >= numDevices)
		return NULL;
		
	// get eeprom address for name
	// calcola l'indirizzo per il nome
	int addr = eepromNameIndex + DEVICE_NAME_MAX * devNum;
	
	// read name from eeprom
	// legge il nome dall'eeprom
	char *namePtr = devName;
	for(uint16_t i = 0; i < maxSize; i++)
	{
		uint8_t c = EEPROM.read(addr + i);
		*namePtr++ = c;
		if(!c)
			break;
	}
	*namePtr = 0;
	
	// check the name, we accept only digits, letters, dot, hyphen, underscore and space
	// if an invalid char is found, the name is replaced with local IP, a colon and the device number
	// controlla il nome, vengono accettati solo civre, lettere, punto, trattino ,underscore e spazio
	// se trova un carattere non valido considera il nome nullo e lo rimpiazza con l'IP, un due punti ed il numero di device
	for(uint8_t i = 0; i <= DEVICE_NAME_MAX; i++)
	{
		char c = devName[i];
		if(!c)
			break;
		if(!isalnum(c) && c != '.' && c != '-' && c != '_' && c != ' ')
		{
			devName[0] = 0;
			break;
		}
	}
	if(!devName[0])
	{
		// put IP and device number as light name
		// inserisce il numero IP ed il numero device come nome
		IPAddress ip = Fishino.localIP();
		sprintf(devName, "%d.%d.%d.%d:%02x", ip[0], ip[1], ip[2], ip[3], devNum);
	}
	devName[DEVICE_NAME_MAX] = 0;
	return true;
}

// sets device name into eeprom - if invalid do nothing
bool setDeviceName(uint8_t devNum, char *devName)
{
	// get eeprom address for name
	// calcola l'indirizzo per il nome
	int addr = eepromNameIndex + DEVICE_NAME_MAX * devNum;
	
	// check the name, we accept only digits, letters, dot, hyphen, underscore and space
	// if an invalid char is found, the name is replaced with local IP, a colon and the device number
	// controlla il nome, vengono accettati solo civre, lettere, punto, trattino ,underscore e spazio
	// se trova un carattere non valido considera il nome nullo e lo rimpiazza con l'IP, un due punti ed il numero di device
	char *namePtr = devName;
	char c;
	while( (c = *namePtr++) != 0)
	{
		if(!isalnum(c) && c != '.' && c != '-' && c != '_' && c != ' ')
			return false;
	}
	// if name is valid, write it to EEPROM
	// otherwise do nothing
	if(devName[0])
	{
		namePtr = devName;
		for(uint16_t i = 0; i < DEVICE_NAME_MAX; i++)
		{
			char c = *namePtr++;
			EEPROM.write(addr + i, c);
			if(!c)
				break;
		}
		EEPROM.write(addr + DEVICE_NAME_MAX, 0);
		return true;
	}
	else
		return false;
}

// process SETLIGHT packets
// processa i pacchetti con codice SETLIGHT
bool processSetLight(uint8_t const *packet)
{
	Serial << F("GOT SETLIGHT PACKET\n");
	// packet contains light device number (byte) and 4 bytes with RGBW values
	// il pacchetto contiene il numero del device (byte) e 4 bytes con i valori RGBW
	
	// get light number
	// legge il numero della luce
	uint8_t device = *packet++;
	
	// if out of range, just return
	// se superiore al numero di devices, ritorna
	if(device >= numDevices)
		return false;
		
	// set light components
	// imposta le 4 componenti della luce
	uint8_t *vp = lightComponents[device];
	for(uint8_t i = 0; i < 4; i++)
	{
		*vp++ = *packet;
		analogWrite(DEVICES[device][i], *packet++);
	}
	
	Serial << F("SETLIGHT DONE\n");
	return true;
}

// process GETLIGHT packets
// processa i pacchetti con codice GETLIGHT
bool processGetLight(uint8_t const *packet)
{
	Serial << F("GOT GETLIGHT PACKET\n");
	// packet contains light device number (byte)
	// il pacchetto contiene il numero del device (byte)
	
	// get light number
	// legge il numero della luce
	uint8_t device = *packet++;
	
	// if out of range, just return
	// se superiore al numero di devices, ritorna
	if(device >= numDevices)
		return false;
	
	// response expects the UDP_GETLIGHT packet code, the device number and the 4 components
	// to be sure that packets are in sync
	// il client si aspetta una risposta contenente il codice UDP_GETLIGHT, il numero di device ed i 4 componenti
	// per essere sicuri che la risposta sia in sincronismo con la richiesta
	uint8_t buf[6];
	uint8_t *pBuf = buf;
	uint8_t *vp = lightComponents[device];
	*pBuf++ = UDP_GETLIGHT;
	*pBuf++ = device;
	for(uint8_t i = 0; i < 4; i++)
		*pBuf++ = *vp++;
	
	// send back the packet to sender
	// reinvia il pacchetto al mittente
	udp.beginPacket(udp.remoteIP(), UDP_PORT);
	udp.write(buf, 6);
	Serial << F("GETLIGHT SENDING TO ") << udp.remoteIP() << ":" << UDP_PORT << "\n";
	return udp.endPacket();
}

// process SETNAME packets
// processa i pacchetti con codice SETNAME
bool processSetName(uint8_t const *packet)
{
	// packet contains light device number (byte) and a string with light name
	// il pacchetto contiene il numero del device (byte) ed una stringa con il nome
	
	// get light number
	// legge il numero della luce
	uint8_t device = *packet++;
	
	// if out of range, just return
	// se superiore al numero di devices, ritorna
	if(device >= numDevices)
		return false;
	
	// read name from packet
	// legge il nome dal pacchetto
	char buf[DEVICE_NAME_MAX + 1];
	strncpy(buf, (char *)packet, DEVICE_NAME_MAX);
	buf[DEVICE_NAME_MAX] = 0;
	
	return setDeviceName(device, buf);
}

// process GETNAME packets
// processa i pacchetti con codice GETNAME
bool processGetName(uint8_t const *packet)
{
	// packet contains light device number (byte)
	// il pacchetto contiene il numero del device (byte)
	
	// get light number
	// legge il numero della luce
	uint8_t device = *packet++;
	
	// if out of range, just return
	// se superiore al numero di devices, ritorna
	if(device >= numDevices)
		return false;
	
	char buf[DEVICE_NAME_MAX + 1];
	if(!getDeviceName(device, buf, DEVICE_NAME_MAX))
		return false;
	
	// response packet should contain the UDP_GETNAME code, the device number, and the requested name
	// il pacchetto di risposta deve contenere il codice UDP_GETNAME, il numero di device ed il nome richiesto
	uint16_t packetBufSize = strlen(buf) + 2;
	uint8_t *packetBuf = (uint8_t *)malloc(packetBufSize);
	uint8_t *bufP = packetBuf;
	*bufP++ = UDP_GETNAME;
	*bufP++ = device;
	memcpy(bufP, buf, strlen(buf));

	// send back the packet to sender
	// reinvia il pacchetto al mittente
	udp.beginPacket(udp.remoteIP(), UDP_PORT);
	udp.write(buf, packetBufSize);
	return udp.endPacket();
}

// process FIND packets
// processa i pacchetti con codice FIND
bool processFind(uint8_t const *_packet)
{
	Serial << F("GOT FIND PACKET\n");
	char buf[DEVICE_NAME_MAX + 1];
	
	uint16_t packetLen = 2;
	
	// calculate result packet size
	for(uint8_t i = 0; i < numDevices; i++)
	{
		Serial << F("Reading name of #") << (int)i << F(" device\n");
		if(!getDeviceName(i, buf, DEVICE_NAME_MAX))
			return false;
		Serial << F("GOT :") << buf << "\n";
		packetLen += strlen(buf) + 2;
	}
	Serial << F("PACKET SIZE : ") << packetLen << "\n";
	uint8_t *packet = (uint8_t *)malloc(packetLen);
	uint8_t *packP = packet;
	*packP++ = UDP_FIND;
	*packP++ = numDevices;
	for(uint8_t i = 0; i < numDevices; i++)
	{
		// first the device number
		*packP++ = i;
		
		// then the device name
		getDeviceName(i, buf, DEVICE_NAME_MAX);
		strcpy((char *)packP, buf);
		packP += strlen(buf) + 1;
	}
	
	// send back the packet to sender
	// reinvia il pacchetto al mittente
	Serial << F("Sending packet to ") << udp.remoteIP() << " at port " << UDP_PORT << "\n";
	udp.beginPacket(udp.remoteIP(), UDP_PORT);
	udp.write(packet, packetLen);
	return udp.endPacket();
}

// process UDP packets
// processa i pacchetti UDP
bool processPacket(uint8_t const *packet)
{
	Serial << F("GOT PACKET\n");
	uint8_t type = packet[0];
	packet++;
	switch(type)
	{
		case UDP_SETLIGHT:
			return processSetLight(packet);

		case UDP_GETLIGHT:
			return processGetLight(packet);

		case UDP_SETNAME:
			return processSetName(packet);

		case UDP_GETNAME:
			return processGetName(packet);
			
		case UDP_FIND:
			return processFind(packet);
			
		default:
			Serial << F("UNKNOWN PACKET ") << (int)type << "\n";
			return false;
	}
}

void printWifiStatus()
{
	// print the SSID of the network you're attached to:
	// stampa lo SSID della rete:
	Serial.print("SSID: ");
	Serial.println(Fishino.SSID());

	// print your WiFi shield's IP address:
	// stampa l'indirizzo IP della rete:
	IPAddress ip = Fishino.localIP();
	Serial << F("IP Address: ");
	Serial.println(ip);

	// print the received signal strength:
	// stampa la potenza del segnale di rete:
	long rssi = Fishino.RSSI();
	Serial << F("signal strength (RSSI):");
	Serial.print(rssi);
	Serial << F(" dBm\n");
}

// Codice di inizializzazionecode
void setup(void)
{
	// Initialize serial and wait for port to open
	// Inizializza la porta seriale e ne attende l'apertura
	Serial.begin(115200);
	
	// only for Leonardo needed
	// necessario solo per la Leonardo
	while (!Serial)
		;

	// reset and test WiFi module
	// resetta e testa il modulo WiFi
	while(!Fishino.reset())
		Serial << F("Fishino RESET FAILED, RETRYING...\n");
	Serial << F("Fishino WiFi RESET OK\n");

	// go into station mode
	// imposta la modalità stazione
	Fishino.setMode(STATION_MODE);

	// try forever to connect to AP
	// tenta la connessione finchè non riesce
	Serial << F("Connecting to AP...");
	while(!Fishino.begin(MY_SSID, MY_PASS))
	{
		Serial << ".";
		delay(2000);
	}
	Serial << "OK\n";
	
	// setup IP or start DHCP client
	// imposta l'IP statico oppure avvia il client DHCP
#ifdef IPADDR
	Fishino.config(ip, gw, nm);
#else
	Fishino.staStartDHCP();
#endif

	// wait till connection is established
	Serial << F("Waiting for IP...");
	while(Fishino.status() != STATION_GOT_IP)
	{
		Serial << ".";
		delay(500);
	}
	Serial << "OK\n";

	
	// print connection status on serial port
	// stampa lo stato della connessione sulla porta seriale
	printWifiStatus();
	
	// initialize light values to all zeros
	// inizializza i valori delle luci a zero
	memset(lightComponents, 0, sizeof(lightComponents));
	
	for(uint8_t iDev = 0; iDev < numDevices; iDev++)
	{
		for(uint8_t col = 0; col < 4; col++)
		{
			pinMode(DEVICES[iDev][col], OUTPUT);
			analogWrite(DEVICES[iDev][col], 0);
		}
	}

	// starts listening on local port
	// inizia l'ascolto dei pacchetti UDP alla porta specificata
	Serial << F("Starting connection to server...\n");
	udp.begin(UDP_PORT);
}

// ciclo infinito
void loop(void)
{
	// check incoming packets
	int packetSize = udp.parsePacket();
	if (packetSize)
	{
		uint8_t *buf = (uint8_t *)malloc(packetSize);
		if(udp.read(buf, packetSize))
			processPacket(buf);
		else
			Serial << F("Error reading packet, size is ") << packetSize << F(" bytes\n");
		free(buf);
	}
}
