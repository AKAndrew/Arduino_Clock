#include "RTClib.h" // RTC library/driver
#include <LiquidCrystal.h> // LCD library/driver
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h> 
using namespace Adafruit_LittleFS_Namespace;

#define joyX A3
#define joyY A4
const int sw = 2, buzzer=13;
unsigned int lastpress = millis(), lastpos, pos, menuVal=0, submenuVal=0, lastframe=0, swTime=0, timerM, timerS, alarmH, alarmM;
bool buzzing=0;
int valX, valY, posX, posY, menuSel=-1, submenuSel=-1, stopwatchVal=-1, timerVal=-1, alarmVal=-1;
char* menuTitle[4][4] = {{"clock"}, {"alarm","enable","check","disable"}, {"stopwatch","start","check","reset"}, {"timer","set","check","reset"}};
RTC_DS1307 rtc;
const int rs = 12, en = 11, d4 = 5, d5 = 7, d6 = 9, d7 = 10;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // assigning pins for the LCD
File file(InternalFS);

void setup() {
  Serial.begin(115200);  
  lcd.begin(16, 2); // start LCD of size 16 cols 2 rows
  InternalFS.begin();
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    rtc.adjust(DateTime(2022, 4, 1, 11, 41, 0));
  }  
  lcd.setCursor(0,0); // set cursor on col 0 line 0
  lcd.write("HELLO00"); // cursor writes this
  lcd.setCursor(0,1); // set cursor on col 0 line 1
  lcd.write("WORLD!"); // cursor writes this
  pinMode(buzzer, OUTPUT);
  noTone(buzzer); digitalWrite(buzzer,LOW);
  delay(10000);
  pinMode(sw, INPUT_PULLUP);
  pinMode(joyX, INPUT);
  pinMode(joyY, INPUT);
  
}

String getDateStamp(){ // collect Date value
    DateTime now = rtc.now();
    String datestr;
    datestr += now.day(); datestr += '/'; 
    datestr += now.month(); datestr += '/'; 
    datestr += now.year();  
    return datestr;
}

String getTimeStamp(){ // collect Time value
    DateTime now = rtc.now();
    String timestr;
    timestr += now.hour(); timestr += ':'; 
    timestr += now.minute(); timestr += ':';
    timestr += now.second();
    return timestr;
}

void clearLine(bool line){
  lcd.setCursor(0,line);
  lcd.print("                ");
}

void printToLCD(String str0, String str1){ // line1, line2
  if(millis()-lastframe>250){ // remove screen flicker
    unsigned int len, len0 = str0.length(), len1 = str1.length(), index=0;
    len0>len1?len=len0:len=len1; // check which line is longer
    lcd.clear(); // clear LCD
    lcd.setCursor(0,0); lcd.print(str0); // print first line
    lcd.setCursor(0,1); lcd.print(str1); // print 2nd line
    lastframe = millis(); // update last frame time - avoid flicker
    while(len-index>15) // check if print is bigger than 16 chars
	  if(millis()-lastframe>250){
		lcd.scrollDisplayLeft(); // scroll text to left
		lastframe=millis(); // update last frame time
		index++; // update position of first char on display
      }
  }
}

void showMenu(unsigned int &menuIndex, unsigned int &submenuIndex){
  menuIndex=menuIndex%4; // 4 = number of total menus
  switch(menuIndex){ // check selected menu index
    case 0: // clock
      submenuIndex=1; // display nothing - clock has no submenu
    break;
    case 1: case 2: case 3: // alarm, timer, sw
      submenuIndex=submenuIndex%4; // 4 = number of entries per menu
      if(submenuIndex<1) submenuIndex=1; // don't go below 1
      if(submenuIndex>3) submenuIndex=3; // don't go above 3
    break;
  }
  printToLCD(menuTitle[menuIndex][0],menuTitle[menuIndex][submenuIndex]);
}

