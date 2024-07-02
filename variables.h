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


// Variable declaration for time schedule in blynk
extern int blynkHr, blynkMin, blynkUpLCD1, blynkDownLCD1, blynkScrollCtr1, blynkEditInput;
extern bool blynkIsPM;

// Variable declaration for weight in blynk
extern double blynkFeedWeight;
extern int blynkUpLCD2, blynkDownLCD2, blynkScrollCtr2, blynkFeedType, blynkWeekDur, blynkNumOfDispense;

// Variable declaration for custom schedule in blynk
extern int blynkUpLCD3, blynkDownLCD3, blynkScrollCtr3, blynkMonth, blynkDate, blynkCustomHr, blynkCustomMin;
extern bool blynkCustomIsPM;
extern double blynkCustomFeedWeight;

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