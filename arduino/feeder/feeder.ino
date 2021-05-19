/*

aggiungere sensore feedback chiusura coperchio

Circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 13
 * 
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC/vdd pin to 5V
 * LCD Anode 10K resistor -> +5v
 * LCD Katode to ground
 * LCD VO pin -> 3kOhm -> +5v
 */
#define _TASK_PRIORITY
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_WDT_IDS
#define _TASK_TIMEOUT 
#define _TASK_TIMECRITICAL
#include "./TaskScheduler.h"
#include "LiquidCrystal.h"
#include <SoftwareSerial.h>
#include "Time.h"


#define MOTOR_IN1 11
#define MOTOR_IN2 10
#define MOTOR_IN3 9
#define MOTOR_IN4 8

#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7

#define DISPLAY_BTN 2
#define CLOSE_BTN 3
#define LCD_ANODE A2
#define CLOSE_SWITCH A3

#define PHOTORESISTOR A5
#define TEMPSENSOR A4

#define MOTOR_STEPS 200
int Steps = 0;
struct orario
{
  byte hours;
  byte minutes;
};
struct orario orario_apertura = {255,255} ;//per evitare che si apra a mezzanotte dati i valori di default 0 (byte range: 0-255)

int steps_left=MOTOR_STEPS;
char infoScreen1[17];
char infoScreen2[17];
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 13, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
SoftwareSerial EEBlue(A0, A1); // RX | TX

int lightDisplay = HIGH;
int displayStatus = HIGH;

void stringToBlue(String str) {
  for (int i = 0; i < str.length(); i++)
  {
    EEBlue.write(str[i]);
  }
}
Task displayOffTask(0, -1, []() {
  digitalWrite(LCD_ANODE,LOW);
  displayStatus = LOW;
});
Task restoreDisplayInfoTask(0,-1, []() {
  lcd.setCursor(0,0);
  lcd.write(infoScreen1);
  lcd.setCursor(0,1);
  lcd.write(infoScreen2);
});
Task closeTask(0, -1, []() {
  //close();
  Serial.println("CLOSE TASK CALLED");
  slowClose();
});
boolean alreadyOpened = false;
//===[TASK CHE CONTROLLA UNA VOLTA AL MINUTO SE E' L'ORARIO DI APRIRE IL COPERCHIO]===//
Task checkTimeTask(59000, TASK_FOREVER, []() {
  Serial.println(hour() + ":" + String(minute()) +" == " + orario_apertura.hours + ":" + orario_apertura.minutes);
  if(!alreadyOpened && hour() == orario_apertura.hours && minute() == orario_apertura.minutes){
    Serial.println("opening");
    open();
    alreadyOpened = true;
  } else {
    alreadyOpened = false;
  }
});


//aggiungere bottone per chiusura

