///////////////////////////////////////////////////////////////////
//   Fishino ESP I/O demo                                        //
//   Created by Massimo Del Fedele, December 9, 2016.            //
//   Test I/O functions on ESP module -- NOT FOR FISHINO32       //
///////////////////////////////////////////////////////////////////
#include <Fishino.h>
#include <SPI.h>

#ifdef _FISHINO32_
#error "Fishino32 has no exposed I/O pins on WiFi module"
#endif

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
	
	Serial << F("FISHINO GPIO BLINK EXAMPLE\n");
	
	delay(500);
	
	// reset and test WiFi module
	// resetta e testa il modulo WiFi
	while(!Fishino.reset())
		Serial << F("Fishino RESET FAILED, RETRYING....\n");
	Serial << F("Fishino WiFi RESET OK\n");
	
	// initialize ESP GPIO4 to output
	Fishino.pinMode(4, OUTPUT);

	Serial << F("pinMode OK\n");
}

// ciclo infinito
void loop(void)
{
	Fishino.digitalWrite(4, LOW);
	Serial << "Analog:" << Fishino.analogRead() << "\n";
	delay(250);
	Fishino.digitalWrite(4, HIGH);
	Serial << "Analog:" << Fishino.analogRead() << "\n";
	delay(250);
}
