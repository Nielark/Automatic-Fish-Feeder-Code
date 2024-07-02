#ifndef EEPROM_H
#define EEPROM_H

#include "variables.h"

void getDataFromEEPROM() {
  /*===========================================================//
        // GETTING THE LAST UPDATED VALUES FROM THE EEPROM
  //===========================================================*/

  // The following code get the last updated values from the eeprom and store them to the designated variables
  // The following code are added so that if sudden power issues happens, the data will not be fully erase in the system
  
  // Checks if the address for the fishSchedCtr is not equal to 255, then initialized the past values in the variable
  // If flase the feedSched counter will be equal to  0
  if(EEPROM.read(0) != 255) {
    feedSchedCtr = EEPROM.read(0);
  }
  else {
    feedSchedCtr = 0;
  }

  if(EEPROM.read(1) != 255) {
    tempSchedCtr = EEPROM.read(1);
  }
  else {
    tempSchedCtr = 0;
  }

  // Checks if the address for the fishScfeedWeight is not equal to 255, then initialized the past values in the variable
  // If flase the feedWeightCtr counter will be equal to  0
  if(EEPROM.read(2) != 255) {
    feedWeightCtr = EEPROM.read(2);
  }
  else {
    feedWeightCtr = 0;
  }

  // Checks if the address for the feedDispenseCtr is not equal to 255, then initialized the past values in the variable
  // If flase the feedDispenseCtr counter will be equal to  0
  if(EEPROM.read(3) != 255) {
    feedDispenseCtr = EEPROM.read(3);
  }
  else {
    feedDispenseCtr = 0;
  }

  // The following code is for retrieving the scheduled time save from the EEPROM address
  int ctr1 = 4;   // For traversing the address of EEPROM

  // Each address in the EEPROM will be read and store it in the struct
  for(int i = 0; i < feedSchedCtr; i++) {
    feedTime[i].hour = EEPROM.read(ctr1);
    ctr1++;   // Increment after each retrieval to succesfully get back the updated information for all schedule
    feedTime[i].minute = EEPROM.read(ctr1);
    ctr1++;
    feedTime[i].isPM = EEPROM.read(ctr1);
    ctr1++;
    feedTime[i].isActivated = EEPROM.read(ctr1);
    ctr1++;
  }

  int ctr2 = 44;  // For traversing the address of EEPROM

  // Each address in the EEPROM will be read and store it in the struct
  for(int i = 0; i < tempSchedCtr; i++) {
    tempSched[i].month = EEPROM.read(ctr2);
    ctr2++;   // Increment after each retrieval to succesfully get back the updated information for all setted weight
    tempSched[i].day = EEPROM.read(ctr2);
    ctr2++;
    tempSched[i].hour = EEPROM.read(ctr2);
    ctr2++;
    tempSched[i].minute = EEPROM.read(ctr2);
    ctr2++;
    tempSched[i].isPM = EEPROM.read(ctr2);
    ctr2++;
    EEPROM.get(ctr2, tempSched[i].feedWeight);
    ctr2 += sizeof(double);
    tempSched[i].isActivated = EEPROM.read(ctr2);
    ctr2++;
    tempSched[i].isFinished = EEPROM.read(ctr2);
    ctr2++;
  }

  // The following code is for retrieving the set weight save from the EEPROM address
  int ctr3 = 154;  // For traversing the address of EEPROM

  // Each address in the EEPROM will be read and store it in the struct
  for(int i = 0; i < feedWeightCtr; i++) {
    feedQuantity[i].feedType = EEPROM.read(ctr3);
    ctr3++;   // Increment after each retrieval to succesfully get back the updated information for all setted weight
    EEPROM.get(ctr3, feedQuantity[i].feedWeight);
    ctr3 += sizeof(double);
    feedQuantity[i].weekDuration = EEPROM.read(ctr3);
    ctr3++;
    feedQuantity[i].howManyTimesADay = EEPROM.read(ctr3);
    ctr3++;
    feedQuantity[i].totalNumOfTimes = EEPROM.read(ctr3);
    ctr3++;
    feedQuantity[i].isActivated = EEPROM.read(ctr3);
    ctr3++;
    feedQuantity[i].isFinished = EEPROM.read(ctr3);
    ctr3++;
  }
}

void updateFeedSchedEEPROM() {
  int addressCtr = 4;
  
  for(int i = 0; i < feedSchedCtr; i++) {
    EEPROM.update(addressCtr, feedTime[i].hour);
    addressCtr++;
    EEPROM.update(addressCtr, feedTime[i].minute);
    addressCtr++;
    EEPROM.update(addressCtr, feedTime[i].isPM);
    addressCtr++;
    EEPROM.update(addressCtr, feedTime[i].isActivated);
    addressCtr++;
  }
}

void updateTemporarySchedEEPROM(){
  int addressCtr = 44;

  for(int i = 0; i < tempSchedCtr; i++) {
    EEPROM.update(addressCtr, tempSched[i].month);
    addressCtr++;
    EEPROM.update(addressCtr, tempSched[i].day);
    addressCtr++;
    EEPROM.update(addressCtr, tempSched[i].hour);
    addressCtr++;
    EEPROM.update(addressCtr, tempSched[i].minute);
    addressCtr++;
    EEPROM.update(addressCtr, tempSched[i].isPM);
    addressCtr++;
    EEPROM.put(addressCtr, tempSched[i].feedWeight);
    addressCtr += sizeof(double);
    EEPROM.update(addressCtr, tempSched[i].isActivated);
    addressCtr++;
    EEPROM.update(addressCtr, tempSched[i].isFinished);
    addressCtr++;
  }
}

void updateWeightEEPROM() {
  int addressCtr = 154;
  
  for(int i = 0; i < feedWeightCtr; i++) {
    EEPROM.update(addressCtr, feedQuantity[i].feedType);
    addressCtr++;
    EEPROM.put(addressCtr, feedQuantity[i].feedWeight);
    addressCtr += sizeof(double);
    EEPROM.update(addressCtr, feedQuantity[i].weekDuration);
    addressCtr++;
    EEPROM.update(addressCtr, feedQuantity[i].howManyTimesADay);
    addressCtr++;
    EEPROM.update(addressCtr, feedQuantity[i].totalNumOfTimes);
    addressCtr++;
    EEPROM.update(addressCtr, feedQuantity[i].isActivated);
    addressCtr++;
    EEPROM.update(addressCtr, feedQuantity[i].isFinished);
    addressCtr++;
  }
}

#endif