#ifndef CUSTOMSCHEDULE_H
#define CUSTOMSCHEDULE_H

#include "variables.h"
#include "eeprom1.h"

void editTemporarySched(int editInput);
void deleteTemporarySched(int delInput);

void viewTemporarySched() {
  top:
  bool editSchedFlag = false;
  int editInput, delInput, doubleDigit = 0, scrollCtr = 0;

  lcd.clear();

  // Prompt if schedule list is empty
  if(tempSchedCtr == 0 && deleteFlag == false){
    deleteFlag = false;
    lcd.setCursor(0, 1);
    lcd.print(" Schedule is Empty");
    delay(2000);
    return;
  }
  else if(tempSchedCtr == 0 && deleteFlag == true){
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
    if(tempSched[scrollCtr].isActivated) {
      // Adds leading zero for time less than 10 and convert it into string
      String date = months[tempSched[scrollCtr].month - 1] + "/" + String(tempSched[scrollCtr].day) + "/" + String(curYear);
      String fHour = (tempSched[scrollCtr].hour < 10 ? "0" : "") + String(tempSched[scrollCtr].hour);
      String fMinute = (tempSched[scrollCtr].minute < 10 ? "0" : "") + String(tempSched[scrollCtr].minute);
      String AMorPM = (tempSched[scrollCtr].isPM ? "PM" : "AM");
      String feedWeight = String(tempSched[scrollCtr].feedWeight) + " Kg";

      lcd.setCursor(0, 0);
      lcd.print("TEMPORARY SCHED " + String(scrollCtr + 1));
      lcd.setCursor(0, 1);
      lcd.print("Date: " + date);
      lcd.setCursor(0, 2);
      lcd.print("Time: " + fHour + ":" + fMinute + " " + AMorPM);
      lcd.setCursor(0, 3);
      lcd.print("Feed Weight: " + feedWeight);
    }

    char exitKey = customKeypad.getKey();

    // Press 'A' to scroll the list upward
    if (exitKey == 'A') { // Scroll up
      lcd.clear();
      if (scrollCtr > 0) {
        scrollCtr--;
      }
    } 
    // Press 'D' to scroll the list downward
    else if (exitKey == 'D') { // Scroll down
      lcd.clear();
      if (scrollCtr < (tempSchedCtr - 1)) {
        scrollCtr++;
      }
    }
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
    else if(exitKey == 'C' && tempSchedCtr >= 10 && doubleDigit < tempSchedCtr ){
      doubleDigit += 10;
    }
    else if(exitKey == 'C' && tempSchedCtr >= 10 && doubleDigit >= tempSchedCtr ){
      doubleDigit = 0;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= tempSchedCtr && deleteFlag == false) {
      // Select the number to edit the sched
      editSchedFlag = true;
      editInput = exitKey - '0'; // Convert char to int
      editInput += doubleDigit;

      if(editInput > 0 && editInput <= tempSchedCtr) {
        editTemporarySched(editInput);
      }
      else{
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Data not found");
        delay(2000);
      }

      goto top;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= tempSchedCtr && deleteFlag == true) {
      // Select the number to delete the sched
      delInput = exitKey - '0'; // Convert char to int
      delInput += doubleDigit;

      if(delInput <= tempSchedCtr) {
        deleteTemporarySched(delInput);
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

void addTemporarySched() {
  int morningOrAfternoon;
  bool isAlreadySet = false;

  if(tempSchedCtr < 10){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));
    
    lcd.setCursor(0, 1);
    lcd.print("Enter Month:");
    tempSched[tempSchedCtr].month = getInput(13, 1, 2, 1, 12);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));
    
    lcd.setCursor(0, 1);
    lcd.print("Enter Day:");
    tempSched[tempSchedCtr].day = getInput(11, 1, 2, 1, 31);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));
    
    lcd.setCursor(0, 1);
    lcd.print("Enter Hour:");
    tempSched[tempSchedCtr].hour = getInput(12, 1, 2, 1, 12);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));

    lcd.setCursor(0, 1);
    lcd.print("Enter Minute:");
    tempSched[tempSchedCtr].minute = getInput(14, 1, 2, 0, 59);

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
      tempSched[tempSchedCtr].isPM = false;
    }
    else {
      tempSched[tempSchedCtr].isPM = true;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Feed Weight:");
    tempSched[tempSchedCtr].feedWeight = getDecimalInput(0, 1, 4, 5);
    
    tempSched[tempSchedCtr].isActivated = true;
    tempSched[tempSchedCtr].isFinished = false;

    if(tempSchedCtr == 0){
      tempSchedCtr++;
      EEPROM.update(1, tempSchedCtr);

      updateTemporarySchedEEPROM();
      // Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");
      addSuccessfulMsg();
      return;
    }

    // Checks if the input time is already set and in the list
    for(int i = 0; i < tempSchedCtr; i++){
      if(tempSched[tempSchedCtr].month == tempSched[i].month &&
        tempSched[tempSchedCtr].day == tempSched[i].day &&
        tempSched[tempSchedCtr].hour == tempSched[i].hour &&
        tempSched[tempSchedCtr].minute == tempSched[i].minute &&
        tempSched[tempSchedCtr].isPM == tempSched[i].isPM
      ){
        isAlreadySet = true;
        break;
      }
    }

    if(!isAlreadySet) {
      tempSchedCtr++;
      EEPROM.update(1, tempSchedCtr);

      updateTemporarySchedEEPROM();
      // Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");
      addSuccessfulMsg();
    }
    else{
      tempSched[tempSchedCtr] = {};

      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("The same time is");
      lcd.setCursor(4, 2);
      lcd.print("already set");
      delay(3000);
    }
  }

  // All ten schedules have been set, display a message
  if(tempSchedCtr >= 10){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All 10 schedules set");
    delay(3000);
  }
}

