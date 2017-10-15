//arduinosaunacontrol
//Sauna Control to replace broken proprietary control system
//Author: formerredbeard
//Version: .5

//Temperature Variables and Constants
byte maxTemp = 119;
byte minTemp = 85;
int lowAlarmTemp = 0;
byte highAlarmTemp = 120;
byte TempLow = 0;
byte lastTemp;
byte TempSet;
float tempC;
float tempF;

//Time and Timer Variables
byte lastTime;
byte TimeSet;
unsigned long minuteTimer = 60000;
unsigned long lastMinute;

//State of operation Variables
byte opState = 0;
char setModeText[2] = { ' ', ' '};
byte setModeLevel = 0;
String displayOpMode[2] = { "     ", "OFF"};

//Pins for Output
byte heater1Pin = 10;
byte heater2Pin = 11;
byte overheadLightPin = 12;
byte buzzerPin = 3;

//Variables for EEPROM
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

//Display and Button Variables
//LCD Screen - RobotDyn
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
char displayUnit[2] = { 'F', 'm'};
byte displayItem[2];
String displayPrefix[2] = { "Temp: ", "Time: "};
byte btnPressed;
byte blinkLightCounter = 0;
String lineDisplay;
byte s;
byte len;
long lastDebounceTime = 0;   // the last time the output pin was toggled
byte debounceDelay = 15;     // Adjust for jitter (if you get a false reading increase this number)
byte lastButtonState;    // the previous reading from the input pin
byte pressedBtn = 0;

// ---------------------------------------------------
//Start of Function Definitions

// read the buttons
int read_LCD_buttons(){
 adc_key_in = analogRead(0);      // read the value from the sensor 

 //my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 // if (adc_key_in > 1000) return btnNONE; // W e make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
/* if (adc_key_in < 50)   return btnONOFF;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnMODE; 
 if (adc_key_in < 850)  return btnLIGHT;  
*/
 byte tmpButtonState = btnNONE;  // when all others fail, tmpButtonState = ...
 if (adc_key_in < 790)  tmpButtonState = btnLIGHT; 
 if (adc_key_in < 555)  tmpButtonState = btnMODE; 
 if (adc_key_in < 380)  tmpButtonState = btnDOWN; 
 if (adc_key_in < 195)  tmpButtonState = btnUP; 
 if (adc_key_in < 50)   tmpButtonState = btnONOFF;  
  
 //Serial.print("tmpButtonState");
 //Serial.println(tmpButtonState);
 
    // If the switch changed (due to noise OR pressing)
    if (tmpButtonState != lastButtonState) {
        // reset the debouncing timer
        lastDebounceTime = millis();
        lastButtonState = tmpButtonState;
        pressedBtn = 1;
    }
  
   if (((millis() - lastDebounceTime) > debounceDelay) and (pressedBtn > 0)) {
        // Assume this press is legitimate, so set it as btnPressed
        pressedBtn = 0;
        return tmpButtonState;
    }
 return btnNONE;  
}

void updateDisplay() {
//  Serial.println(LineNum);
  byte LineNum;
  for (LineNum=0; LineNum<2; LineNum++){
    lineDisplay =displayPrefix[LineNum] + String(displayItem[LineNum]) + displayUnit[LineNum] + setModeText[LineNum] + displayOpMode[LineNum];
    len=lineDisplay.length();
    for(s=0; s<16-len; s++) lineDisplay+=" ";
    lcd.setCursor(0,LineNum);
    lcd.print(lineDisplay);
//    Serial.println(lineDisplay);
  } 
}

void setTemp(int incTemp){
  TempSet=TempSet + incTemp;
  if (TempSet < minTemp) TempSet=minTemp;
  if (TempSet > maxTemp) TempSet=maxTemp;
  displayItem[0]=TempSet;
  TempLow = TempSet - 2;
}

void setTime(int incTime){
  TimeSet=TimeSet + incTime;
  if (TimeSet < 1) TimeSet=1;
  if (TimeSet > 99) TimeSet=99;
  displayItem[1]=TimeSet;  
}

