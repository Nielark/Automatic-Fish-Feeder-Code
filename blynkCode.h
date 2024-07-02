#ifndef BLYNKCODE_H
#define BLYNKCODE_H

#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL6-P0buzgr"
#define BLYNK_TEMPLATE_NAME "Fish Feeder"

#include <BlynkSimpleShieldEsp8266.h>
#include "variables.h"
#include "sortSched.h"
#include "eeprom1.h"

void displayLCD1();
void displayLCD3();

// Array initialization for months
extern String months[12];

extern WidgetLCD LCD1;
extern WidgetLCD LCD2;
extern WidgetLCD LCD3;

// Getting the input for hour using number widget
BLYNK_WRITE(V1){
  // Assign incoming value from pin V1 to a variable
  blynkHr = param.asInt(); 
}

// Getting the input for minute using number widget
BLYNK_WRITE(V2){
  // Assign incoming value from pin V2 to a variable
  blynkMin = param.asInt(); 
}

// Getting the input for AM or PM using menu widget
BLYNK_WRITE(V3){
  switch (param.asInt()) {
    case 0: {
      blynkIsPM = false;
      break;
    }
    case 1: {
      blynkIsPM = true;
      break;
    }
    case 2: {
      
    break;
    }
  }
}

// Set the input time schedule and save to EEPROM
BLYNK_WRITE(V4){
  int pinValue = param.asInt(); // assigning incoming value from pin V4 to a variable
  bool isAlreadySet = false;
  
  // When the set button is press save the input to the structure of array when feedSched is not full and hour input is not 0
  if (pinValue == 1){
    if(feedSchedCtr < 10){
      if(blynkHr != 0 && feedSchedCtr == 0){
        feedTime[feedSchedCtr].hour = blynkHr;
        feedTime[feedSchedCtr].minute = blynkMin;
        feedTime[feedSchedCtr].isPM = blynkIsPM;
        feedTime[feedSchedCtr].isActivated = true;

        feedSchedCtr++;                   // Increment the number of schedule time
        EEPROM.update(0, feedSchedCtr);   // Update the number of schedule in the EEPROM

        sortFeedSched();                  // Sort the schedule in ascending order according to time preferences

        Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");

        // Display a successful message when the data is save
        LCD1.clear();
        LCD1.print(5, 0, "Added");
        LCD1.print(2, 1, "Successfully");
        delay(800);
        LCD1.clear();

        displayLCD1();
      }
      else if(blynkHr != 0 && feedSchedCtr > 0){
        // Checks if the input time is already set and in the list
        for(int i = 0; i < feedSchedCtr; i++){
          if(blynkHr == feedTime[i].hour &&
            blynkMin == feedTime[i].minute &&
            blynkIsPM == feedTime[i].isPM
          ){
            isAlreadySet = true;
            break;
          }
        }

        if(!isAlreadySet) {
          feedTime[feedSchedCtr].hour = blynkHr;
          feedTime[feedSchedCtr].minute = blynkMin;
          feedTime[feedSchedCtr].isPM = blynkIsPM;
          feedTime[feedSchedCtr].isActivated = true;

          feedSchedCtr++;                   // Increment the number of schedule time
          EEPROM.update(0, feedSchedCtr);   // Update the number of schedule in the EEPROM

          sortFeedSched();                  // Sort the schedule in ascending order according to time preferences

          Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");

          // Display a successful message when the data is save
          LCD1.clear();
          LCD1.print(5, 0, "Added");
          LCD1.print(2, 1, "Successfully");
          delay(800);
          LCD1.clear();

          displayLCD1();
        }
        else{
          // Display a message when schedule is already on the lists
          LCD1.clear();
          LCD1.print(2, 0, "The same time");
          LCD1.print(1, 1, "is already set");
          delay(1000);
          LCD1.clear();
        }
      }
      else{
        LCD1.clear();
        LCD1.print(0, 0, "Please set hour");
        delay(1000);
        LCD1.clear();
      }
    }

    if(feedSchedCtr >= 10){
      LCD1.clear();
      LCD1.print(1, 0, "Schedule limit");
      LCD1.print(4, 1, "reached!");
      delay(1000);
      LCD1.clear();
    }
  } 

  // Initialize the variables back into zero
  blynkHr = 0;
  blynkMin = 0;
  blynkIsPM = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 2);
}

