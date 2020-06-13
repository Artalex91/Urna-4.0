// Urna-4.0
// 12.05.20 увеличел время до защиты

#include <Arduino.h>
#include <avr/wdt.h> //вачдог
#include <Wire.h>
#include <VL53L0X.h>
VL53L0X sensor;

const int ledPin      =13;
const int releUpPin   = 4;
const int releDownPin = 5;
const int concDownPin = 2; // прерывание 0   2
const int concUpPin   = 3; // прерывание 1   3

volatile int  motion  =4;
bool open             =false;

int range             =1000;
const int constRange  =800;
const int constOpenMill = 10000;  // время пока открыто
uint32_t openMill     =0;

uint32_t printMill    =0;

uint32_t timeUpOld=0, timeDownOld=0, timeUpNew=0, timeDownNew=0, mill=0;
bool timeFlagUp=false, timeFlagDown=false;
uint32_t timeOpen=0, timeClose=0;

bool protect=false;
bool ledState=false;
uint32_t blinkMill=0;
const int timeProtect=1500;  // защита через..
const int blinkDelay=500;    // частота моргания при протесте

// для медианного фильтра
int val[3] = {1000, 1000, 1000};
byte index;


// медианный фильтр из 3ёх значений
int middle_of_3(int a, int b, int c) {
  int middle;
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  }
  else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    }
    else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}
//-----------------------------------


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


void setup() {
  Serial.begin(9600);
    Wire.begin();

  sensor.init();
  sensor.setTimeout(500);
  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();

  pinMode(concDownPin, INPUT_PULLUP);//2
  pinMode(concUpPin,   INPUT_PULLUP);//3

  attachInterrupt(0, concUpAttach,   FALLING); //2
  attachInterrupt(1, concDownAttach, FALLING); //3

  pinMode(releUpPin,   OUTPUT);
  pinMode(releDownPin, OUTPUT);
  pinMode(ledPin,      OUTPUT);
  wdt_enable(WDTO_8S);  // вачдог
}

void loop() {
  
  if(millis()-printMill>1000){
    printMill=millis();
    /*
    Serial.print(sensor.readRangeSingleMillimeters());
    Serial.print("  up ");
    Serial.print(digitalRead(concUpPin));
    Serial.print("  down ");
    Serial.print(digitalRead(concDownPin));
    Serial.print("  open ");
    Serial.print(open);
    Serial.print("  RU ");
    Serial.print(digitalRead(releUpPin));
    Serial.print("  RD ");
    Serial.print(digitalRead(releDownPin));
   */
    if (sensor.timeoutOccurred())
    {
      Serial.print("  TIMEOUT");
    }
    else 
    {
      //Serial.print("  no TIMEOUT");
      wdt_reset();  // вачдог
    }
    //Serial.println();
  }

    //range = sensor.readRangeSingleMillimeters();  // при работе без фильтра

// медианный фильтр
 if (++index > 2) index = 0; // переключаем индекс с 0 до 2 (0, 1, 2, 0, 1, 2…)
  val[index] = sensor.readRangeSingleMillimeters(); // записываем значение с датчика в массив
  // фильтровать медианным фильтром из 3ёх ПОСЛЕДНИХ измерений
  if (index == 2){
     range = middle_of_3(val[0], val[1], val[2]);
     if (range < 8190){
     Serial.print(val[0]);
     Serial.print("  ");
     Serial.print(val[1]);
     Serial.print("  ");
     Serial.println(val[2]);
     }
  }
 // if (range < 8190) Serial.println(range); // для примера выводим в порт
//------------------------------
 if (index == 2){
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
 }



  if(digitalRead(concUpPin)==HIGH && digitalRead(concDownPin)==HIGH){ // оба отжаты

    if(open==true){
      if     (digitalRead(releUpPin)==HIGH){}
      else if(digitalRead(releDownPin)==HIGH){motion=1;}
      else if(digitalRead(releUpPin)==LOW && digitalRead(releDownPin)==LOW){motion=1;}
    }
    else{
      if     (digitalRead(releUpPin)==HIGH){}
      else if(digitalRead(releDownPin)==HIGH){}
      else if(digitalRead(releUpPin)==LOW && digitalRead(releDownPin)==LOW){}
    }
  } 
  else if(digitalRead(concUpPin)==LOW  && digitalRead(concDownPin)==HIGH){ // открыта крышка

    if(open==true){motion=2;}
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
      if(protect==false)digitalWrite(releUpPin,   HIGH);
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
      if(protect==false)digitalWrite(releDownPin, HIGH);
      motion=4;
      break;
    case 4: //ожидание
      break;
  }

  
  if (digitalRead(releUpPin)==HIGH && timeFlagUp==false){
    timeFlagUp=true;
    timeUpOld=millis();
  }
  if (digitalRead(releUpPin)==LOW && timeFlagUp==true){
    timeFlagUp=false;
    timeUpNew=millis();
    timeOpen=timeUpNew-timeUpOld;
    Serial.print("timeOpen ");
    Serial.println(timeOpen);
  }
  mill=millis();
  if(mill-timeUpOld>timeProtect && timeFlagUp==true){
    timeFlagUp=false;
    digitalWrite(releUpPin,   LOW);
    digitalWrite(releDownPin, LOW);
    protect=true;
    Serial.print("PROTECT open ");
    Serial.print(millis()-timeUpOld);
  }

  if (digitalRead(releDownPin)==HIGH && timeFlagDown==false){
    timeFlagDown=true;
    timeDownOld=millis();
  }
  if (digitalRead(releDownPin)==LOW && timeFlagDown==true){
    timeFlagDown=false;
    timeDownNew=millis();
    timeClose=timeDownNew-timeDownOld;
    Serial.print("timeClose ");
    Serial.println(timeClose);
  }
  mill=millis();
  if(mill-timeDownOld>timeProtect && timeFlagDown==true){
    timeFlagDown=false;
    digitalWrite(releUpPin,   LOW);
    digitalWrite(releDownPin, LOW);
    protect=true;
    Serial.print("PROTECT close ");
    Serial.print(millis()-timeDownOld);
  }
  mill=millis();
  if(protect==true && mill-blinkMill>blinkDelay){
    blinkMill=millis();
    ledState= !ledState;
    digitalWrite(ledPin, ledState);
  }


}

