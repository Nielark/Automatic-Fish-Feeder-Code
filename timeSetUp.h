#ifndef TIMESETUP_H
#define TIMESETUP_H

#include "variables.h"

void displayTime(DateTime currentTime) {
  // Display the date
  lcd.setCursor(0, 0);
  lcd.print("DATE:");
  lcd.setCursor(6, 0);
  if(currentTime.month() < 10) {
    lcd.print("0");
  }
  lcd.print(currentTime.month(), DEC);
  lcd.print("/");

  if(currentTime.day() < 10) {
    lcd.print("0");
  }
  lcd.print(currentTime.day(), DEC);
  lcd.print("/");
  lcd.print(currentTime.year(), DEC);

  // Display the day of the week
  lcd.print(" ");
  lcd.print(daysOfTheWeek[currentTime.dayOfTheWeek()]);

  // Display the time;
  curYear = currentTime.year();
  curHour = currentTime.twelveHour();   // Convert the time into 12 hour format
  // lcd.setCursor(0, 3);
  // lcd.print(currentTime.hour());     // For 24 hour format

  lcd.setCursor(0, 1);
  lcd.print("TIME:");

  // Adds Leading zero to hour if it is a single digit
  lcd.setCursor(6, 1);
  if(curHour < 10) {
    lcd.print("0");
  }
  lcd.print(curHour, DEC);
  lcd.print(":");

  // Adds Leading zero to minute if it is a single digit
  lcd.setCursor(9, 1);
  if(currentTime.minute() < 10) {
    lcd.print("0");
  }
  lcd.print(currentTime.minute(), DEC);
  lcd.print(":");

  // Adds Leading zero to seconds if it is a single digit
  lcd.setCursor(12, 1);
  if(currentTime.second() < 10) {
    lcd.print("0");
  }
  lcd.print(currentTime.second(), DEC);

  // Checks and display if the time is AM or PM
  lcd.setCursor(15, 1);
  if(currentTime.isPM()) {
    lcd.print("PM");
  }
  else {
    lcd.print("AM");
  }

  displayPrevTime = displayCurTime;
}

void setTime() {
  int morningOrAfternoon;

  // Set the date
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Month:");
  int month = getInput(13, 0, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Day Of Month:");
  int day = getInput(0, 1, 2, 1, 31);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Year:");
  int year = getInput(12, 0, 4, 1, 9000);

  // Set the time
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Hours:");
  int hour = getInput(13, 0, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Minutes:");
  int minute = getInput(15, 0, 2, 0, 59);

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice:");
  morningOrAfternoon = getInput(14, 2, 1, 1, 2);

  // Convert the input time into 24 hour format
  if(morningOrAfternoon == 1) {
    if(hour == 12) {
      hour += 12;
    }
    else {
      hour +=  0;
    }
  }
  else {
    if(hour == 12) {
      hour += 0;
    }
    else {
       hour += 12;
    }
  }

  // Uncomment this code if you also want to set the seconds
  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("Enter Seconds: ");
  // int second = getInput();

  // Set the date and time from the input values
  rtc.adjust(DateTime(year, month, day, hour, minute));
}

#endif