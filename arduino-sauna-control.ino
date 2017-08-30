//arduinosaunacontrol
//Sauna Control to replace broken proprietary control system
//Author: formerredbeard
//Version: .3

byte maxTemp = 119;
byte minTemp = 85;
byte lastTemp;
byte TempSet;
byte opState = 0;
String setModeText;
byte setModeLevel = 0;
float tempC;
float tempF;
int heater1Pin = 10;
int heater2Pin = 11;
int overheadLightPin = 12;
char displayUnit[2] = { 'F', 'm'};
byte displayItem[1];
String displayPrefix[2] = { "Temp: ", "Time: "};
String displayOpMode[2] = { "     ", "OFF"};
byte lastTime;
byte TimeSet;
byte btnPressed;
String alarmReason;
unsigned long minuteTimer = 60000;
unsigned long lastMinute;
byte blinkLightCounter = 0;
String lineDisplay;
byte s;
byte len;
byte TempLow = 0;

//Store values in EEPROM
#include <EEPROM.h>
byte LastTempAddr = 0;
byte LastTimeAddr = 1;

//Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
//Temperature Probe connected to Pin 2
#define ONE_WIRE_BUS 2  
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//DeviceAddress SensorAddr = { 0x28, 0xFF, 0x55, 0x8E, 0x03, 0x15, 0x02, 0x88 };
DeviceAddress SensorAddr;

//LCD Screen - RobotDyn
//#include <LCDKeypad.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// define some values used by the panel    buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnONOFF  0
#define btnUP     1
#define btnDOWN   2
#define btnMODE   3
#define btnLIGHT  4
#define btnNONE   5

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // W e make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
/* if (adc_key_in < 50)   return btnONOFF;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnMODE; 
 if (adc_key_in < 850)  return btnLIGHT;  
*/
 // For V1.0 comment the other threshold and use the one below:
 if (adc_key_in < 50)   return btnONOFF;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnMODE; 
 if (adc_key_in < 790)  return btnLIGHT;   

 return btnNONE;  // when all others fail, return this...
}

void printLine(int LineNum) {
//  Serial.println(LineNum);
  lineDisplay =displayPrefix[LineNum] + String(displayItem[LineNum]) + displayUnit[LineNum] + setModeText + displayOpMode[LineNum];
  len=lineDisplay.length();
  for(s=0; s<16-len; s++) lineDisplay+=" ";
  lcd.setCursor(0,LineNum);
  lcd.print(lineDisplay);
  Serial.println(lineDisplay);
}

void setTemp(int incTemp){
  if ((TempSet > minTemp) and (TempSet < maxTemp)) {
    TempSet=TempSet + incTemp;
    displayItem[0]=TempSet;
    TempLow = TempSet - 2;
  }
}

void setTime(int incTime){
  if ((TimeSet > 1) and (TimeSet < 99)) {
    TimeSet=TimeSet + incTime;
    displayItem[1]=TimeSet;  
  }
}

void turnOff(){
    displayOpMode[0]="    ";
    displayOpMode[1]="OFF";
    displayItem[0]=TempSet;
    displayItem[1]=TimeSet;
    heaterCtrl(LOW);         
    printLine(1);
    printLine(0);
    opState = 0; 

}

void heaterCtrl(int hctrl){
   digitalWrite(heater1Pin, hctrl);
   digitalWrite(heater2Pin, hctrl);
   if (hctrl) {
    displayOpMode[0] = "H  On";  
   }
   else {
    displayOpMode[0] = "H Off";
   }
}

void OnState(){
    //check buttons
    btnPressed = read_LCD_buttons();
    switch (btnPressed) {
      case btnONOFF:
         turnOff();
         delay(10000);
         return;
         break;
      case btnLIGHT:
         digitalWrite(overheadLightPin, !digitalRead(overheadLightPin));
         break;      
    }
    //check timer
    if ((millis() - lastMinute) > minuteTimer) {
        lastMinute=millis();
        if (--displayItem[1] < 1){
          turnOff();
          //Sound End Buzzer
          //Blink Light
          while (blinkLightCounter < 14) {   
                blinkLightCounter++;
                digitalWrite(overheadLightPin, !digitalRead(overheadLightPin));
                delay(750);
          }
          blinkLightCounter = 0;
          return; 
        }
    }
    //check temp and take appropriate actions with the heaters
    tempC = sensors.getTempC(SensorAddr);
    tempF = DallasTemperature::toFahrenheit(tempC);
    Serial.println(tempF);

    displayItem[0]=tempF;
    if ((tempF > maxTemp) or (tempF < -130)){
      opState = 3;  
    } 
    if (tempF > TempSet) {
      //turn off heaters
      heaterCtrl(LOW);
    }
    if (tempF < TempLow) {
      //turn on heaters
      heaterCtrl(HIGH);
    }
    
    //Update Display 
    printLine(0);
    printLine(1);

}