/*
COMANDI DISPONIBILI:
I;               --> return info like shining (photoresistor), opening time loaded for autofeeder
S hh:mm;   --> Set next time opening slot = 0..NSECTION
T timestamp;      --> set datetime
*/
String command = "";
void executeCommand() {
  if(command[0] == 'I') {
    Serial.println("Info requested");
    char luminosita[6];
    strcpy(luminosita, "bassa");
    int l = analogRead(PHOTORESISTOR);
    int t = analogRead(TEMPSENSOR);
    float temp = (t*5000.0/1024)/10; //PROPORZIONE: ogni 10mV è un grado Celsius --> range analogRead 0-1024 --> 0-5000mV
    if(l > 340 && l < 680) {
      strcpy(luminosita, "media");
    } else if(l >= 680) {
      strcpy(luminosita, "alta");
    }
    char infoStr[106];
    strcpy(infoStr, "luminosita: ");
    strcat(infoStr, luminosita);
    strcat(infoStr, "\n");
    strcat(infoStr, "temperatura: ");
    char tempStr[15];
    char hh[3];
    char mm[3];
    char ss[3];
    dtostrf(temp, 2, 2, tempStr);
    //sprintf(tempStr, "%f", temp);
    sprintf(hh, "%02d", hour());
    sprintf(mm, "%02d", minute());
    sprintf(ss, "%02d", second());
    strcat(infoStr, tempStr);
    strcat(infoStr, "\n");
    strcat(infoStr, "orario: ");
    strcat(infoStr, hh);
    strcat(infoStr, ":");
    strcat(infoStr, mm);
    strcat(infoStr, ":");
    strcat(infoStr, ss);
    strcat(infoStr, "\n");
    strcat(infoStr, "apertura programmata: ");
    if(orario_apertura.hours == 255 && orario_apertura.minutes == 255) { //orario apertura non ancora impostato
      strcat(infoStr,"non impostata\n");
    } else {
      char hh2[3];
      char mm2[3];
      sprintf(hh2, "%02d", orario_apertura.hours);
      strcat(infoStr, hh2);
      strcat(infoStr, ":");
      sprintf(mm2, "%02d", orario_apertura.minutes);
      strcat(infoStr, mm2);
      strcat(infoStr, "\n");
    }
    //infoStr += "orario: " + String(hour()) + ":"+String(minute()) + ":" + String(second()) + " UTC\n";
    Serial.println(infoStr);
    stringToBlue(infoStr);
    //EEBlue.write('x');
  } else if(command[0] == 'T') {
     unsigned long timestamp = command.substring(2,command.length()-1).toInt(); //-1 remove last char ";" from reading
     setTime(timestamp);
     Serial.print("Orario:");
     Serial.println(String(hour()) +":"+String(minute())+":"+String(second())+"\n");
  } else if(command[0] == 'S') {
    orario_apertura.hours = (byte)command.substring(2,4).toInt();
    orario_apertura.minutes = (byte)command.substring(5,7).toInt();
    Serial.println("Orario impostato.");
    char lcdbuf[6];
    command.substring(2,7).toCharArray(lcdbuf,6);
    strcpy(infoScreen2,"     ");
    strcat(infoScreen2,lcdbuf);
    strcat(infoScreen2,"     ");
    
    lcd.setCursor(0,1);
    lcd.write(infoScreen2);
  }
}
Task bluetoothSerialTask(1000, TASK_FOREVER, []() {
  while (EEBlue.available()) { //build command
    char c = EEBlue.read();
    command += c;
    if(c == ';') { //end command character. (for dirty input)
      executeCommand();
      Serial.println(command);
      command = "";
    }
  }
  //if(command.length() > 0) Serial.println(command);
  
  
  
  while (Serial.available()) EEBlue.write(Serial.read());
  //Serial.println("Ended bluetooth check for data");
});
long lastTimeBtnDisplayPressed = 0;
void btnDisplayPressed() {
  //Serial.print("a");
  if(millis() - lastTimeBtnDisplayPressed > 100) {
    displayStatus = !displayStatus;
    digitalWrite(LCD_ANODE,displayStatus);
    displayOffTask.disable();
    displayOffTask.enableDelayed(5 * TASK_SECOND);
    lastTimeBtnDisplayPressed = millis();
  }
}
long lastTimeBtnClosePressed = 0;
void btnClosePressed() {
  Serial.println("BTN CLOSE PRESSED");
  if(millis() - lastTimeBtnClosePressed > 1000) {
    if(alreadyOpened) {
      lcd.setCursor(0,0);
      lcd.write("    CHIUSURA    ");
      lcd.setCursor(0,1);
      lcd.write("    IN CORSO    ");
      //don't close here because of interrupt short amount of time dedicated!!!!!!!!!
      closeTask.disable(); //eventually already started task.
      closeTask.enableDelayed(1 * TASK_SECOND);
      alreadyOpened = false;
    } else {
      lcd.setCursor(0,0);
      lcd.write("    CIOTOLA     ");
      lcd.setCursor(0,1);
      lcd.write("   GIA CHIUSA   ");
    }
    restoreDisplayInfoTask.disable();
    restoreDisplayInfoTask.enableDelayed(6 * TASK_SECOND);
    lastTimeBtnClosePressed = millis();
  }
}
Scheduler r1;
void setup()
{
  Serial.begin(9600);
  EEBlue.begin(9600);
  Serial.println("The bluetooth gates are open.\n Connect to HC-05 from any other bluetooth device with 1234 as pairing key!.");
  pinMode(MOTOR_IN1, OUTPUT); 
  pinMode(MOTOR_IN2, OUTPUT); 
  pinMode(MOTOR_IN3, OUTPUT); 
  pinMode(MOTOR_IN4, OUTPUT); 
  pinMode(DISPLAY_BTN,INPUT_PULLUP);
  pinMode(CLOSE_BTN,INPUT_PULLUP);
  pinMode(LCD_ANODE, OUTPUT);
  pinMode(PHOTORESISTOR,INPUT);
  pinMode(TEMPSENSOR,INPUT);
  pinMode(CLOSE_SWITCH,INPUT);
  
  r1.init();
  r1.addTask(displayOffTask);
  r1.addTask(bluetoothSerialTask);
  r1.addTask(checkTimeTask);
  r1.addTask(restoreDisplayInfoTask);
  r1.addTask(closeTask);
  attachInterrupt(digitalPinToInterrupt(DISPLAY_BTN), btnDisplayPressed, FALLING); //WITH FALLING instead of RISING(+ time check) i avoid bouncing effect on release because the interrupt is called only on release and not on press
  attachInterrupt(digitalPinToInterrupt(CLOSE_BTN), btnClosePressed, FALLING);
  
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  strcpy(infoScreen1,"PROSS. APERTURA:");
  strcpy(infoScreen2,"    NOT SET     ");
  lcd.write(infoScreen1);
  lcd.setCursor(0,1);
  lcd.write(infoScreen2);
  delay(1000);
  bluetoothSerialTask.enable();

  digitalWrite(LCD_ANODE,HIGH); //accendi schermo
  displayOffTask.enableDelayed(15 * TASK_SECOND); //spegni dopo primi 15secondi

  checkTimeTask.enable();
  //open();
  //open();
  //slowClose();
}
void loop()
{
  //Serial.print(digitalRead(DISPLAY_BTN));
  /*buttonStatus = digitalRead(7);
  Serial.print(buttonStatus);
  if(n == 0) {
    nextSection();
    n++;
  }
  delay(1000);
  digitalWrite(LCD_ANODE,lightDisplay);
  //digitalWrite(LCD_ANODE,buttonStatus);
  lightDisplay = !lightDisplay;*/
  r1.execute();
  //Serial.println(digitalRead(CLOSE_SWITCH));
}