void turnOff(){
    displayOpMode[0]="    ";
    displayOpMode[1]="OFF";
    displayItem[0]=TempSet;
    displayItem[1]=TimeSet;
    heaterCtrl(LOW);         
    updateDisplay();
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
//    if (btnPressed != btnNONE) {
      switch (btnPressed) {
        case btnONOFF:
         turnOff();
         delay(3000);  //until I get Debouncing Buttons fixed. After that likely change to 3000
         return;
         break;
        case btnLIGHT:
         digitalWrite(overheadLightPin, !digitalRead(overheadLightPin));
         Serial.print("Light State: ");
         Serial.println(digitalRead(overheadLightPin));

         break;      
//      }
    }  
    //check timer
    if ((millis() - lastMinute) > minuteTimer) {
        lastMinute=millis();
        if (--displayItem[1] < 1){
          turnOff();
          //Sound End Buzzer and Blink Light
          while (blinkLightCounter < 14) {   
                blinkLightCounter++;
                digitalWrite(overheadLightPin, !digitalRead(overheadLightPin));
                tone(buzzerPin, 1000);
                delay(400);
                noTone(buzzerPin);
                delay (400);
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
    if ((tempF > highAlarmTemp) or (tempF < lowAlarmTemp)){
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
    updateDisplay();

}

void OffState(){
    //check buttons
    btnPressed = read_LCD_buttons();
    switch (btnPressed) {
      case btnONOFF:
         opState = 1; //Change to OnState
         //delay(3000); //just till I learn how to debounce the buttons
         displayOpMode[1]=" ON";
         updateDisplay();
         if (lastTemp!=TempSet) {
          lastTemp = TempSet;
          EEPROM.write(LastTempAddr,TempSet);
         }
         if (lastTime!=TimeSet){
          lastTime = TimeSet;
          EEPROM.write(LastTimeAddr,lastTime);
         }
         //Set time for timer to start
 
         updateDisplay();    
         lastMinute=millis();
         break;
      case btnLIGHT:
         digitalWrite(overheadLightPin, !digitalRead(overheadLightPin));
         Serial.print("Light State: ");
         Serial.println(digitalRead(overheadLightPin));
         break;      
      case btnMODE:
         opState = 2; //Change to SetState
         setModeLevel = 0;
         displayOpMode[1]="Settings";
         updateDisplay();
         break;
    }

}

void SetState(){
    updateDisplay();
    setModeText[setModeLevel] = '<';
    updateDisplay();
    //check buttons
    btnPressed = read_LCD_buttons();
    switch (btnPressed) {
      case btnUP:
         if (setModeLevel < 1) {
         setTemp(1);           
         }
         else {
         setTime(1); 
         }
         break;
      case btnDOWN:
         if (setModeLevel < 1) {
         setTemp(-1);           
         }
         else {
         setTime(-1); 
         }
         break;      
       case btnMODE:
         if (setModeLevel < 1){
          setModeText[setModeLevel] = ' ';
          updateDisplay();
          setModeLevel=1;
 
         }
         else{
          setModeText[setModeLevel] = ' ';
          setModeLevel=0;
          opState=0;
          displayOpMode[1]="OFF";
          updateDisplay();
         }
         break;
    }
     
    updateDisplay();
}

void alarmState(){
  heaterCtrl(LOW);
  //Sound Alarm Bell
  lcd.setCursor(0,0);
  lcd.print("ALARM: "+String(tempF)+"F");
  lcd.setCursor(0,1);
  lcd.print("  REMOVE POWER  ");
  //Blink light first few times
  if (blinkLightCounter++ < 20) {   
    digitalWrite(overheadLightPin, HIGH);
    tone(buzzerPin, 1500);
    delay(300);
    digitalWrite(overheadLightPin, LOW);
  }
  noTone(buzzerPin);
  delay(700);
  lcd.setCursor(0,0);
  lcd.print("                ");
//  delay(50000);  //added for debug troubleshooting

}


// ---------------------------------------------------
//Start of Builtin Setup and Loop function Definitions


void setup() {

  Serial.begin(9600);
  Serial.println("Sauna Test");
  opState = 0;
  lcd.begin(16, 2);
  lcd.clear();
  lastTemp = EEPROM.read(LastTempAddr);
  if(lastTemp < 20 || lastTemp > 120) {
    lastTemp = 92;  //Starting temp used for testing as I could put hand on probe to warm.
  }
  TempSet = lastTemp;
  TempLow = TempSet - 2;
  displayItem[0] = TempSet;
  Serial.println("lastTemp TempSet TempLow");
  Serial.println(lastTemp);
  Serial.println(TempSet);
  Serial.println(TempLow);
    
  lastTime = EEPROM.read(LastTimeAddr);
  if(lastTime < 2 || lastTime > 99) {
    lastTime = 20;
  }
  TimeSet = lastTime;

  displayItem[1] = TimeSet;

  sensors.begin();
  if (!sensors.getAddress(SensorAddr, 0)) opState = 3;
  sensors.setResolution(SensorAddr, 9);
  Serial.println("TimeSet");
  Serial.println(TimeSet);

  updateDisplay();
  
  pinMode(heater1Pin, OUTPUT);
  pinMode(heater2Pin, OUTPUT);
  pinMode(overheadLightPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  lastButtonState=btnNONE;


}

void loop() {
  Serial.println(opState);
  sensors.requestTemperatures(); // Send the command to get temperatures

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
      turnOff();
      break;
  }
      
}

//End of Program