void OffState(){
    //check buttons
    btnPressed = read_LCD_buttons();
    switch (btnPressed) {
      case btnONOFF:
         opState = 1; //Change to OnState
         delay(10000); //just till I learn how to debounce the buttons
         displayOpMode[1]=" ON";
         printLine(1);
         if (lastTemp!=TempSet) {
          lastTemp = TempSet;
          EEPROM.write(LastTempAddr,TempSet);
         }
         if (lastTime!=TimeSet){
          lastTime = TimeSet;
          EEPROM.write(LastTimeAddr,lastTime);
         }
         //Set time for timer to start
 
         printLine(0);
         printLine(1);    
         lastMinute=millis();
         break;
      case btnLIGHT:
         digitalWrite(overheadLightPin, !digitalRead(overheadLightPin));
         break;      
       case btnMODE:
         opState = 2; //Change to SetState
         setModeText = " ";
         setModeLevel = 0;
         printLine(setModeLevel);
         break;
    }

}

void SetState(){
    setModeText = "<";
    printLine(setModeLevel);
    delay(200);
    //check buttons
    setModeText = " ";
    btnPressed = read_LCD_buttons();
    switch (btnPressed) {
      case btnUP:
         if (setModeLevel) {
         setTemp(1);           
         }
         else {
         setTime(1); 
         }
      case btnDOWN:
         if (setModeLevel) {
         setTemp(-1);           
         }
         else {
         setTime(-1); 
         }      
       case btnMODE:
         if (setModeLevel < 1){
          setModeText = " ";
          printLine(setModeLevel);
          setModeLevel=1;
         }
         else{
          setModeLevel=0;
          setModeText = " ";
          opState=0;
          printLine(0);
          printLine(1);
          return;
         }
    }
     
 
    //set the temp or time
 
    printLine(setModeLevel);
}

void alarmState(){
  heaterCtrl(LOW);
  //Sound Alarm Bell
  lcd.setCursor(0,0);
  lcd.print("ALARM: "+String(tempF)+"F");
  lcd.setCursor(0,1);
  lcd.print("  REMOVE POWER  ");
  //Blink light first few times
  if (blinkLightCounter < 20) {   
    blinkLightCounter++;
    digitalWrite(overheadLightPin, HIGH);
    delay(300);
    digitalWrite(overheadLightPin, LOW);
  }
  delay(700);
  lcd.setCursor(0,0);
  lcd.print("                ");
  //Blink Light

}

void setup() {
  Serial.begin(9600);
  Serial.println("Sauna Test");
  lcd.begin(16, 2);
  lcd.clear();
  // put your setup code here, to run once:
  lastTemp = EEPROM.read(LastTempAddr);
  if(lastTemp < 20 || lastTemp > 120) {
    lastTemp = 97;
  }
  TempSet = lastTemp;
  TempLow = TempSet - 2;
  displayItem[0] = TempSet;
  Serial.println("lastTemp TempSet TempLow");
  Serial.println(lastTemp);
  Serial.println(TempSet);
  Serial.println(TempLow);
    
  TimeSet = EEPROM.read(LastTimeAddr);
  if(lastTime < 2 || lastTime > 99) {
    lastTime = 2;
  }
  TimeSet = lastTime;

  displayItem[1] = TimeSet;

  sensors.begin();
  
  setModeText = " ";
  
  Serial.println("TimeSet lastTime");
  Serial.println(TimeSet);
  Serial.println(lastTime);

  printLine(0);
  printLine(1);
  
  pinMode(heater1Pin, OUTPUT);
  pinMode(heater2Pin, OUTPUT);
  pinMode(overheadLightPin, OUTPUT);


}

void loop() {
  // put your main code here, to run repeatedly:
  switch (opState){
    case 0:
      OffState();
      break;
    case 1:
      OnState();
      break;
    case 2:
      SetState();
      break;
    case 3:
      alarmState();
      break;
    default:
      //Force to off state
      opState = 0;
      //PUT CODE HERE TO INDICATE OFF
      OnState();
    break;
  }
      
}