// Selecting the respective number of the schedule
BLYNK_WRITE(V10){
  int pinValue = param.asInt(); // assigning incoming value from pin V10 to a variable

  if(pinValue <= feedSchedCtr){
    Blynk.setProperty(V4, "isDisabled", true);  // Disable the set button widget
    Blynk.setProperty(V8, "isDisabled", false); // Enable the edit button widget
    Blynk.setProperty(V9, "isDisabled", false); // Enable the delete button widget

    switch (param.asInt()) {
      case 0: {
        Blynk.setProperty(V4, "isDisabled", false); // Enable the set button widget
        Blynk.setProperty(V8, "isDisabled", true); // Disable the edit button widget
        Blynk.setProperty(V9, "isDisabled", true); // Disable the delete button widget

        // Display 0 as a default in the widgets
        Blynk.virtualWrite(V1, 0);
        Blynk.virtualWrite(V2, 0);
        Blynk.virtualWrite(V3, 2);
        break;
      }
      case 1: {
        blynkEditInput = 0;
        break;
      }
      case 2: {
        blynkEditInput = 1;
        break;
      }
      case 3: {
        blynkEditInput = 2;
        break;
      }  
      case 4: { // Item 2
        blynkEditInput = 3;
        break;
      }  
      case 5: {
        blynkEditInput = 4;
        break;
      }  
      case 6: {
        blynkEditInput = 5;
        break;
      }
      case 7: {
        blynkEditInput = 6;
        break;
      }   
      case 8: {
        blynkEditInput = 7;
        break;
      }   
      case 9: {
        blynkEditInput = 8;
        break;
      }   
      case 10: {
        blynkEditInput = 9;
        break;
      }    
    }
    
    if(pinValue != 0){
      // Display the selected schedule in the hour, minute, and AM or PM widget
      Blynk.virtualWrite(V1, feedTime[blynkEditInput].hour);
      Blynk.virtualWrite(V2, feedTime[blynkEditInput].minute);
      Blynk.virtualWrite(V3, feedTime[blynkEditInput].isPM);

      // Store the value in the variables
      blynkHr = feedTime[blynkEditInput].hour;
      blynkMin = feedTime[blynkEditInput].minute;
      blynkIsPM = feedTime[blynkEditInput].isPM;
    }
  }
  else {
    Blynk.setProperty(V4, "isDisabled", false); // Enable the set button widget
    Blynk.setProperty(V8, "isDisabled", true); // Disable the edit button widget
    Blynk.setProperty(V9, "isDisabled", true); // Disable the delete button widget

    // Display 0 as a default in the widgets
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 2);
    Blynk.virtualWrite(V10, 0);

    LCD1.clear();
    LCD1.print(4, 0, "Schedule");
    LCD1.print(4, 1, "not found");
    delay(800);
    LCD1.clear();
  }
}

// Deleting the value of selected schedule
BLYNK_WRITE(V8){
  int pinValue = param.asInt(); // assigning incoming value from pin V8 to a variable
  
  if (pinValue == 1){
    for(int i = blynkEditInput; i < feedSchedCtr; i++){
      // When the 10th schedule is selected, initialize all value into 0
      if(i == 9){
        feedTime[i] = {0, 0, 0, 0};
      }
      else {
        feedTime[i] = feedTime[i + 1];  // Traverse the value starting from the index of deleted schedule
      }

      updateFeedSchedEEPROM();          // Update the changes into the EEPROM
    }

    feedSchedCtr--;                     // Decrement the number of schedule
    EEPROM.update(0, feedSchedCtr);     // Update the changes of the number of schedule in EEPROM

    Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");

    // Display a successful message when the data is deleted
    LCD1.clear();
    LCD1.print(4, 0, "Deleted");
    LCD1.print(2, 1, "Successfully");
    delay(800);
    LCD1.clear();
  } 

  /// Initialize the variables back into zero
  blynkHr = 0;
  blynkMin = 0;
  blynkIsPM = 0;

  // Dispaly the value in the hour, minute, AM or PM in widget as 0 or default
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 2);
  Blynk.virtualWrite(V10, 0);
  Blynk.setProperty(V4, "isDisabled", false);   // Enable the set schedule button
  Blynk.setProperty(V8, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V9, "isDisabled", true); // Disable the delete button widget
}

// Editing the schedule
BLYNK_WRITE(V9){
  int pinValue = param.asInt(); // assigning incoming value from pin V9 to a variable
  bool isAlreadySet = false;
  
  if (pinValue == 1){
    if(blynkHr != 0){
      // Checks if the input time is already set and in the list
      for(int i = 0; i < feedSchedCtr; i++){
        if(blynkHr == feedTime[i].hour &&
          blynkMin == feedTime[i].minute &&
          blynkIsPM == feedTime[i].isPM
        ){
          isAlreadySet = true;
          break;
        }
      }

      if(!isAlreadySet) {
        feedTime[blynkEditInput].hour = blynkHr;
        feedTime[blynkEditInput].minute = blynkMin;
        feedTime[blynkEditInput].isPM = blynkIsPM;
        feedTime[blynkEditInput].isActivated = true;

        sortFeedSched();    // Sort the schedue after editing
        
        // Display a successful message when the data is edited
        LCD1.clear();
        LCD1.print(5, 0, "Edited");
        LCD1.print(2, 1, "Successfully");
        delay(800);
        LCD1.clear();

        displayLCD1();
      }
      else{
        // Display a message when schedule is already on the lists
        LCD1.clear();
        LCD1.print(2, 0, "The same time");
        LCD1.print(1, 1, "is already set");
        delay(1000);
        LCD1.clear();
      }
    }
    else{
      LCD1.clear();
      LCD1.print(0, 0, "Please set hour");
      delay(1000);
      LCD1.clear();
    }
  } 

  // Initialize the variables back into zero
  blynkHr = 0;
  blynkMin = 0;
  blynkIsPM = 0;

  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 2);
  Blynk.virtualWrite(V10, 0);
  Blynk.setProperty(V4, "isDisabled", false);   // Enable the set schedule button
  Blynk.setProperty(V8, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V9, "isDisabled", true); // Disable the delete button widget
}

