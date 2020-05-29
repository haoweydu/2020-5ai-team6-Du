///////////////////////////////////////////////////////////////////
//   Fishino ESP Serial demo                                     //
//   Created by Massimo Del Fedele, December 9, 2016.            //
//   Test Serial port I/O on ESP module -- NOT FOR FISHINO32     //
///////////////////////////////////////////////////////////////////
#include <Fishino.h>

#ifdef _FISHINO32_
	#error "Fishino32 has no exposed serial port on WiFi module"
#endif

void setup(void)
{
	Serial.begin(115200);
	FishinoSerial.begin(115200);
}

void loop(void)
{
	if(Serial.available())
		FishinoSerial.write(Serial.read());
	while(FishinoSerial.available())
		Serial.print((char)FishinoSerial.read());
}