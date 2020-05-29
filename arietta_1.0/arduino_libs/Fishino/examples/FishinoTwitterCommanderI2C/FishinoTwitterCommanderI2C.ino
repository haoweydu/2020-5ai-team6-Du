/////////////////////////////////////////////////////////////////////////////////////////////
// Twitter search sample sketch                                                            //
//                                                                                         //
// This sketch uses Twitter APIs to search for twitters using Fishino's https capabilities.//
// It search the hashtag "#fishino" on Twitter network and displays results on serial port //
//                                                                                         //
// Circuit:                                                                                //
//   Fishino                                                                               //
//                                                                                         //
// created 23 September 2015 by Massimo Del Fedele                                         //
/////////////////////////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <IOExpanderMCP23017.h>
#include <JSONStreamingParser.h>
#include <Fishino.h>
#include <SPI.h>

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// CONFIGURATION DATA		-- ADAPT TO YOUR NETWORK !!!
// DATI DI CONFIGURAZIONE	-- ADATTARE ALLA PROPRIA RETE WiFi !!!

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

// here put your application Bearer authorization key
// see https://dev.twitter.com/oauth/application-only for details
// inserire qui la chiave "bearer" di autenticazione dell'applicazione
// vedere https://dev.twitter.com/oauth/application-only per dettagli
#define MY_TWITTER_BEARER ""

#endif

// here put your Twitter user name; if empty the application will react to ANY user
// inserire qui il nome utente ai cui tweets il programma si attiva; se vuoto, il programma si attiva con TUTTI gli utenti
#define TWITTER_USER "mdelfede"

// here put twitter hashtag needed to trigger the event (without the # char!)
// inserire qui l'hashtag necessario ad avviare l'applicazione (senza il carattere #!)
#define TWITTER_HASHTAG "fishino"

// here put the delay in milliseconds between requsts
// there is a limit of about 400 requests every 15 minutes, so don't put a too short time here
// inserire qui il ritardo tra due richieste successive
// c'è un limite di circa 180 richieste ogni 15 minuti, quindi non inserire un tempo troppo breve
#define REQUESTS_INTERVAL 5000

// NOTES :
//	the application waits for a tweet from a given user (or ANY tweet if empty....) of this format:
//	#TWITTER_HASHTAG digital_number ON|OFF
//	example: #fishino 13 ON
// NOTE :
//	l'applicazione si aspetta un tweet dall'utente specificato (o da qualsiasi utente, se vuoto) con l'hashtag specificato.
//	#TWITTER_HASHTAG digital_number ON|OFF
//	esempio: #fishino 13 ON

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

// the I/O expander board classes and count
// we find the amount of connected boards on setup
// if none we default to use the (few) I/O pins on Fishino
// le schede I/O expanders ed il loro numero
// il numero di schede viene determinato nella setup()
// se nessuna scheda viene trovata, il programma usa i (pochi) I/O ports del Fishino
IOExpanderMCP23017 *expanders[8];
uint8_t numExpanders;

// The web secure client used to access twitter API
// il client https usato per l'accesso all'API di Twitter
FishinoSecureClient client;

// prints WiFi status
// stampa lo stato della connessione WiFi
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

// flags and variables to store twitter events
// flags e variabili per memorizzare gli eventi Twitter

// the JSON parser
// il parser JSON
JSONStreamingParser parser;

// user has been found in message
// l'utente è stato trovato nel messaggio
bool gotUser;

// command got from message (ON/OFF)
// comando letto dal messaggio (ON/OFF)
bool command;

// port to change, got from message. 0xff if error
// port da modificare, letto dal messaggio. 0xff in caso di errore
uint8_t port;

// minimum ID for searched messages. Used to skip older ones
// ID da cui partire a cercare i messaggi.Utilizzato per saltare quelli già letti
uint64_t sinceID;

// last ID for searched messages. Used to walk back to read older ones one by one
// ultimo ID nella ricerca dei messaggi. Usato per percorrere all'indietro uno ad uno i messaggi
uint64_t maxID;

// next since ID, used to set sinceID after walkback on current search
// prossimo sinceID, utilizzato per impostare il sinceID dopo la ricerca a ritroso corrente
uint64_t nextID;

// we need this one to print IDs which are 64 bit unsigned integers----
template<>inline Print &operator <<(Print &stream, uint64_t arg)
{
	if(!arg)
		stream.print('0');
	else
	{
		char buf[21];
		char *bufp = buf + 20;
		*bufp = 0;
		while(arg && bufp > buf)
		{
			uint8_t i = arg % 10;
			arg /= 10;
			*(--bufp) = i + '0';
		}
			
		stream.print(bufp);
	}
	return stream;
}


// string comparaison, no case and fixed length
// confronto stringhe senza differenziare maiuscole e minuscole ed a lunghezza fissa
int strnicmp(const char *s1, const char *s2, size_t len)
{
	while(len--)
	{
		if(!*s1 || !*s2)
			return 1;
		if(toupper(*s1++) != toupper(*s2++))
			return 1;
	}
	return 0;
}