void enterMenu(unsigned int menuIndex, unsigned int submenuIndex){
  switch(menuIndex){
    case 0: // clock
      switch(submenuIndex){
        default: // no submenu
          printToLCD(getTimeStamp(), getDateStamp());
        break;
      }
    break;
    case 1: // alarm
      switch(submenuIndex){
        case 1: // enable
          if(alarmVal == 0){
            DateTime now = rtc.now();
            alarmH = now.hour(); alarmM = now.minute();
            setAlarm();
          }
          submenuSel = 2;
        break;
        case 2: // check
          if(alarmVal == -1)
            printToLCD(menuTitle[menuIndex][0],"not set");
          else if(alarmVal == 0)
            printToLCD(menuTitle[menuIndex][0],"ALARM");
          else printToLCD((String)menuTitle[menuIndex][0]+" set to", (String)(alarmH)+":"+(String)(alarmM));
        break;
        case 3: // disable
          alarmVal = 0;
          submenuSel = 2;
        break;
      }
    break;
    case 2: // stopwatch
      switch(submenuIndex){
        case 1: // start
          if(stopwatchVal==-1) stopwatchVal=millis();
          submenuSel=2;
        break;
        case 2: // check
          if(stopwatchVal==-1)
            printToLCD(menuTitle[menuIndex][0],"not started");
          else {
            swTime = (millis()-stopwatchVal)/1000; // number of seconds
            if(swTime>59)
              printToLCD(menuTitle[menuIndex][0],(String)(swTime/60)+":"+(String)(swTime%60)+" min:sec");
            else
              printToLCD(menuTitle[menuIndex][0],(String)swTime+" sec");
          }
        break;
        case 3: // reset
          stopwatchVal=-1;
          submenuSel=2;
        break;
      }
    break;
    case 3: // timer
      switch(submenuIndex){
        case 1: // set
          if(timerVal==-1) setTimer();
          submenuSel=2;
        break;
        case 2: // check
          if(timerVal==-1)
            printToLCD(menuTitle[menuIndex][0],"not set");
          else if(timerVal==0)
            printToLCD(menuTitle[menuIndex][0],"ALARM");
          else {
            timerM = (timerVal - millis())/60000;
            timerS = ((timerVal - millis())%60000)/1000;
            if(timerS == 0 && timerM == 0)
              timerVal=0;
            else
              printToLCD((String)menuTitle[menuIndex][0]+" set", (String)timerM+":"+(String)timerS);
          }
        break;
        case 3: // reset
          timerVal=-1;
          submenuSel=2;
        break;
      }
    break;
  }
}

void setAlarm(){
  menuVal = alarmH;
  submenuVal = alarmM;
  while(digitalRead(sw)){
    navigate();
    alarmH = menuVal;
    alarmM = submenuVal;
    printToLCD("set "+(String)menuTitle[menuSel][0],(String)(alarmH)+":"+(String)(alarmM));
  }
  menuVal = 1; submenuVal = 2;
  alarmVal = 1;
}

void setTimer(){
  timerM = 1; timerS = 30;
  menuVal = timerM; submenuVal = timerS;
  while(digitalRead(sw)){
    navigate();
    timerM = menuVal; timerS = submenuVal;
    printToLCD("set "+(String)menuTitle[menuSel][0],(String)timerM+":"+(String)timerS);
    if(!digitalRead(sw)) break;
  }
  timerVal = millis() + timerM*60000 + timerS*1000;
}

void navigate(){
  valX = analogRead(joyX);
  valY = analogRead(joyY);
  posX = map(valX, 0, 1023, -9, 9);
  posY = map(valY, 0, 1023, -9, 9);  
  if(posX == 4 && posY == -9) pos = 1; // up
  else if((posX == 0 || posX == 1) && (posY == 6 || posY == 5)) pos = 2; //right
  else if(posX == 5 && (posY == 0 || posY == 1)) pos = 3; // down
  else if(posX == -9 && posY == 4) pos = 4; // left
  else if(posX == 4 && posY == 4) pos = 0; // center
  if(millis()-lastpress > 500){
    if(pos!=lastpos){ // actioning only when current pos is different from last pos (no spam)
      switch(pos){ // current joystick pos
        case 0: // center
        break;
        case 1: // up
          menuVal++;
        break;
        case 2: // right
          submenuVal++;
        break;
        case 3: // down
          menuVal--;
        break;
        case 4: // left
          submenuVal--;
        break;
        default: // any other - unknown
        break;
      }
      lastpos=pos; lastpress=millis(); // update last pos to current pos
    }
  }
}

void soundAlarm(){
  if(millis()-lastpress>500){
    buzzing=!buzzing;
    lastpress=millis();
  }
  if(buzzing)
    noTone(buzzer); // stop buzzing
  else
    tone(buzzer,1000);
}

void loop() {
  if(timerVal!=-1 && timerVal<millis()){ timerVal=0; menuSel=3; submenuSel=2; }
  if(alarmVal!=-1){
    DateTime now = rtc.now();
    if(alarmH>=now.hour() && alarmM>=now.minute()){ alarmVal=0; menuSel=1; submenuSel=2; }
  }
  if(alarmVal==0 || timerVal==0) soundAlarm();
  navigate();
  while(!digitalRead(sw) && pos==0){ // when the joystick is centered and button actioned
      switch(menuSel){ // check in which menu we are currently
        case -1: // we are in no menu
          menuSel = menuVal; // enter displayed menu
          submenuSel = submenuVal; // and submenu
        break;
        default: // we are in a menu
          menuSel = -1; // exit current menu
          submenuSel = -1; // and submenu
        break;
    }
    lastpress=millis(); // avoid spam
  }
  while(digitalRead(sw)){ // while joystick button is not actioned
    if(menuSel!=-1 && submenuSel!=-1) // we are currently in a menu/submenu
      enterMenu(menuSel, submenuSel); // stay entered in that menu/submenu
    else{ // we are not in a menu
      navigate(); // keep listening for joystick input
      showMenu(menuVal,submenuVal); // display menu/submenu on display
    }
  }
}
