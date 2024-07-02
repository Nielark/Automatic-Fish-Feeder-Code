#ifndef FEEDSCHEDULE_H
#define FEEDSCHEDULE_H

#include "variables.h"
#include "eeprom1.h"

void editFeedSched(int editInput);
void deleteFeedSched(int delInput);

int LCD_ROWS = 4; // Define the number of rows in your LCD

int scrollPosition = 0; // Initialize the scroll position

void viewSched(){
  top:
  bool editSchedFlag = false;
  int editInput, delInput, doubleDigit = 0, scrollCtr = 0;

  lcd.clear();

  // Prompt if schedule list is empty
  if(feedSchedCtr == 0 && deleteFlag == false){
    lcd.setCursor(0, 1);
    lcd.print(" Schedule is Empty");
    delay(2000);
    return;
  }
  else if(feedSchedCtr == 0 && deleteFlag == true){
    lcd.setCursor(0, 1);
    lcd.print("Schedule is Empty");
    lcd.setCursor(0, 2);
    lcd.print("No Data to Delete");
    deleteFlag = false;
    delay(2000);
    return;
  }

  // Display the feeding shedules
  while(true){
    for (int i = scrollPosition; i < min(feedSchedCtr, scrollPosition + LCD_ROWS); i++) {
      if(feedTime[i].isActivated) {
        lcd.setCursor(0, i - scrollPosition);
        // Adds leading zero for time less than 10 and convert it into string
        String fHour = (feedTime[i].hour < 10 ? "0" : "") + String(feedTime[i].hour);
        String fMinute = (feedTime[i].minute < 10 ? "0" : "") + String(feedTime[i].minute);
        String AMorPM = (feedTime[i].isPM ? "PM" : "AM");
        lcd.print(String(i + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);
      }
    }
    
    char exitKey = customKeypad.getKey();

    // Press 'A' to scroll the list upward
    if (exitKey == 'A') { // Scroll up
      lcd.clear();
      if (scrollPosition > 0) {
        scrollPosition--;
      }
    } 
    // Press 'D' to scroll the list downward
    else if (exitKey == 'D') { // Scroll down
      lcd.clear();
      if (scrollPosition < feedSchedCtr - LCD_ROWS) {
        scrollPosition++;
      }
    }
    // Exit and back to menu
    else if(exitKey == 'B' && deleteFlag == false) {
      // Return back (when only viewing the schedule)
      return;
    }
    else if(exitKey == 'B' && deleteFlag == true) {
      // Return back (when deleting was included)
      schedReturnFlag = true;
      deleteFlag = false;
      return;
    }
    else if(exitKey == 'C' && feedSchedCtr >= 10 && doubleDigit < feedSchedCtr ){
      doubleDigit += 10;
    }
    else if(exitKey == 'C' && feedSchedCtr >= 10 && doubleDigit >= feedSchedCtr ){
      doubleDigit = 0;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= feedSchedCtr && deleteFlag == false) {
      // Select the number to edit the sched
      editSchedFlag = true;
      editInput = exitKey - '0'; // Convert char to int
      editInput += doubleDigit;

      if(editInput > 0 && editInput <= feedSchedCtr) {
        editFeedSched(editInput);
      }
      else{
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Data not found");
        delay(2000);
      }

      goto top;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= feedSchedCtr && deleteFlag == true) {
      // Select the number to delete the sched
      delInput = exitKey - '0'; // Convert char to int
      delInput += doubleDigit;

      if(delInput > 0 && delInput <= feedSchedCtr) {
        deleteFeedSched(delInput);
      }
      else{
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Data not found");
        delay(2000);
      }

      goto top;
    }
  }
}

void addFeedSched() {
  int morningOrAfternoon;
  bool isAlreadySet = false;

  if(feedSchedCtr < 10){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(feedSchedCtr + 1));
    
    lcd.setCursor(0, 1);
    lcd.print("Enter Hour:");
    feedTime[feedSchedCtr].hour = getInput(12, 1, 2, 1, 12);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(feedSchedCtr + 1));

    lcd.setCursor(0, 1);
    lcd.print("Enter Minute:");
    feedTime[feedSchedCtr].minute = getInput(14, 1, 2, 0, 59);
    feedTime[feedSchedCtr].isActivated = true;

    // Selection for AM and PM
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[1] - AM");
    lcd.setCursor(0, 1);
    lcd.print("[2] - PM");
    lcd.setCursor(0, 2);
    lcd.print("Enter choice:");
    morningOrAfternoon = getInput(14, 2, 1, 1, 2);

    if(morningOrAfternoon == 1) {
      feedTime[feedSchedCtr].isPM = false;
    }
    else {
      feedTime[feedSchedCtr].isPM = true;
    }

    if(feedSchedCtr == 0){
      feedSchedCtr++;
      EEPROM.update(0, feedSchedCtr);
      Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");

      sortFeedSched();
      addSuccessfulMsg();
      return;
    }

    // Checks if the input time is already set and in the list
    for(int i = 0; i < feedSchedCtr; i++){
      if(feedTime[feedSchedCtr].hour == feedTime[i].hour &&
        feedTime[feedSchedCtr].minute == feedTime[i].minute &&
        feedTime[feedSchedCtr].isPM == feedTime[i].isPM
      ){
        isAlreadySet = true;
        break;
      }
    }

    if(!isAlreadySet) {
      feedSchedCtr++;
      EEPROM.update(0, feedSchedCtr);
      Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");

      sortFeedSched();
      addSuccessfulMsg();
    }
    else{
      feedTime[feedSchedCtr] = {};

      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("The same time is");
      lcd.setCursor(4, 2);
      lcd.print("already set");
      delay(3000);
    }
  }
  
  // All three schedules have been set, display a message
  if(feedSchedCtr >= 10){
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("All 10 schedules set");
    delay(3000);
  }
}