/* ==============================
          SET FEED WEIGHT
================================*/

// Getting the input for the type of feed using menu widget
BLYNK_WRITE(V11){
  switch (param.asInt()) {
    case 0: {
      
      break;
    }
    case 1: {
      blynkFeedType = 1;
      break;
    }
    case 2: {
      blynkFeedType = 2;
    break;
    }
    case 3: {
      blynkFeedType = 3;
    break;
    }
  }
}

// Getting the input for feed weight using number widget
BLYNK_WRITE(V12){
  // Assign incoming value from pin V12 to a variable
  blynkFeedWeight = param.asDouble();
}

/// Getting the input for week duration using number widget
BLYNK_WRITE(V13){
  // Assign incoming value from pin V13 to a variable
  blynkWeekDur = param.asInt(); 
}

// Getting the input for number of dispense per day using number widget
BLYNK_WRITE(V14){
  // Assign incoming value from pin V14 to a variable
  blynkNumOfDispense = param.asInt(); 
}

// Set the input feed information and save to EEPROM
BLYNK_WRITE(V16){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  // When the set button is press save the input to the structure of array when feedSched is not full and hour input is not 0
  if (pinValue == 1){
    if(feedWeightCtr < 10){
      if(blynkFeedType != 0 && blynkFeedWeight != 0 && blynkWeekDur != 0 && blynkNumOfDispense != 0){
        feedQuantity[feedWeightCtr].feedType = blynkFeedType;
        feedQuantity[feedWeightCtr].feedWeight = blynkFeedWeight;
        feedQuantity[feedWeightCtr].weekDuration = blynkWeekDur;
        feedQuantity[feedWeightCtr].howManyTimesADay = blynkNumOfDispense;
        feedQuantity[feedWeightCtr].totalNumOfTimes = (blynkWeekDur * 7) * blynkNumOfDispense;
        feedQuantity[feedWeightCtr].isActivated = true;
        feedQuantity[feedWeightCtr].isFinished = false;

        feedWeightCtr++;                   // Increment the number of feed weight list
        EEPROM.update(2, feedWeightCtr);   // Update the number of feed weight in the EEPROM

        updateWeightEEPROM();              // Update changes in the EEPROM

        Blynk.virtualWrite(V39, String(feedWeightCtr) + " / 10");

        // Display a successful message when the data is save
        LCD2.clear();
        LCD2.print(5, 0, "Added");
        LCD2.print(2, 1, "Successfully");
        delay(800);
        LCD2.clear();
      }
      else{
        LCD2.clear();
        LCD2.print(0, 0, "Fill out all the");
        LCD2.print(3, 1, "information");
        delay(1000);
        LCD2.clear();
      }
    }

    if(feedWeightCtr >= 10){
      LCD1.clear();
      LCD1.print(2, 0, "Weight limit");
      LCD1.print(4, 1, "reached!");
      delay(1000);
      LCD1.clear();
    }
  }

  // Initialize the variables back into zero
  blynkFeedType = 0;
  blynkFeedWeight = 0;
  blynkWeekDur = 0;
  blynkNumOfDispense = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V11, 0);
  Blynk.virtualWrite(V12, 0);
  Blynk.virtualWrite(V13, 0);
  Blynk.virtualWrite(V14, 0);
}

// Editing the weight
BLYNK_WRITE(V17){
  int pinValue = param.asInt(); // assigning incoming value from pin V9 to a variable
  
  if (pinValue == 1){
    if(blynkFeedType != 0 && blynkFeedWeight != 0 && blynkWeekDur != 0 && blynkNumOfDispense != 0){
      feedQuantity[blynkEditInput].feedType = blynkFeedType;
      feedQuantity[blynkEditInput].feedWeight = blynkFeedWeight;
      feedQuantity[blynkEditInput].weekDuration = blynkWeekDur;
      feedQuantity[blynkEditInput].howManyTimesADay = blynkNumOfDispense;
      feedQuantity[blynkEditInput].totalNumOfTimes = (blynkWeekDur * 7) * blynkNumOfDispense;
      feedQuantity[blynkEditInput].isActivated = true;
      feedQuantity[blynkEditInput].isFinished = false;

      updateWeightEEPROM();              // Update changes in the EEPROM
      
      // Display a successful message when the data is edited
      LCD2.clear();
      LCD2.print(5, 0, "Edited");
      LCD2.print(2, 1, "Successfully");
      delay(800);
      LCD2.clear();
    }
    else{
      LCD2.clear();
      LCD2.print(0, 0, "Fill out all the");
      LCD2.print(3, 1, "information");
      delay(3000);
      LCD2.clear();
    }
  }

  // Initialize the variables back into zero
  blynkFeedType = 0;
  blynkFeedWeight = 0;
  blynkWeekDur = 0;
  blynkNumOfDispense = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V11, 0);
  Blynk.virtualWrite(V12, 0);
  Blynk.virtualWrite(V13, 0);
  Blynk.virtualWrite(V14, 0);
  Blynk.virtualWrite(V19, 0);
  Blynk.setProperty(V16, "isDisabled", false); // Enable the set button widget
  Blynk.setProperty(V17, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V18, "isDisabled", true); // Disable the delete button widget
  Blynk.virtualWrite(V23, "0 remaining out of 0");
}

