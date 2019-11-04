// Urna-4.0
#include <Arduino.h>

#include <Wire.h>
#include <VL53L0X.h>
VL53L0X sensor;

const int releUpPin   = 4;
const int releDownPin = 5;
const int concDownPin = 2; // прерывание 0   2
const int concUpPin   = 3; // прерывание 1   3

volatile int  motion  =4;
bool open             =false;
int cycle             =0;

int range             =0;
const int constRange  =550;
const int constOpenMill = 5000;
uint32_t openMill     =0;


void setup() {

    Wire.begin();

  sensor.init();
  sensor.setTimeout(500);
  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();

  pinMode(concDownPin, INPUT);//2
  pinMode(concUpPin,   INPUT);//3

  attachInterrupt(0, concUpAttach,   FALLING); //2
  attachInterrupt(1, concDownAttach, FALLING); //3

  pinMode(releUpPin,   OUTPUT);
  pinMode(releDownPin, OUTPUT);

}

void loop() {

    range = sensor.readRangeSingleMillimeters();

  if (range < constRange)
    openMill = millis();
  if (range < constRange && open != true)
  {
    open = true;
  }
  else if (range >= constRange && open != false)
  {
    if (millis() - openMill > constOpenMill)
    {
      open = false;
    }
  }




  if(digitalRead(concUpPin)==HIGH && digitalRead(concDownPin)==HIGH){ // оба отжаты
    if(open==true){
      if     (releUpPin==HIGH){}
      else if(releDownPin==HIGH){motion=1;}
      else if(releUpPin==LOW && releDownPin==LOW && cycle<5){
        digitalWrite(releUpPin, HIGH);
        delay(200);
        cycle++;
        digitalWrite(releUpPin, LOW);
        delay(1000);
      }
    }
    else{
      if     (releUpPin==HIGH){}
      else if(releDownPin==HIGH){}
    }
  } 
  else if(digitalRead(concUpPin)==LOW  && digitalRead(concDownPin)==HIGH){ // открыта крышка
    if(open==true){motion=2; cycle=0;}
    else{motion=3;}
  }
  else if(digitalRead(concUpPin)==HIGH && digitalRead(concDownPin)==LOW ){ // закрыта крышка
    if(open==true){motion=1;}
    else{motion=2;}
  }
  else if(digitalRead(concUpPin)==LOW  && digitalRead(concDownPin)==LOW ){ // оба нажаты
    if(open==true){}
    else{}
  }


  switch (motion) {
    case 1: //вверх
      digitalWrite(releUpPin,   HIGH);
      digitalWrite(releDownPin, LOW );
      motion=4;
      break;
    case 2: //стоп
      digitalWrite(releUpPin,   LOW);
      digitalWrite(releDownPin, LOW);
      motion=4;
      break;
    case 3: //вниз
      digitalWrite(releUpPin,   LOW );
      digitalWrite(releDownPin, HIGH);
      motion=4;
      break;
    case 4: //ожидание
      break;
  }
}

void concUpAttach() {
  digitalWrite(releUpPin,   LOW);
  digitalWrite(releDownPin, LOW);
  motion=2;
}

void concDownAttach() {
  digitalWrite(releUpPin,   LOW);
  digitalWrite(releDownPin, LOW);
  motion=2;
}