void editTemporarySched(int editInput) {
  int morningOrAfternoon;
  bool isAlreadySet = false;
  editInput = editInput - 1;

  String date = months[tempSched[editInput].month - 1] + "/" + String(tempSched[editInput].day) + "/" + String(curYear);
  String fHour = (tempSched[editInput].hour < 10 ? "0" : "") + String(tempSched[editInput].hour);
  String fMinute = (tempSched[editInput].minute < 10 ? "0" : "") + String(tempSched[editInput].minute);
  String AMorPM = (tempSched[editInput].isPM ? "PM" : "AM");
  String feedWeight = String(tempSched[editInput].feedWeight) + " Kg";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  lcd.print("Date: " + date);

  lcd.setCursor(0, 2);
  lcd.print("Enter Month:");
  int checkMonth = getInput(13, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  date = months[checkMonth - 1] + "/" + String(tempSched[editInput].day) + "/" + String(curYear);
  lcd.print("Date: " + date);

  lcd.setCursor(0, 2);
  lcd.print("Enter Day:");
  int checkDay = getInput(11, 2, 2, 1, 31);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  date = months[checkMonth - 1] + "/" + String(checkDay) + "/" + String(curYear);
  lcd.print("Date: " + date);


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  lcd.print(String(editInput + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);
  
  lcd.setCursor(0, 2);
  lcd.print("Enter Hour:");
  int checkHour = getInput(12, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  fHour = (checkHour < 10 ? "0" : "") + String(checkHour);
  lcd.print(String(editInput + 1) + ") " + checkHour + ":" + fMinute + " " + AMorPM);

  lcd.setCursor(0, 2);
  lcd.print("Enter Minute:");
  int checkMinute = getInput(14, 2, 2, 0, 59);

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

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  lcd.print("Feed Weight: " + feedWeight);

  lcd.setCursor(0, 2);
  lcd.print("Enter Feed Weight:");
  double checkFeedWeight = getDecimalInput(0, 3, 4, 5);

  bool checkIsActivated = true;
  bool checkIsFinished = false;

  if(tempSchedCtr == 0){
    tempSched[editInput] = {checkMonth, checkDay, checkHour, checkMinute, checkIsPM, checkFeedWeight, checkIsActivated, checkIsFinished};

    updateTemporarySchedEEPROM();
    editSuccessfulMsg();
    return;
  }

  // Checks if the input time is already set and in the list
  for(int i = 0; i < tempSchedCtr; i++){
    if(checkMonth == tempSched[i].month &&
      checkDay == tempSched[i].day &&
      checkHour == tempSched[i].hour &&
      checkMinute == tempSched[i].minute &&
      checkIsPM == tempSched[i].isPM
    ){
      isAlreadySet = true;
      break;
    }
  }

  if(!isAlreadySet) {
    tempSched[editInput] = {checkMonth, checkDay, checkHour, checkMinute, checkIsPM, checkFeedWeight, checkIsActivated, checkIsFinished};
    
    updateTemporarySchedEEPROM();
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

void deleteTemporarySched(int delInput) {
  if(delInput == 0){
    schedReturnFlag = true;
    //deleteFlag = true;
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

    for(int i = delInput; i < tempSchedCtr; i++){
      if(i == 9){
        tempSched[i] = {0, 0, 0, 0, 0, 0, 0, 0};
      }
      else {
        tempSched[i] = tempSched[i + 1];
      }

      updateTemporarySchedEEPROM();
    }

    tempSchedCtr--;   // Decrement the number of schedule
    EEPROM.update(1, tempSchedCtr);
    // Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");
    //deleteFlag = false;
    deleteSuccessfulMsg();
  }
}

#endif