// Deleting the value of selected weight
BLYNK_WRITE(V18){
  int pinValue = param.asInt(); // assigning incoming value from pin V8 to a variable
  
  if (pinValue == 1){
    for(int i = blynkEditInput; i < feedWeightCtr; i++){
      // When the 10th schedule is selected, initialize all value into 0
      if(i == 9){
        feedQuantity[i] = {0, 0, 0, 0, 0, 0, 0};
      }
      else {
        feedQuantity[i] = feedQuantity[i + 1];  // Traverse the value starting from the index of deleted schedule
      }

      updateWeightEEPROM();              // Update changes in the EEPROM
    }

    feedWeightCtr--;                   // Increment the number of feed weight list
    EEPROM.update(2, feedWeightCtr);   // Update the number of feed weight in the EEPROM

    Blynk.virtualWrite(V39, String(feedWeightCtr) + " / 10");

    if(feedWeightCtr == 0){
      feedDispenseCtr = 0;
      EEPROM.update(3, feedDispenseCtr);
    }

    // Display a successful message when the data is deleted
    LCD2.clear();
    LCD2.print(4, 0, "Deleted");
    LCD2.print(2, 1, "Successfully");
    delay(800);
    LCD2.clear();
  }

  // Initialize the variables back into zero
  blynkFeedType = 0;
  blynkFeedWeight = 0;
  blynkWeekDur = 0;
  blynkNumOfDispense = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V11, 0);
  Blynk.virtualWrite(V12, 0);
  Blynk.virtualWrite(V13, 0);
  Blynk.virtualWrite(V14, 0);
  Blynk.virtualWrite(V19, 0);
  Blynk.setProperty(V16, "isDisabled", false); // Enable the set button widget
  Blynk.setProperty(V17, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V18, "isDisabled", true); // Disable the delete button widget
  Blynk.virtualWrite(V23, "0 remaining out of 0");
}

// Selecting the respective number of the weight
BLYNK_WRITE(V19){
  int pinValue = param.asInt(); // assigning incoming value from pin V10 to a variable

  if(pinValue <= feedWeightCtr){
    Blynk.setProperty(V16, "isDisabled", true);  // Disable the set button widget
    Blynk.setProperty(V17, "isDisabled", false); // Enable the edit button widget
    Blynk.setProperty(V18, "isDisabled", false); // Enable the delete button widget

    switch (param.asInt()) {
      case 0: {
        Blynk.setProperty(V16, "isDisabled", false); // Enable the set button widget
        Blynk.setProperty(V17, "isDisabled", true); // Disable the edit button widget
        Blynk.setProperty(V18, "isDisabled", true); // Disable the delete button widget
        Blynk.virtualWrite(V23, "0 remaining out of 0");

        // Display 0 as a default in the widgets
        Blynk.virtualWrite(V11, 0);
        Blynk.virtualWrite(V12, 0);
        Blynk.virtualWrite(V13, 0);
        Blynk.virtualWrite(V14, 0);
        break;
      }
      case 1: {
        blynkEditInput = 0;
        break;
      }
      case 2: {
        blynkEditInput = 1;
        break;
      }
      case 3: {
        blynkEditInput = 2;
        break;
      }  
      case 4: { // Item 2
        blynkEditInput = 3;
        break;
      }  
      case 5: {
        blynkEditInput = 4;
        break;
      }  
      case 6: {
        blynkEditInput = 5;
        break;
      }
      case 7: {
        blynkEditInput = 6;
        break;
      }   
      case 8: {
        blynkEditInput = 7;
        break;
      }   
      case 9: {
        blynkEditInput = 8;
        break;
      }   
      case 10: {
        blynkEditInput = 9;
        break;
      }    
    }
    
    if(pinValue != 0){
      // Display the selected schedule in the hour, minute, and AM or PM widget
      Blynk.virtualWrite(V11, feedQuantity[blynkEditInput].feedType);
      Blynk.virtualWrite(V12, feedQuantity[blynkEditInput].feedWeight);
      Blynk.virtualWrite(V13, feedQuantity[blynkEditInput].weekDuration);
      Blynk.virtualWrite(V14, feedQuantity[blynkEditInput].howManyTimesADay);
      Blynk.virtualWrite(V23, String(feedQuantity[blynkEditInput].totalNumOfTimes) + " remaining out of " + String((feedQuantity[blynkEditInput].weekDuration * 7) * feedQuantity[blynkEditInput].howManyTimesADay));

      // Store the value in the variables
      blynkFeedType = feedQuantity[blynkEditInput].feedType;
      blynkFeedWeight = feedQuantity[blynkEditInput].feedWeight;
      blynkWeekDur = feedQuantity[blynkEditInput].weekDuration;
      blynkNumOfDispense = feedQuantity[blynkEditInput].howManyTimesADay;
    }
  }
  else{
    Blynk.setProperty(V16, "isDisabled", false); // Enable the set button widget
    Blynk.setProperty(V17, "isDisabled", true); // Disable the edit button widget
    Blynk.setProperty(V18, "isDisabled", true); // Disable the delete button widget
    Blynk.virtualWrite(V23, "0 remaining out of 0");

    // Display 0 as a default in the widgets
    Blynk.virtualWrite(V11, 0);
    Blynk.virtualWrite(V12, 0);
    Blynk.virtualWrite(V13, 0);
    Blynk.virtualWrite(V14, 0);
    Blynk.virtualWrite(V19, 0);

    LCD2.clear();
    LCD2.print(4, 0, "Data");
    LCD2.print(4, 1, "not found");
    delay(800);
    LCD2.clear();
  }
}

