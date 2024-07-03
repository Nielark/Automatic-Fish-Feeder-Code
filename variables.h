#ifndef VARIABLES_H
#define VARIABLES_H
#include "struct.h"

extern Keypad customKeypad; 
extern LiquidCrystal_I2C lcd;
extern RTC_DS1307 rtc;

extern String months[12];
extern char daysOfTheWeek[7][12];

extern const unsigned long displayTimer;
extern unsigned long displayPrevTime;
extern unsigned long displayCurTime;

extern Time feedTime[];
extern Custom tempSched[];
extern Quantity feedQuantity[];

extern int cm;
extern double DOlevel;
extern int feedSchedCtr, feedWeightCtr, feedDispenseCtr;
extern int tempSchedCtr, tempFeedDispenseCtr;
extern int curHour, curYear;
extern bool deleteFlag, weightDeleteFlag, schedReturnFlag, weightReturnFlag, menuFlag, delMenuFlag;

void addSuccessfulMsg(){
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("Added Successfully");
  delay(800);
}

void editSuccessfulMsg(){
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("Edited Successfully");
  delay(800);
}

void deleteSuccessfulMsg(){
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Deleted Successfully");
  delay(800);
}

#endif