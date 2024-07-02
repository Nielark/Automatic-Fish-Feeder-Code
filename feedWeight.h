#ifndef FEEDWEIGHT_H
#define FEEDWEIGHT_H

#include "variables.h"
#include "eeprom1.h"

void editWeightSched(int editInput);
void deleteFeedWeight(int delInput);

void viewFeedWeight() {
  top:
  bool editWeightFlag = false;
  int editInput, delInput, doubleDigit = 0, scrollCtr = 0;
  String feedType;

  lcd.clear();

  // Promt if the weight list is empty
  if(feedWeightCtr == 0 && !weightDeleteFlag) {
    lcd.setCursor(0, 1);
    lcd.print("Weight list is Empty");
    delay(2000);
    return;
  }
  else if(feedWeightCtr == 0 && weightDeleteFlag) {
    lcd.setCursor(0, 1);
    lcd.print("Weight list is Empty");
    lcd.setCursor(0, 2);
    lcd.print("No Data to Delete");
    weightDeleteFlag = false;
    delMenuFlag = false;
    delay(2000);
    return;
  }

  // Display the weight List
  while(true){
    if(feedQuantity[scrollCtr].feedType == 1) {
      feedType = "Mash";
    }
    else if(feedQuantity[scrollCtr].feedType == 2) {
      feedType = "Starter";
    }
    else if(feedQuantity[scrollCtr].feedType == 3) {
      feedType = "Grower";
    }

    lcd.setCursor(0, 0);
    lcd.print("FEED INFORMATION " + String(scrollCtr + 1));
    lcd.setCursor(0, 1);
    lcd.print(feedType);
    lcd.setCursor(13, 1);
    lcd.print(String(feedQuantity[scrollCtr].feedWeight) + " Kg");
    lcd.setCursor(0, 2);
    lcd.print(String(feedQuantity[scrollCtr].weekDuration) + " week/s, " + String(feedQuantity[scrollCtr].howManyTimesADay) + "x a day");
    lcd.setCursor(0, 3);
    lcd.print("Last " + String(feedQuantity[scrollCtr].totalNumOfTimes) + " dispense");

    char inputKey = customKeypad.getKey();

    // Press 'A' to scroll the list upward
    if (inputKey == 'A') { // Scroll up
      lcd.clear();
      if(scrollCtr > 0) {
        scrollCtr--;
      }
    } 
    // Press 'D' to scroll the list downward
    else if (inputKey == 'D') { // Scroll down
      lcd.clear();
      if(scrollCtr < (feedWeightCtr - 1)) {
        scrollCtr++;
      }
    }
    // Press 'B' to back
    else if(inputKey == 'B') {
      weightReturnFlag = true;
      weightDeleteFlag = false;
      delMenuFlag = false;
      return;
    }
    else if(inputKey == 'C' && feedWeightCtr >= 10 && doubleDigit < feedWeightCtr){
      doubleDigit += 10;
    }
    else if(inputKey == 'C' && feedWeightCtr >= 10 && doubleDigit >= feedWeightCtr ){
      doubleDigit = 0;
    }
    else if(isDigit(inputKey) && (inputKey - '0') <= feedWeightCtr && weightDeleteFlag == false) {
      editWeightFlag = true;
      editInput = inputKey - '0'; // Convert char to int
      editInput += doubleDigit;

      if(editInput > 0 && editInput <= feedWeightCtr) {
        editWeightSched(editInput);
      }
      else{
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Data not found");
        delay(2000);
      }

      goto top;
    }
    else if(isDigit(inputKey) && (inputKey - '0') <= feedWeightCtr && weightDeleteFlag == true) {
      delInput = inputKey - '0'; // Convert char to int
      delInput += doubleDigit;

      if(delInput > 0 && delInput <= feedWeightCtr) {
        deleteFeedWeight(delInput);
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

void editWeightSched(int editInput) {
  menuFlag = true;
  String feedType;

  editInput = editInput - 1;

  // Ask user input for editing the feed weight to be dispense

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1]-Mash");
  lcd.setCursor(0, 1);
  lcd.print("[2]-Starter");
  lcd.setCursor(0, 2);
  lcd.print("[3]-Grower");

  // Ask input
  lcd.setCursor(0, 3);
  lcd.print("Enter your choice:");
  feedQuantity[editInput].feedType = getInput(19, 3, 1, 1, 3);

  menuFlag = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  if(feedQuantity[editInput].feedType == 1) {
    feedType = "Mash";
  }
  else if(feedQuantity[editInput].feedType == 2) {
    feedType = "Starter";
  }
  else if(feedQuantity[editInput].feedType == 3) {
    feedType = "Grower";
  }
  lcd.print(String(editInput + 1) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("Enter Feed Weight:");
  feedQuantity[editInput].feedWeight = getDecimalInput(0, 3, 4, 5);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  if(feedQuantity[editInput].feedType == 1) {
    feedType = "Mash";
  }
  else if(feedQuantity[editInput].feedType == 2) {
    feedType = "Starter";
  }
  else if(feedQuantity[editInput].feedType == 3) {
    feedType = "Grower";
  }
  lcd.print(String(editInput + 1) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("Enter week duration:");
  feedQuantity[editInput].weekDuration = getInput(0, 3, 2, 1, 10);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  lcd.print(String(editInput + 1) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("No. of feedings per");
  lcd.setCursor(0, 3);
  lcd.print("day:");
  feedQuantity[editInput].howManyTimesADay = getInput(5, 3, 1, 1,9);

  feedQuantity[editInput].totalNumOfTimes = (feedQuantity[editInput].weekDuration * 7) * feedQuantity[editInput].howManyTimesADay;
  feedQuantity[editInput].isActivated = true;
  feedQuantity[editInput].isFinished = false;

  updateWeightEEPROM();
  editSuccessfulMsg();
}

void addFeedWeight() {
  if(feedWeightCtr < 10) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[1]-Mash");
    lcd.setCursor(0, 1);
    lcd.print("[2]-Starter");
    lcd.setCursor(0, 2);
    lcd.print("[3]-Grower");

    // Ask input
    lcd.setCursor(0, 3);
    lcd.print("Enter your choice:");
    int input = getInput(19, 3, 1, 0, 3);

    if(input == 0){
      return;
    }

    feedQuantity[feedWeightCtr].feedType = input;

    menuFlag = false;
  
    // Ask user input for setting up feed weight to be dispense

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Feed Weight:");
    feedQuantity[feedWeightCtr].feedWeight = getDecimalInput(0, 1, 4, 5);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter week duration:");
    feedQuantity[feedWeightCtr].weekDuration = getInput(0, 1, 2, 1, 10);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No. of feedings per");
    lcd.setCursor(0, 1);
    lcd.print("day:");
    feedQuantity[feedWeightCtr].howManyTimesADay = getInput(5, 1, 1, 1,9);

    feedQuantity[feedWeightCtr].totalNumOfTimes = (feedQuantity[feedWeightCtr].weekDuration * 7) * feedQuantity[feedWeightCtr].howManyTimesADay;
    feedQuantity[feedWeightCtr].isActivated = true;
    feedQuantity[feedWeightCtr].isFinished = false;

    feedWeightCtr++;
    EEPROM.update(2, feedWeightCtr);
    // Blynk.virtualWrite(V39, String(feedWeightCtr) + " / 10");

    updateWeightEEPROM();
    addSuccessfulMsg();
  }
  
  if(feedWeightCtr >= 10) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All 10 weights are set");
    delay(3000);
  }
}

void deleteFeedWeight(int delInput) {
  if(delInput == 0){
    weightReturnFlag = true;
    weightDeleteFlag = false;
    delMenuFlag = false;
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Delete weight " + String(delInput));
  lcd.setCursor(0, 1);
  lcd.print("[1]-YES");
  lcd.setCursor(0, 2);
  lcd.print("[2]-CANCEL");
  lcd.setCursor(0, 3);
  lcd.print("Enter your choice:");
  int choice = getInput(19, 3, 1, 1, 2);

  if(choice == 1){
    delInput = delInput - 1; // Subtract one on the number to delete to locate it using array index

    for(int i = delInput; i < feedWeightCtr; i++){
      if(i == 9){
        feedQuantity[i] = {0, 0, 0, 0, 0, 0, 0};
      }
      else {
        feedQuantity[i] = feedQuantity[i + 1];
      }

      updateWeightEEPROM();
    }

    feedWeightCtr--;   // Decrement the number of schedule
    EEPROM.update(2, feedWeightCtr);
    // Blynk.virtualWrite(V39, String(feedWeightCtr) + " / 10");

    if(feedWeightCtr == 0){
      feedDispenseCtr = 0;
      EEPROM.update(3, feedDispenseCtr);
    }
    
    deleteSuccessfulMsg();
  }

  weightDeleteFlag = true;
  delMenuFlag = true;
}


#endif