/* ==============================
        SET CUSTOM SCHEDULE
================================*/

// Getting the input for month using menu widget
BLYNK_WRITE(V24){
  blynkMonth = param.asInt();
}

// Getting the input for date using menu widget
BLYNK_WRITE(V25){
  blynkDate = param.asInt();
}

// Getting the input for hour using number widget
BLYNK_WRITE(V27){
  // Assign incoming value from pin V27 to a variable
  blynkCustomHr = param.asInt();
}

// Getting the input for minute using number widget
BLYNK_WRITE(V28){
  // Assign incoming value from pin V28 to a variable
  blynkCustomMin = param.asInt();
}

// Getting the input for AM or PM using menu widget
BLYNK_WRITE(V29){
  switch (param.asInt()) {
    case 0: {
      blynkCustomIsPM = false;
      break;
    }
    case 1: {
      blynkCustomIsPM = true;
      break;
    }
    case 2: {
      
    break;
    }
  }
}

// Getting the input for feed weight using number widget
BLYNK_WRITE(V26){
  // Assign incoming value from pin V12 to a variable
  blynkCustomFeedWeight = param.asDouble(); 
}

// Set the custom schedule and save to EEPROM
BLYNK_WRITE(V31){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  bool isAlreadySet = false;
  
  // When the set button is press save the input to the structure of array when feedSched is not full and hour input is not 0
  if (pinValue == 1){
    if(tempSchedCtr < 10){
      if(blynkMonth != 0 && blynkDate != 0 && blynkCustomHr != 0 && blynkCustomFeedWeight != 0 && tempSchedCtr == 0){
        tempSched[tempSchedCtr].month = blynkMonth;
        tempSched[tempSchedCtr].day = blynkDate;
        tempSched[tempSchedCtr].hour = blynkCustomHr;
        tempSched[tempSchedCtr].minute = blynkCustomMin;
        tempSched[tempSchedCtr].isPM = blynkCustomIsPM;
        tempSched[tempSchedCtr].feedWeight = blynkCustomFeedWeight;
        tempSched[tempSchedCtr].isActivated = true;
        tempSched[tempSchedCtr].isFinished = false;

        tempSchedCtr++;                    // Increment the number of custom feed list
        EEPROM.update(1, tempSchedCtr);    // Update the number of feed weight in the EEPROM
        updateTemporarySchedEEPROM();      // Update changes in the EEPROM

        Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");

        // Display a successful message when the data is save
        LCD3.clear();
        LCD3.print(5, 0, "Added");
        LCD3.print(2, 1, "Successfully");
        delay(800);
        LCD3.clear();

        displayLCD3();
      }
      else if(blynkMonth != 0 && blynkDate != 0 && blynkCustomHr != 0 && blynkCustomFeedWeight != 0 && tempSchedCtr > 0){
        // Checks if the input time is already set and in the list
        for(int i = 0; i < tempSchedCtr; i++){
          if(blynkMonth == tempSched[i].month &&
            blynkDate == tempSched[i].day &&
            blynkCustomHr == tempSched[i].hour &&
            blynkCustomMin == tempSched[i].minute &&
            blynkCustomIsPM == tempSched[i].isPM
          ){
            isAlreadySet = true;
            break;
          }
        }

        if(!isAlreadySet) {
          tempSched[tempSchedCtr].month = blynkMonth;
          tempSched[tempSchedCtr].day = blynkDate;
          tempSched[tempSchedCtr].hour = blynkCustomHr;
          tempSched[tempSchedCtr].minute = blynkCustomMin;
          tempSched[tempSchedCtr].isPM = blynkCustomIsPM;
          tempSched[tempSchedCtr].feedWeight = blynkCustomFeedWeight;
          tempSched[tempSchedCtr].isActivated = true;
          tempSched[tempSchedCtr].isFinished = false;

          tempSchedCtr++;
          EEPROM.update(1, tempSchedCtr);
          updateTemporarySchedEEPROM();

          Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");

          // Display a successful message when the data is save
          LCD3.clear();
          LCD3.print(5, 0, "Added");
          LCD3.print(2, 1, "Successfully");
          delay(800);
          LCD3.clear();

          displayLCD3();
        }
        else{
          // Display a message when schedule is already on the lists
          LCD3.clear();
          LCD3.print(2, 0, "The same time");
          LCD3.print(1, 1, "is already set");
          delay(1000);
          LCD3.clear();
        }
      }
      else{
        LCD3.clear();
        LCD3.print(0, 0, "Fill out all the");
        LCD3.print(3, 1, "information");
        delay(1000);
        LCD3.clear();
      }
    }
    
    // All ten schedules have been set, display a message
    if(tempSchedCtr >= 10){
      LCD3.clear();
      LCD3.print(1, 0, "Schedule limit");
      LCD3.print(4, 1, "reached!");
      delay(800);
      LCD3.clear();
    }
  } 

  // Initialize the variables back into zero
  blynkMonth = 0;
  blynkDate = 0;
  blynkCustomHr = 0;
  blynkCustomMin = 0;
  blynkCustomIsPM = 0;
  blynkCustomFeedWeight = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V24, 0);
  Blynk.virtualWrite(V25, 0);
  Blynk.virtualWrite(V26, 0);
  Blynk.virtualWrite(V27, 0);
  Blynk.virtualWrite(V28, 0);
  Blynk.virtualWrite(V29, 2);
  Blynk.virtualWrite(V30, 0);
}