void close() {
  int i = 0;
  while(i < 2038/4) {
    nextStep();
    delay(4);
    i++;
  }
}
boolean fullyClosed = false;
void slowClose() {
  boolean touched = false;
  int divisor = 256;
  
  Serial.println("Trying to close");
  while(!fullyClosed) {
    int i = 0;
    Serial.println("PRIMO CICLO");
    while(i < 2038/divisor) {
      Serial.println("SECONDO CICLO");
      nextStep();
      Serial.println(digitalRead(CLOSE_SWITCH));
      if(digitalRead(CLOSE_SWITCH)) { //controllo che l'interrutore sia chiuso (coperchio)
        touched = true;
        divisor = 2038;
        break;
      }
      delay(20);
      i++;
    }
    if(touched) {
      Serial.println("TOUCHEDDDD!");
      fullyClosed = true;
      break;
    }
  }
}
void open() {
  int i = 0;
  while(i < 2038/4) {
    previousStep();
    delay(4);
    i++;
  }
}
/*void nextSection() {
  int i = 0;
  while(i < 2038/NSECTION) {
    nextStep();
    delay(2);
    i++;
  }
}*/
void goToStep() {
  switch(Steps){
       case 0:
         digitalWrite(MOTOR_IN1, HIGH); 
         digitalWrite(MOTOR_IN2, LOW);
         digitalWrite(MOTOR_IN3, LOW);
         digitalWrite(MOTOR_IN4, LOW);
       break; 
       case 1:
         digitalWrite(MOTOR_IN1, LOW); 
         digitalWrite(MOTOR_IN2, HIGH);
         digitalWrite(MOTOR_IN3, LOW);
         digitalWrite(MOTOR_IN4, LOW);
       break; 
       case 2:
         digitalWrite(MOTOR_IN1, LOW); 
         digitalWrite(MOTOR_IN2, LOW);
         digitalWrite(MOTOR_IN3, HIGH);
         digitalWrite(MOTOR_IN4, LOW);
       break; 
       case 3:
         digitalWrite(MOTOR_IN1, LOW); 
         digitalWrite(MOTOR_IN2, LOW);
         digitalWrite(MOTOR_IN3, LOW);
         digitalWrite(MOTOR_IN4, HIGH);
       break; 
       default:
         digitalWrite(MOTOR_IN1, LOW); 
         digitalWrite(MOTOR_IN2, LOW);
         digitalWrite(MOTOR_IN3, LOW);
         digitalWrite(MOTOR_IN4, LOW);
       break; 
    }
}
void previousStep() {
  if(Steps == 0) { Steps = 4; }
  Steps--;
  goToStep();  
}
void nextStep(){
  if(Steps == 3){Steps = -1;}
  Steps++;
  goToStep();
    
}
