////////////////////////////////////////////////////////////////////
// FishinoWebButton                                               //
//                                                                //
// A simple web server that shows some buttons on a browser page  //
// allowing the remote control of some digital I/Os.              //
//                                                                //
//	Circuit:                                                      //
//	  3 LEDs with 330 Ohm resistances connected to I/O 3, 5 and 6 //
// created 22 July 2016 by Massimo Del Fedele                     //
////////////////////////////////////////////////////////////////////
#include <Fishino.h>
#include <SPI.h>

///////////////////////////////////////////////////////////////////////
//           CONFIGURATION DATA -- ADAPT TO YOUR NETWORK             //
//     CONFIGURAZIONE SKETCH -- ADATTARE ALLA PROPRIA RETE WiFi      //
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
#define MY_SSID  ""

// here put PASSWORD of your network. Use "" if none
// inserire qui la PASSWORD della rete WiFi -- Usare "" se la rete non ￨ protetta
#define MY_PASS ""

// comment this line if you want a dynamic IP through DHCP
// obtained IP will be printed on serial port monitor
// commentare la linea seguente per avere un IP dinamico tramite DHCP
// l'IP ottenuto verrà visualizzato sul monitor seriale
#define IPADDR		192, 168,   1, 251
#define GATEWAY		192, 168,   1,   1
#define NETMASK		255, 255, 255,   0

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

FishinoServer server(80);

void printWifiStatus()
{
	// print the SSID of the network you're attached to:
	// stampa lo SSDI della rete a cui si è connessi
	Serial.print("SSID: ");
#ifdef STANDALONE_MODE
	Serial.println(MY_SSID);
#else
	Serial.println(Fishino.SSID());
#endif

	// get phy mode and show it
	uint8_t mode = Fishino.getPhyMode();
	Serial.print("PHY MODE: (");
	Serial.print(mode);
	Serial.print(") ");
	switch(mode)
	{
		case PHY_MODE_11B:
			Serial.println("11B");
			break;

		case PHY_MODE_11G:
			Serial.println("11G");
			break;

		case PHY_MODE_11N:
			Serial.println("11N");
			break;
			
		default:
			Serial.println("UNKNOWN");
	}
	
#ifdef STANDALONE_MODE

	// get AP IP info
	IPAddress ip, gw, nm;
	if(Fishino.getApIPInfo(ip, gw, nm))
	{
		Serial << F("Fishino IP      :") << ip << "\r\n";
		Serial << F("Fishino GATEWAY :") << gw << "\r\n";
		Serial << F("Fishino NETMASK :") << nm << "\r\n";
	}
	else
		Serial << F("Couldn't get Fishino IP info\r\n");
	
#else

	// print your Fishino's IP address:
	// stampa l'indirizzo IP del Fishino
	IPAddress ip = Fishino.localIP();
	Serial.print("IP Address: ");
	Serial.println(ip);

	// print the received signal strength:
	// stampa la qualità del segnale WiFi
	Serial.print("signal strength (RSSI):");
	Serial.print(Fishino.RSSI());
	Serial.println(" dBm");
	
#endif

}

// this structure contains info for a single led
// questra struttura contiene le informazioni per un singolo led
struct LED
{
	const char *name;
	uint8_t port;
	bool state;
};

// led array - contains informations of all connected leds
// array di led - contiene le informazioni di tutti i led connessi
LED leds[] =
{
	{ "LED3", 3, false },
	{ "LED5", 5, false },
	{ "LED6", 6, false },
};

// number of connected leds - calculated from array
// numero di led connessi - ricavato automaticamente dall'array
const int numLeds = sizeof(leds) / sizeof(LED);

// sketch intialization
// inizializzazione dello sketch
void setup()
{
	// initialize serial port
	// apre la porta seriale
	Serial.begin(115200);

	// wait for serial port to connect. Needed for Leonardo only
	// attende l'apertura della porta seriale. Necessario solo per le boards Leonardo
	while (!Serial)
		;

	// reset and test wifi module
	// resetta e testa il modulo WiFi
	Serial << F("Resetting Fishino...");
	while(!Fishino.reset())
	{
		Serial << ".";
		delay(500);
	}
	Serial << F("OK\r\n");

	Serial << F("Fishino WiFi Web Button\r\n");

	// set PHY mode to 11G
	// imposta la modalità fisica a 11G
	Fishino.setPhyMode(PHY_MODE_11G);
	
	// for AP MODE, setup the AP parameters
	// imposta i parametri dell'Access Point, se in modalità standalone
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
	Fishino.setApIPInfo(ip, gw, nm);

	Serial << F("Setting AP WiFi parameters\r\n");
	Fishino.softApConfig(MY_SSID, MY_PASS, 1, false);
	
	// restart DHCP server
	Serial << F("Starting DHCP server\r\n");
	Fishino.softApStartDHCPServer();
	
#else

	// setup STATION mode
	// imposta la modalitè STATION
	Serial << F("Setting mode STATION_MODE\r\n");
	Fishino.setMode(STATION_MODE);

	// NOTE : INSERT HERE YOUR WIFI CONNECTION PARAMETERS !!!!!!
	Serial << F("Connecting to AP...");
	while(!Fishino.begin(MY_SSID, MY_PASS))
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

#endif

	// show connection status
	// visualizza lo stato della connessione
	printWifiStatus();
	
	// init leds I/O ports
	// inizializza gli I/O dei led
	for(uint8_t iLed = 0; iLed < numLeds; iLed++)
	{
		uint8_t port = leds[iLed].port;
		pinMode(port, OUTPUT);
		digitalWrite(port, LOW);
	}

	// start listening for clients
	// inizia l'attesa delle connessioni
	server.begin();
}