// read a twitter id from a string (and advance string pointer)
// legge un ID twitter da una stringa (e avanza il puntatore)
uint64_t readID(const char *&str)
{
	uint64_t res = 0;
	while(*str && isdigit(*str))
		res = res * 10 + *str++ - '0';
	return res;
}

// parser callbacks
// callbacks del parser

// parser callback used just to gather last ID
// callback del parser utilizzata solo per leggere l'ultimo ID
void lastIDCallback(uint8_t filter, uint8_t level, const char *name, const char *value, void *obj)
{
	if(level == 1 && !strcmp(name, "max_id"))
	{
		Serial << F("\n\nVALUE : ") << value << "\n\n";
		sinceID = readID(value);
	}
}

// analyze data and act on devices
// nalizza i dati in arrivo da Twitter
void parserCallback(uint8_t filter, uint8_t level, const char *name, const char *value, void *obj)
{
	// check for user name
	// controlla il nome utente
	if(level == 3 && !strcmp(name, "screen_name") && !strcmp(value, "\"" TWITTER_USER "\""))
		gotUser = true;
	
	// check twitter message - MUST contain the requested hashtag, a port number and word ON or OFF
	// controlla il testo del messaggio - DEVE contenere l'hashtag richiesto, un numero di porta e le parole ON o OFF
	else if(level == 2 && !strcmp(name, "text") && value && value[0] && !strnicmp(value + 1, "#" TWITTER_HASHTAG, strlen(TWITTER_HASHTAG) + 1))
	{
		// hashtag found, skip it
		// trovato l'hashtag, lo salta
		const char *p = value + strlen(TWITTER_HASHTAG) + 3;
		
		// skip spaces after hashtag
		// salta gli spazi dopo l'hashtag
		while(*p && isspace(*p))
			p++;
		
		// read port number - sets it to 0xff if not found
		// legge il numero di porta - lo imposta a 0xff se non trovato
		if(isdigit(*p))
		{
			port = 0;
			while(isdigit(*p))
				port = port * 10 + *p++ - '0';
		}
		else
			port = 0xff;
		
		// if port not found, just leave
		// se non trova il numero di port ritorna
		if(port == 0xff)
			return;
		
		// skip spaces after port number
		// salta gli spazi dopo il numero di porta
		while(*p && isspace(*p))
			p++;
		
		// chek for ON or OFF keywords - if not found sets port to 0xff and leave
		// cerca le parole chiave ON o OFF - se non trovate imposta la porta a 0xff ed esce
		if(!strnicmp(p, "on", 2))
			command = true;
		else if(!strnicmp(p, "off", 3))
			command = false;
		else
			port = 0xff;
	}
	
	// look if there are previous tweets to handle
	// cerca se deve elaborare tweets precedenti
	else if(level == 1 && !strcmp(name, "next_results"))
	{
		// get next tweet id to handle
		const char *s = strstr(value, "\"?max_id=");
		if(s)
		{
			s += 9;
			nextID = readID(s);
		}
	}
	
	// get max id of search, needet to go further after walk back
	// cerca il massimo ID corrispondente alla ricerca, necessario per proseguire dopo la ricerca all'indietro
	else if(level == 2 && !strcmp(name, "id"))
	{
		uint64_t id = readID(value);
		if(id > maxID)
			maxID = id;
	}
}

// run a query on twitter
// esegue la query a Twitter
void doQuery(FishinoSecureClient &client, const char *query)
{
	// if walkback is finished (nextID == -1) we reset sinceID to next found maximum ID
	// otherwise we let it as it is and continue the walk back
	// se il walkback è finito (nextID == -1) re-impostiamo il sinceID a dopo il massimo ID trovato
	// altrimenti lasciamo tutto come sta e continuiamo col walkback
	if(nextID == (uint64_t)-1)
		sinceID = maxID;

	client << F("GET /1.1/search/tweets.json?count=1&q=") << query;

	if(strlen(TWITTER_USER))
		client << F("+from%3A" TWITTER_USER);

	client << F("&result_type=recent");

	if(nextID != (uint64_t)-1)
		client << F("&max_id=") << nextID;

	client
		<< F("&since_id=")
		<< sinceID
		<< F(" HTTP/1.1\n")
		<< F("Host: api.twitter.com\n")
		<< F("User-Agent: Fishino Twitter Client\n")
	;

	// send auth key to server
	// invia la chiave di autorizzazione al server
	client << F("Authorization: Bearer " MY_TWITTER_BEARER "\n\n");
	
	// reset nextID - it will be filled by parser if more results are pending
	// azzera il nextID - verrà riempito dal parser se ci sono altri risultati da leggere
	nextID = -1;
}

