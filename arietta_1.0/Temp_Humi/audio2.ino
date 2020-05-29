#include <math.h>

#define bias 514
#define DB_REF 75
#define AC_REF 3.00

int value;
int i;
double result;
int vett[200];
double rms;
double db;
double dimm = 200;





double returnDB(double sound){
  if (sound < 2.40){  
    return 50;
    
  }else{
    db = 20 * log10(sound/AC_REF);
    result = db + DB_REF;
    return result;
  }
}


double measureAudio(){
  int somma;
  
  for (i = 0; i < dimm; i++){
    int sensorValue = analogRead(A0);
    value = (sensorValue - bias);
    somma += value * value;
  }
  rms = sqrt(somma / dimm);
  return rms;
}