void editFeedSched(int editInput) {
  int morningOrAfternoon;
  bool isAlreadySet = false;
  editInput = editInput - 1;

  String fHour = (feedTime[editInput].hour < 10 ? "0" : "") + String(feedTime[editInput].hour);
  String fMinute = (feedTime[editInput].minute < 10 ? "0" : "") + String(feedTime[editInput].minute);
  String AMorPM = (feedTime[editInput].isPM ? "PM" : "AM");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scheduled Time:");
  lcd.setCursor(0, 1);
  lcd.print(String(editInput + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);
  
  lcd.setCursor(0, 2);
  lcd.print("Enter Hour:");
  int checkHour = getInput(12, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scheduled Time:");
  lcd.setCursor(0, 1);
  fHour = (checkHour < 10 ? "0" : "") + String(checkHour);
  lcd.print(String(editInput + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);

  lcd.setCursor(0, 2);
  lcd.print("Enter Minute:");
  int checkMinute = getInput(14, 2, 2, 0, 59);
  bool checkIsActivated = true;

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice:");
  morningOrAfternoon = getInput(14, 2, 1, 1, 2);

  bool checkIsPM;
  if(morningOrAfternoon == 1) {
    checkIsPM = false;
  }
  else {
    checkIsPM = true;
  }

  if(feedSchedCtr == 0){
    feedTime[editInput] = {checkHour, checkMinute, checkIsPM, checkIsActivated};

    sortFeedSched();
    editSuccessfulMsg();
    return;
  }

  // Checks if the input time is already set and in the list
  for(int i = 0; i < feedSchedCtr; i++){
    if(checkHour == feedTime[i].hour &&
      checkMinute == feedTime[i].minute &&
      checkIsPM == feedTime[i].isPM
    ){
      isAlreadySet = true;
      break;
    }
  }

  if(!isAlreadySet) {
    feedTime[editInput] = {checkHour, checkMinute, checkIsPM, checkIsActivated};
    
    sortFeedSched();
    editSuccessfulMsg();
  }
  else{
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("The same time is");
    lcd.setCursor(4, 2);
    lcd.print("already set");
    delay(3000);
  }
}

void deleteFeedSched(int delInput) {
  if(delInput == 0){
    schedReturnFlag = true;
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Delete sched " + String(delInput));
  lcd.setCursor(0, 1);
  lcd.print("[1]-YES");
  lcd.setCursor(0, 2);
  lcd.print("[2]-CANCEL");
  lcd.setCursor(0, 3);
  lcd.print("Enter your choice:");
  int choice = getInput(19, 3, 1, 1, 2);

  if(choice == 1){
    delInput = delInput - 1; // Subtract one on the number to delete to locate it using array index

    for(int i = delInput; i < feedSchedCtr; i++){
      if(i == 9){
        feedTime[i] = {0, 0, 0, 0};
      }
      else {
        feedTime[i] = feedTime[i + 1];
      }

      updateFeedSchedEEPROM();
    }

    feedSchedCtr--;   // Decrement the number of schedule
    EEPROM.update(0, feedSchedCtr);
    Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");
    deleteSuccessfulMsg();
  }
}

#endif