// Editing the custom schedule
BLYNK_WRITE(V32){
  int pinValue = param.asInt(); // assigning incoming value from pin V32 to a variable
  bool isAlreadySet = false;
  
  if (pinValue == 1){
    if(blynkMonth != 0 && blynkDate != 0 && blynkCustomHr != 0 && blynkCustomFeedWeight != 0){
      // Checks if the input time is already set and in the list
      for(int i = 0; i < tempSchedCtr; i++){
        if(blynkMonth == tempSched[i].month &&
          blynkDate == tempSched[i].day &&
          blynkCustomHr == tempSched[i].hour &&
          blynkCustomMin == tempSched[i].minute &&
          blynkCustomIsPM == tempSched[i].isPM
        ){
          isAlreadySet = true;
          break;
        }
      }

      if(!isAlreadySet){
        tempSched[blynkEditInput - 1].month = blynkMonth;
        tempSched[blynkEditInput - 1].day = blynkDate;
        tempSched[blynkEditInput - 1].hour = blynkCustomHr;
        tempSched[blynkEditInput - 1].minute = blynkCustomMin;
        tempSched[blynkEditInput - 1].isPM = blynkCustomIsPM;
        tempSched[blynkEditInput - 1].feedWeight = blynkCustomFeedWeight;
        
        updateTemporarySchedEEPROM();        // Update changes in the EEPROM

        // Display a successful message when the data is edited
        LCD3.clear();
        LCD3.print(5, 0, "Edited");
        LCD3.print(2, 1, "Successfully");
        delay(800);
        LCD3.clear();

        displayLCD3();
      }
      else{
        // Display a message when schedule is already on the lists
        LCD3.clear();
        LCD3.print(2, 0, "The same time");
        LCD3.print(1, 1, "is already set");
        delay(1000);
        LCD3.clear();
      }       
    }
    else{
      LCD3.clear();
      LCD3.print(0, 0, "Fill out all the");
      LCD3.print(3, 1, "information");
      delay(1000);
      LCD3.clear();
    }
  }

  // Initialize the variables back into zero
  blynkMonth = 0;
  blynkDate = 0;
  blynkCustomHr = 0;
  blynkCustomMin = 0;
  blynkCustomIsPM = 0;
  blynkCustomFeedWeight = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V24, 0);
  Blynk.virtualWrite(V25, 0);
  Blynk.virtualWrite(V26, 0);
  Blynk.virtualWrite(V27, 0);
  Blynk.virtualWrite(V28, 0);
  Blynk.virtualWrite(V29, 2);
  Blynk.virtualWrite(V30, 0);
  Blynk.setProperty(V31, "isDisabled", false); // Enable the set button widget
  Blynk.setProperty(V32, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V33, "isDisabled", true); // Disable the delete button widget
}

// Deleting the value of selected custom schedule
BLYNK_WRITE(V33){
  int pinValue = param.asInt(); // assigning incoming value from pin V33 to a variable
  
  if (pinValue == 1){
    for(int i = (blynkEditInput - 1); i < tempSchedCtr; i++){
      // When the 10th schedule is selected, initialize all value into 0
      if(i == 9){
        tempSched[i] = {0, 0, 0, 0, 0, 0, 0, 0};
      }
      else {
        tempSched[i] = tempSched[i + 1];  // Traverse the value starting from the index of deleted schedule
      }

      updateTemporarySchedEEPROM();    // Update changes in the EEPROM
    }

    tempSchedCtr--;                    // Increment the number of feed weight list
    EEPROM.update(1, tempSchedCtr);    // Update the number of feed weight in the EEPROM
    
    Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");

    // Display a successful message when the data is deleted
    LCD3.clear();
    LCD3.print(4, 0, "Deleted");
    LCD3.print(2, 1, "Successfully");
    delay(800);
    LCD3.clear();
  } 

  // Initialize the variables back into zero
  blynkMonth = 0;
  blynkDate = 0;
  blynkCustomHr = 0;
  blynkCustomMin = 0;
  blynkCustomIsPM = 0;
  blynkCustomFeedWeight = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V24, 0);
  Blynk.virtualWrite(V25, 0);
  Blynk.virtualWrite(V26, 0);
  Blynk.virtualWrite(V27, 0);
  Blynk.virtualWrite(V28, 0);
  Blynk.virtualWrite(V29, 2);
  Blynk.virtualWrite(V30, 0);
  Blynk.setProperty(V31, "isDisabled", false); // Enable the set button widget
  Blynk.setProperty(V32, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V33, "isDisabled", true); // Disable the delete button widget
}

