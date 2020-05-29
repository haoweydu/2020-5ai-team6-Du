/////////////////////////////////////////////////////////////////
// Repeating Wifi SSL Web Client                               //
//                                                             //
// This sketch connects to a a web server and makes an SSL     //
// request using Fishino                                       //
//                                                             //
// Circuit:                                                    //
//   None                                                      //
//                                                             //
// created on 16 Ago 2015 by Massimo Del Fedele                //
/////////////////////////////////////////////////////////////////
#include <Fishino.h>
#include <SPI.h>

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

// Initialize the Fishino client library
FishinoSecureClient client;

// server address:
const char server[] = "api.github.com";
const int port = 443;

// last time you connected to the server, in milliseconds
// l'ultima volta che vi siete connessi al server, in millisecondi
unsigned long lastConnectionTime = 0;

// delay between updates, in milliseconds
// ritardo tra gli aggiornamenti, in millisecondi
const unsigned long postingInterval = 2L * 1000L;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "CF 05 98 89 CA FF 8E D8 5E 5C E0 C2 E4 F7 E6 C3 C7 50 DD 5C";

void printWifiStatus()
{
	// print the SSID of the network you're attached to:
	// stampa lo SSID della rete:
	Serial.print("SSID: ");
	Serial.println(Fishino.SSID());

	// print your WiFi shield's IP address:
	// stampa l'indirizzo IP della rete:
	IPAddress ip = Fishino.localIP();
	Serial.print("IP Address: ");
	Serial.println(ip);

	// print the received signal strength:
	// stampa la potenza del segnale di rete:
	long rssi = Fishino.RSSI();
	Serial.print("signal strength (RSSI):");
	Serial.print(rssi);
	Serial.println(" dBm");
}

void setup()
{
	// Initialize serial and wait for port to open
	// Inizializza la porta seriale e ne attende l'apertura
	Serial.begin(115200);

	// only for Leonardo needed
	// necessario solo per la Leonardo
	while (!Serial)
		;

	Serial << "Fishino secure client\n";

	// initialize SPI
	// inizializza il modulo SPI
	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2);

	// reset and test WiFi module
	// resetta e testa il modulo WiFi
	Serial << F("Reset WiFi module...");
	Fishino.reset();
	Serial << F("OK\n");

	// go into station mode
	// imposta la modalità stazione
	Fishino.setMode(STATION_MODE);

	// Initialize the Wifi connection.
	// Inizializza la connessione WiFi
	Serial << F("Setting up the Wifi connection\n");

	// try forever to connect to AP
	// tenta la connessione finchè non riesce
	while (!Fishino.begin(MY_SSID, MY_PASS))
		Serial << F("ERROR CONNECTING TO AP, RETRYING....\n");

	// setup IP or start DHCP client
	// imposta l'IP statico oppure avvia il client DHCP
#ifdef IPADDR
	Fishino.config(ip, gw, nm);
#else
	Fishino.staStartDHCP();
#endif

	Serial << F("Waiting for IP..");
	while (Fishino.status() != STATION_GOT_IP)
	{
		delay(500);
		Serial << ".";
	}
	Serial << F("OK\n");

	// print connection status on serial port
	// stampa lo stato della connessione sulla porta seriale
	printWifiStatus();


// Use WiFiClientSecure class to create TLS connection
	FishinoSecureClient client;
	Serial.print("connecting to ");
	Serial.println(server);
	if (!client.connect(server, port))
	{
		Serial.println("connection failed");
		return;
	}

/*
	if (client.verify(fingerprint, server))
	{
		Serial.println("certificate matches");
	}
	else
	{
		Serial.println("certificate doesn't match");
	}
*/

	String url = "/repos/esp8266/Arduino/commits/master/status";
	Serial.print("requesting URL: ");
	Serial.println(url);

	client.print(String("GET ") + url + " HTTP/1.1\r\n" +
				 "Host: " + server + "\r\n" +
				 "User-Agent: BuildFailureDetectorESP8266\r\n" +
				 "Connection: close\r\n\r\n");

	Serial.println("request sent");
	while (client.connected())
	{
		String line = client.readStringUntil('\n');
		Serial.println(line);
		if (line == "\r")
		{
			Serial.println("headers received");
			break;
		}
	}
	uint32_t tim = millis() + 100;
	while(client.connected())
	{
		if(client.available())
		{
			Serial.print((char)client.read());
			tim = millis() + 100;
		}
		else if(millis() > tim)
			break;
	}
	Serial.println("\nclosing connection");
}

void loop()
{
}
