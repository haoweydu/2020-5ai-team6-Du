#include <Fishino.h>
#include "DHT.h"

#define DHTPIN 2 //definisco il PIN digitale al quale connettere il sensore.   
#define DHTTYPE DHT22  
#define COMPUTER_LOCAL_IP ip //qui puoi definire l'ip della tua macchina locale al posto di "ip"

DHT dht(DHTPIN, DHTTYPE);  //inizializzo il sensore e gli passo come argomenti il PIN utilizzato per il trasferimento dei dati e la TIPOLOGIA di sensore
char server[] = "COMPUTER_LOCAL_IP"; //indirizzo locale della macchina sulla quale risiede il db
FishinoClient client;

String postData;
String postVariable = "temp=";
String postVariable2 = "umi=";
String postVariable3 = "suono=";





void setup() {
  Serial.begin(9600); //inizializza porta seriale
  
  checkReset(); //reset di ESP8266 e verifica del suo funzionamento
  setStation();  //imposta il Fishino in modalità stazione
  
  
  connectToNetwork("SSID", "Wifi-Pass");  //inserire SSID della rete Wifi e relativa password
 
  Serial.println("\n");

 

  Serial.println("Misuro umidità e temperatura");
  Serial.println("\n");
  dht.begin(); //rende operativo il modulo DHT22
}

void loop() {
 
  float h = dht.readHumidity();   //leggo l'umidità
  float t = dht.readTemperature();;  //leggo la temperatura
  double rms = measureAudio();
  //Serial.println(rms);
  double s = returnDB(rms);
  //Serial.println(s);
 
  
  sendDataToServer(t, h, s);
  Serial.println("Dati inviati al DB");
  Serial.println("\n");
  delay(600000);  //raccolgo i dati e li invio al DB ogni 10 minuti
}





void checkReset(){   
  while(true){
    if(Fishino.reset()){  
      Serial.println("Wifi resettato con successo");
      break;
    }else{
      Serial.println("Fallimento nel reset Wifi");
    }
  }
}



void connectToNetwork(const char* ssid, const char* password){   //tenta di connettersi alla rete con ssid e password, se fallisce printa messaggio di errore
    while(true){
    if(Fishino.begin(ssid, password)){
      Serial.println("Connesso alla rete");
      break;
    }
    else {
      Serial.println("Connessione fallita");
      Serial.println("Riprovo...");
    }
  }
}



void sendDataToServer(float t, float h, double s){

  postData = postVariable + t + "&" + postVariable2 + h + "&" + postVariable3 + s;

  Serial.println(postData);

   while(true){
    if (client.connect(server, 80)) {
      client.println("POST /arietta_1.0/arduinoconnection.php HTTP/1.1");
      client.println("Host: localhost");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.print(postData);
      break;
   } else {
    Serial.println("Invio dei dati al server fallito, riprovo..."); //errore in caso di connessione fallita
    }
    
  }
  
}



void setStation(){
  Serial.println("Imposto il modulo in modalità stazione'...");
  while(true){
    if (Fishino.setMode(STATION_MODE)){    //imposta il modulo wifi in modalità STAZIONE (si connette normalmente al modem Wifi)
      Serial.println("Modulo impostato in modalità STAZIONE");
      break;
    }else{
      Serial.println("Fallimento, riprovo...");
    }
  }
}






  