// Selecting the respective number of the custom schedule
BLYNK_WRITE(V30){
  int pinValue = param.asInt(); // assigning incoming value from pin V10 to a variable

  if(pinValue <= tempSchedCtr){
  Blynk.setProperty(V31, "isDisabled", true);  // Disable the set button widget
  Blynk.setProperty(V32, "isDisabled", false); // Enable the edit button widget
  Blynk.setProperty(V33, "isDisabled", false); // Enable the delete button widget

  blynkEditInput = pinValue;
  
  if(blynkEditInput == 0){
    Blynk.setProperty(V31, "isDisabled", false); // Enable the set button widget
    Blynk.setProperty(V32, "isDisabled", true); // Disable the edit button widget
    Blynk.setProperty(V33, "isDisabled", true); // Disable the delete button widget

    // Display 0 as a default in the widgets
    Blynk.virtualWrite(V24, 0);
    Blynk.virtualWrite(V25, 0);
    Blynk.virtualWrite(V26, 0);
    Blynk.virtualWrite(V27, 0);
    Blynk.virtualWrite(V28, 0);
    Blynk.virtualWrite(V29, 2);
  }
  else if(blynkEditInput != 0){
    // Display the selected schedule in the hour, minute, and AM or PM widget
    Blynk.virtualWrite(V24, tempSched[blynkEditInput - 1].month);
    Blynk.virtualWrite(V25, tempSched[blynkEditInput - 1].day);
    Blynk.virtualWrite(V27, tempSched[blynkEditInput - 1].hour);
    Blynk.virtualWrite(V28, tempSched[blynkEditInput - 1].minute);
    Blynk.virtualWrite(V29, tempSched[blynkEditInput - 1].isPM);
    Blynk.virtualWrite(V26, tempSched[blynkEditInput - 1].feedWeight);

    // Store the value in the variables
    blynkMonth = tempSched[blynkEditInput - 1].month;
    blynkDate = tempSched[blynkEditInput - 1].day;
    blynkCustomHr = tempSched[blynkEditInput - 1].hour;
    blynkCustomMin = tempSched[blynkEditInput - 1].minute;
    blynkCustomIsPM = tempSched[blynkEditInput - 1].isPM;
    blynkCustomFeedWeight = tempSched[blynkEditInput - 1].feedWeight;
  }
  }
  else {
    Blynk.setProperty(V31, "isDisabled", false); // Enable the set button widget
    Blynk.setProperty(V32, "isDisabled", true); // Disable the edit button widget
    Blynk.setProperty(V33, "isDisabled", true); // Disable the delete button widget
    
    // Display 0 as a default in the widgets
    Blynk.virtualWrite(V24, 0);
    Blynk.virtualWrite(V25, 0);
    Blynk.virtualWrite(V26, 0);
    Blynk.virtualWrite(V27, 0);
    Blynk.virtualWrite(V28, 0);
    Blynk.virtualWrite(V29, 2);
    Blynk.virtualWrite(V30, 0);

    LCD3.clear();
    LCD3.print(4, 0, "Schedule");
    LCD3.print(4, 1, "not found");
    delay(800);
    LCD3.clear();
  }
}

void myTimer(){
  // This function describes what will happen with each timer tick
  Blynk.virtualWrite(V0, cm);
  Blynk.virtualWrite(V40, DOlevel);

  if(feedSchedCtr == 0){
    LCD1.print(0, 0, "Schedule list is");
    LCD1.print(5, 1, "empty");
  }
  
  if(feedWeightCtr == 0){
    LCD2.print(1, 0, "Weight list is");
    LCD2.print(5, 1, "empty");
  }

  if(tempSchedCtr == 0){
    LCD3.print(0, 0, "Schedule list is");
    LCD3.print(5, 1, "empty");
  }
}

// Display list of schedules in blynk LCD
void displayLCD1(){
  if(feedTime[blynkScrollCtr1].isActivated) {
    // Adds leading zero for time less than 10 and convert it into string
    String fHour = (feedTime[blynkScrollCtr1].hour < 10 ? "0" : "") + String(feedTime[blynkScrollCtr1].hour);
    String fMinute = (feedTime[blynkScrollCtr1].minute < 10 ? "0" : "") + String(feedTime[blynkScrollCtr1].minute);
    String AMorPM = (feedTime[blynkScrollCtr1].isPM ? "PM" : "AM");

    LCD1.print(0, 0, String(blynkScrollCtr1 + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);

    if(feedSchedCtr > 1){
      String fHour2 = (feedTime[blynkScrollCtr1 + 1].hour < 10 ? "0" : "") + String(feedTime[blynkScrollCtr1 + 1].hour);
      String fMinute2 = (feedTime[blynkScrollCtr1 + 1].minute < 10 ? "0" : "") + String(feedTime[blynkScrollCtr1 + 1].minute);
      String AMorPM2 = (feedTime[blynkScrollCtr1 + 1].isPM ? "PM" : "AM");

      LCD1.print(0, 1, String(blynkScrollCtr1 + 2) + ") " + fHour2 + ":" + fMinute2 + " " + AMorPM2);
    }
  }
}

// For scrolling up the content of LCD1 widget
BLYNK_WRITE(V5){
  int pinValue = param.asInt(); // assigning incoming value from pin V5 to a variable
  
  if (pinValue == 1){
    blynkUpLCD1 = pinValue;

    if(blynkUpLCD1 == 1){ // Scroll up
      LCD1.clear();
      if (blynkScrollCtr1 > 0) {
        blynkScrollCtr1--;
      }

      blynkUpLCD1 = 0;
    }
  }

  displayLCD1();
}

// For scrolling down the content of the LCD1 widget
BLYNK_WRITE(V6){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
    blynkDownLCD1 = pinValue;

    if(blynkDownLCD1 == 1){ // Scroll down
      LCD1.clear();
      if (blynkScrollCtr1 < (feedSchedCtr - 1)) {
        blynkScrollCtr1++;
      }

      blynkDownLCD1 = 0;
    }
  }

  displayLCD1();
}