// waits for data from twitter and parse them - return true on sucess, false if no data
bool doReceive(FishinoSecureClient &client, JSONStreamingParser &parser)
{
	// wait 1 second max for data from server
	// attende al massimo 1 secondo i dati dal server
	unsigned long tim = millis() + 1000;
	while(!client.available() && millis() < tim)
		;
	
	// if no data is available, close the connection
	// se non ci sono dati, chiude la connessione
	if(!client.available())
		return false;
	
	// skip data up to 2 consecutive cr
	bool cr = false;
	while(client.available())
	{
		char c;
		if(cr)
		{
			if((c = client.read()) == 0x0d && client.read() == 0x0a)
				break;
			else
				cr = false;
		}
		else if( (c = client.read()) == 0x0d && client.read() == 0x0a)
			cr = true;
	}
	
	while(client.available())
	{
		char c = client.read();
		parser.feed(c);
	}
	return true;
}

// find all available IOExpander boards
void findIOExpanders(void)
{
	numExpanders = 0;
	for(uint8_t addr = 0; addr < 8; addr++)
	{
		IOExpanderMCP23017 *exp = new IOExpanderMCP23017;
		
		// set expander address
		exp->begin(addr);

		// this is because expander board hasn't reset connected to arduino one
		// and it checks register 0 upon init to see if board is present
		// (register should be 0xff, all A port as input)
		for(int i = 0; i < 16; i++)
			exp->pinMode(i, INPUT);
		
		if(exp->init())
		{
			// expander found, add to list and setup pins
			expanders[numExpanders++] = exp;

			// set pin modes
			for(int i = 0; i < 8; i++)
				exp->pinMode(15-i, OUTPUT);
		}
		else
			// not found, delete expander object
			delete exp;
	}
	if(numExpanders)
		Serial << F("Found ") << numExpanders << F(" I/O expander boards\n");
	else
		Serial << F("No expander boards found - using Fishino's I/O pins\n");
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
	
	// accept max 40 chars from twitter items (to not use too much memory
	// accetta massimo 40 caratteri per item da twitter in modo da non usare troppa memoria
	parser.setMaxDataLen(40);
	
	// initialize wire library
	Wire.begin();
	
	// gather any available expander boards
	// ricerca le schede IO expander
	findIOExpanders();
	// initialize SPI
	// inizializza il modulo SPI
	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	
	// reset and test WiFi module
	// resetta e testa il modulo WiFi
	while(!Fishino.reset())
		Serial << F("Fishino RESET FAILED, RETRYING....\n");
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
	
	// run a dummy query at startup to gather current maximum ID
	// in order to not get older tweets
	sinceID = 0;
	nextID = -1;
	parser.setCallback(lastIDCallback, NULL);
	parser.reset();
	if(client.connect("api.twitter.com", 443))
	{
		doQuery(client, "%23" TWITTER_HASHTAG);
		doReceive(client, parser);
		client.stop();
	}
	sinceID++;
	maxID = sinceID;
	Serial << F("Processing tweets from ID #") << sinceID << "\n";

	// reinit parser with true callback
	// reinizializza il parser con la callback definitiva
	parser.setCallback(parserCallback, 0);
	parser.reset();
}

void loop()
{
	// if client is not connected and there are still no data to process....
	// se il client non è connesso e se non sono rimasti dati da processare....
	if (!client.connected() && !client.available())
	{
		Serial.println("\nStarting connection to server...");
		if(client.connect("api.twitter.com", 443))
		{
			Serial.println("connected to server");
/*
Serial << F("SinceID:") << sinceID << "\n";
Serial << F("MaxID  :") << maxID << "\n";
Serial << F("NextID :") << nextID << "\n";
*/
			// run the query looking from given hashtag and user
			doQuery(client, "%23" TWITTER_HASHTAG);
		}
	}
	
	// clear port
	// azzera la porta
	port = 0xff;
	
	// reset the parser
	// reinizializza il parser
	parser.reset();
	
	// gather data from twitter, stop client if none
	// raccoglie i dati da twitter, ferma il client se non ce ne sono
	if(!doReceive(client, parser))
		client.stop();
	
	// if port has been set, change its output
	// se la porta è stata impostata, ne modifica l'output
	if(port != 0xff)
	{
		// if no expanders found, check if pin is free on Fishino
		// and set its output (must be changed on Mega!)
		if(!numExpanders)
		{
			if(port == 4 || port == 7 || port >= 10)
				Serial << F("INVALID PORT #") << port << F(" -- COMMAND DISCARDED\n");
			else
			{
				Serial << "\n\nSETTING PORT " << port << " TO " << (command ? "HIGH" : "LOW") << "\n";
				pinMode(port, OUTPUT);
				digitalWrite(port, command ? HIGH : LOW);
			}
		}
		else
		{
			uint8_t board = port / 8;
			if(board >= numExpanders)
				Serial << F("INVALID PORT #") << port << F(" -- COMMAND DISCARDED\n");
			else
			{
				Serial << "\n\nSETTING PORT " << port << " TO " << (command ? "HIGH" : "LOW") << "\n";
				port %= 8;
				expanders[board]->digitalWrite(15-port, command ? HIGH : LOW);
			}
		}
	}
	
	// wait given time between requests
	// attende i secondi dati tra 2 richieste
	if(!client.available())
		delay(REQUESTS_INTERVAL);
}