// process client's GET line
// processa la linea GET proveniente dal client
void processLine(const char *buf)
{
	char ledName[30];
	
	// the line must start with 'GET /?'
	// la linea deve iniziare con 'GET /?'
	if(strncmp(buf, "GET /?", 6))
		// oops, an error occurred!
		// ops, qualcosa non va
		return;
	
	// ok, advance to led name
	// of, avanza fino al nome del led
	buf += 6;
	
	// copia il nome del led
	char *pName = ledName;
	while(*buf && *buf != '=' && pName - ledName < 29)
		*pName++ = *buf++;
	*pName = 0;
	
	Serial.println();
	Serial.print("NOME : ");
	Serial.println(ledName);
	
	// search for led by name inside available leds
	// cerca il led per nome tra i leds disponibili
	for(uint8_t iLed = 0; iLed < numLeds; iLed++)
	{
		if(!strcmp(ledName, leds[iLed].name))
		{
			LED & led = leds[iLed];
			
			// found!! toggle it
			// trovato!! ne inverte il valore
			led.state = !led.state;
			
			// modifica l'uscita corrispondente
			digitalWrite(led.port, led.state);
			
			// termina il ciclo
			break;
		}
	}
}

// ciclo infinito
void loop(void)
{
	// wait for a new client:
	// attende nuovi clienti
	FishinoClient client = server.available();

	if (client)
	{
		Serial.println("new client");
		// an http request ends with a blank line
		// ogni richiesta http termina con una linea vuota
		bool currentLineIsBlank = true;
		
		// flag for first line (the GET one) read
		// flag che indica che la prima riga (quella con la GET) è stata letta
		bool gotGetLine = false;
		
		// buffer for GET line
		// buffer per la linea GET
		char buf[100];
		uint8_t iBuf = 0;
		
		while (client.connected())
		{
			if (client.available())
			{
				char c = client.read();
				Serial.write(c);
				
				// if we're reading the first line, appena the char to it
				// se stiamo leggendo la prima linea, aggiungiamoci il carattere letto
				if(!gotGetLine && c != '\r' && c != '\n' && iBuf < 99)
					buf[iBuf++] = c;
				
				// if we've got an end of line character, first line is terminated
				//
				// se leggiamo un carattere di fine linea, la linea è terminata
				if((c == '\r' || c == '\n') && !gotGetLine)
				{
					// mark first line as read
					// indica che la prima linea è stata letta
					buf[iBuf] = 0;
					gotGetLine = true;
					
					// process the line to toggle the led
					// processa la linea per attivare/disattivare il led
					processLine(buf);
				}
				
				// if you've gotten to the end of the line (received a newline
				// character) and the line is blank, the http request has ended,
				// so you can send a reply
				// se si è arrivati a fine linea (carattere 'newline' ricevuto
				// e la linea è vuota, la richiesa http è terminata
				// quindi è possibile inviera una risposta
				if (c == '\n' && currentLineIsBlank)
				{
					// send a standard http response header
					// invia uno header standard http
					client << F("HTTP/1.1 200 OK\r\n");
					client << F("Content-Type: text/html\r\n");

					// the connection will be closed after completion of the response
					client << F("Connection: close\r\n\r\n");
					client << F(
						"<!DOCTYPE HTML>\r\n"
						"<html>\r\n"
						"<head>\r\n"
						"<title>Fishino Web Button Demo</title>\r\n"
						"</head>\r\n"
						"<body>\r\n"
						"<style>\r\n"
						"input[type=submit] {\r\n"
						"-webkit-appearance: none; -moz-appearance: none;\r\n"
						" display: block;\r\n"
						" margin: 1.5em 0;\r\n"
						" font-size: 2em; line-height: 3em;\r\n"
						" color: #333;\r\n"
						" font-weight: bold;\r\n"
						" height: 3em; width: 100%;\r\n"
						" background: #fdfdfd; background: -moz-linear-gradient(top, #fdfdfd 0%, #bebebe 100%); background: -webkit-gradient(linear, left top, left bottom, color-stop(0%,#fdfdfd), color-stop(100%,#bebebe)); background: -webkit-linear-gradient(top, #fdfdfd 0%,#bebebe 100%); background: -o-linear-gradient(top, #fdfdfd 0%,#bebebe 100%); background: -ms-linear-gradient(top, #fdfdfd 0%,#bebebe 100%); background: linear-gradient(to bottom, #fdfdfd 0%,#bebebe 100%);\r\n"
						" border: 1px solid #bbb;\r\n"
						" -webkit-border-radius: 10px; -moz-border-radius: 10px; border-radius: 10px;\r\n"
						"}\r\n"
						"</style>\r\n"

						"<form action=\"\" method=\"get\">\r\n"
					);
					for(uint8_t iLed = 0; iLed < numLeds; iLed++)
					{
						LED &led = leds[iLed];
						client << F(
							"<div>\r\n"
							"<input type=\"submit\" name=\""
						);
						client << led.name;
						client << F("\" value=\"");
						client << led.name << " ";
						if(led.state)
							client << "OFF";
						else
							client << "ON";
						client << F("\">\r\n");
					}
					client <<
						"</form>\r\n"
						"</body>\r\n"
						"</html>\r\n"
					;
					break;
				}
				if (c == '\n')
				{
					// you're starting a new line
					// inizio di una nuova linea
					currentLineIsBlank = true;
				}
				else if (c != '\r')
				{
					// you've gotten a character on the current line
					// sono stati ricevuti dei caratteri nella linea corrente
					currentLineIsBlank = false;
				}
			}
		}
		// give the web browser time to receive the data
		// lascia tempo al browser per ricevere i dati
		delay(1);

		// close the connection:
		// chiudi la connessione
		client.stop();
		Serial.println("client disonnected");
	}
}