// Display list of weight in blynk LCD
void displayLCD2() {
  String feedType;

  switch(feedQuantity[blynkScrollCtr2].feedType) {
    case 1:
      feedType = "Mash";
      break;
    case 2:
      feedType = "Starter";
      break;
    case 3:
      feedType = "Grower";
      break;
  }

  if(feedQuantity[blynkScrollCtr2].isActivated) {
    LCD2.print(0, 0, String(blynkScrollCtr2 + 1) + ") " + feedType);
    LCD2.print(3, 1, String(feedQuantity[blynkScrollCtr2].feedWeight) + " Kg");
  }
}

// For scrolling up the content of LCD2 widget
BLYNK_WRITE(V21){
  int pinValue = param.asInt(); // assigning incoming value from pin V5 to a variable
  
  if (pinValue == 1){
    blynkUpLCD2 = pinValue;

    // Scroll up
    if(blynkUpLCD2 == 1 && blynkScrollCtr2 > 0) {
      LCD2.clear();
      blynkScrollCtr2--;
      blynkUpLCD2 = 0;
    }
  }

  displayLCD2();
}

// For scrolling down the content of the LCD2 widget
BLYNK_WRITE(V22){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
    blynkDownLCD2 = pinValue;

    // Scroll down
    if(blynkDownLCD2 == 1 && blynkScrollCtr2 < (feedWeightCtr - 1)) {
      LCD2.clear();
      blynkScrollCtr2++;
      blynkDownLCD2 = 0;
    }
  }

  displayLCD2();
}

// Display list of custom schedules in blynk LCD3
void displayLCD3() {
  String date = months[tempSched[blynkScrollCtr3].month - 1] + "/" + String(tempSched[blynkScrollCtr3].day) + "/" + String(curYear);
  String fHour = (tempSched[blynkScrollCtr3].hour < 10 ? "0" : "") + String(tempSched[blynkScrollCtr3].hour);
  String fMinute = (tempSched[blynkScrollCtr3].minute < 10 ? "0" : "") + String(tempSched[blynkScrollCtr3].minute);
  String AMorPM = (tempSched[blynkScrollCtr3].isPM ? "PM" : "AM");
  String feedWeight = String(tempSched[blynkScrollCtr3].feedWeight) + " Kg";

  if(tempSched[blynkScrollCtr3].isActivated) {
    LCD3.print(0, 0, String(blynkScrollCtr3 + 1) + ") " + date);
    LCD3.print(3, 1, fHour + ":" + fMinute + " " + AMorPM);
  }
}

// For scrolling up the content of LCD3 widget
BLYNK_WRITE(V35){
  int pinValue = param.asInt(); // assigning incoming value from pin V5 to a variable
  
  if (pinValue == 1){
    blynkUpLCD3 = pinValue;

    // Scroll up
    if(blynkUpLCD3 == 1 && blynkScrollCtr3 > 0) {
      LCD3.clear();
      blynkScrollCtr3--;
      blynkUpLCD3 = 0;
    }
  }

  displayLCD3();
}

// For scrolling down the content of the LCD3 widget
BLYNK_WRITE(V36){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
    blynkDownLCD3 = pinValue;

    // Scroll down
    if(blynkDownLCD3 == 1 && blynkScrollCtr3 < (tempSchedCtr - 1)) {
      LCD2.clear();
      blynkScrollCtr3++;
      blynkDownLCD3 = 0;
    }
  }

  displayLCD3();
}

// For refreshing the content of the LCD1 widget and number of list
BLYNK_WRITE(V41){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
    displayLCD1();
    Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");
  }
}

// For refreshing the content of the LCD1 widget and number of list
BLYNK_WRITE(V42){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
     displayLCD2();
     Blynk.virtualWrite(V39, String(feedWeightCtr) + " / 10");

    if(feedWeightCtr == 0){
      feedDispenseCtr = 0;
      EEPROM.update(3, feedDispenseCtr);
    }
  }
}

// For refreshing the content of the LCD1 widget and number of list
BLYNK_WRITE(V43){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
    displayLCD3();
    Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");
  }
}

#endif