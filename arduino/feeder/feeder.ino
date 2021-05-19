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

//infomazioni mostrate sul display lcd --> memorizza le informazioni durante le transizioni del coperchio
char infoScreen1[17]; //16 caratteri prima linea display lcd
char infoScreen2[17]; //16 caratteri seconda linea display lcd

LiquidCrystal lcd(12, 13, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
SoftwareSerial EEBlue(A0, A1); // RX | TX
Scheduler r1;

int lightDisplay = HIGH;
int displayStatus = HIGH;


//===[Invia informazioni al bluetooth]===//
void stringToBlue(String str) {
  for (int i = 0; i < str.length(); i++)
  {
    EEBlue.write(str[i]);
  }
}

//===[Task per spegnere il display]===//
Task displayOffTask(0, TASK_ONCE, []() {
  digitalWrite(LCD_ANODE,LOW);
  displayStatus = LOW;
});

//===[Task per reimpostare le informazioni presenti prima delle transizioni]===//
Task restoreDisplayInfoTask(0,TASK_ONCE, []() {
  lcd.setCursor(0,0);
  lcd.write(infoScreen1);
  lcd.setCursor(0,1);
  lcd.write(infoScreen2);
});

//===[Task per chiudere il coperchio]===//
Task closeTask(0, TASK_ONCE, []() {
  //close();
  Serial.println("CLOSE TASK CALLED");
  slowClose();
  restoreInfoScreen();
});

//===[Task per aprire il coperchio]===//
Task openTask(0, TASK_ONCE, []() {
  Serial.println("OPEN TASK CALLED");
  open();
  restoreInfoScreen();
});


/*
STATI APERTURA:
0 => chiuso
1 => aperto
2 => in apertura
3 => in chiusura
*/
int openState = 0;



//===[TASK CHE CONTROLLA UNA VOLTA AL MINUTO SE E' L'ORARIO DI APRIRE IL COPERCHIO]===//
Task checkTimeTask(59000, TASK_FOREVER, []() {
  Serial.println(hour() + ":" + String(minute()) +" == " + orario_apertura.hours + ":" + orario_apertura.minutes);
  if(openState == 0 && hour() == orario_apertura.hours && minute() == orario_apertura.minutes){
    Serial.println("opening");
    open();
  }
});

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

//===[Legge se sono disponibili dati dalla seriale del bluetooth, e in tal caso esegue i comandi ricevuti]===//
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
  
  //while (Serial.available()) EEBlue.write(Serial.read()); //E' possibile inviare i comandi anche da seriale usb nel caso non si abbia a disposizione l'app
});

//===[funzione chiamata al rilascio del pulsante del display]===//
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

//===[funzione chiamata al rilascio del pulsante del coperchio]===//
long lastTimeBtnTogglePressed = 0;
void btnTogglePressed() {
  Serial.println("BTN CLOSE PRESSED");
  Serial.println(openState);
  if(millis() - lastTimeBtnTogglePressed > 1000) {
    Serial.println("executing");
    lastTimeBtnTogglePressed = millis();
    if(openState == 1) {
      lcd.setCursor(0,0);
      lcd.write("    CHIUSURA    ");
      lcd.setCursor(0,1);
      lcd.write("    IN CORSO    ");
      //don't close here because of interrupt short amount of time dedicated!!!!!!!!!
      openState = 3;
      r1.addTask(closeTask);
      closeTask.restartDelayed(1 * TASK_SECOND); //use this instead of closeTask.enableDelayed(1 * TASK_SECOND);
      
    } else if(openState == 0) {
      lcd.setCursor(0,0);
      lcd.write("    APERTURA    ");
      lcd.setCursor(0,1);
      lcd.write("    IN CORSO    ");
      openState = 2;
      r1.addTask(openTask);
      openTask.restartDelayed(1 * TASK_SECOND);
    }
  }
}
void restoreInfoScreen() {
  r1.addTask(restoreDisplayInfoTask);
  restoreDisplayInfoTask.restart();
}
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
  attachInterrupt(digitalPinToInterrupt(DISPLAY_BTN), btnDisplayPressed, FALLING); //CON FALLING invece che RISING(+ time check) evito effetto bounce sul rilascio perché l'interrupt è chiamato solo al rilascio e non alla pressione
  attachInterrupt(digitalPinToInterrupt(CLOSE_BTN), btnTogglePressed, FALLING);
  
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
  //slowClose();
}
void loop()
{
  r1.execute();
}

//===[sistema di chiusura senza feedback]===//
void close() {
  openState = 3;
  int i = 0;
  while(i < 2038/4) {
    nextStep();
    delay(4);
    i++;
  }
  openState = 0;
}

//===[sistema di chiusura con feedback]===//
void slowClose() {
  openState = 3;
  boolean touched = false;
  int divisor = 256;
  
  Serial.println("Trying to close");
  while(true) {
    int i = 0;
    //Serial.println("PRIMO CICLO");
    while(i < 2038/divisor) {
      //Serial.println("SECONDO CICLO");
      nextStep();
      Serial.println(digitalRead(CLOSE_SWITCH));
      if(digitalRead(CLOSE_SWITCH)) { //controllo che l'interrutore sia chiuso (coperchio)
        touched = true;
        divisor = 2038;
        break;
      }
      delay(10);
      i++;
    }
    if(touched) {
      Serial.println("TOUCHEDDDD!");
      openState = 0;
      break;
    }
  }
}
void open() {
  openState = 2;
  int i = 0;
  while(i < 2038/4) {
    previousStep();
    delay(4);
    i++;
  }
  openState = 1;
}
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
