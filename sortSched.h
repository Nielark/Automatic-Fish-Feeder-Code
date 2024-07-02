#ifndef SORTSHED_H
#define SORTSHED_H

#include "variables.h"
#include "eeprom1.h"

void sortFeedSched() {
  int pos, pos2 = 0;
  int len = 3;
  int hourMinVal, minuteMinVal;
  bool isPM;
  bool isActivated;

  // Sort the schedule list based on time and in terms of AM and PM
  // Sorts the schedule list when there are more than one schedule
  if(feedSchedCtr > 1){
    // Start of AM
    for(int i = 0; i < feedSchedCtr; i++){
      // Initialization of minimum values for comparison
      hourMinVal = 12;
      minuteMinVal = 59;
      for(int j = i; j < feedSchedCtr; j++){
        // Check if the schedule time is AM
        if(!feedTime[j].isPM){
          if(feedTime[j].hour < hourMinVal || (feedTime[j].hour == hourMinVal && feedTime[j].minute < minuteMinVal)){
            // Temporary save the current minimum time value in variables
            hourMinVal = feedTime[j].hour;
            minuteMinVal = feedTime[j].minute;
            isPM = feedTime[j].isPM;
            isActivated = feedTime[j].isActivated;
            pos = j;
          }
        }
      }
      
      // Swap the schedule times
      if(hourMinVal != 12 || minuteMinVal != 59){
        feedTime[pos].hour = feedTime[i].hour;
        feedTime[i].hour = hourMinVal;

        feedTime[pos].minute = feedTime[i].minute;
        feedTime[i].minute = minuteMinVal;

        feedTime[pos].isPM = feedTime[i].isPM;
        feedTime[i].isPM = isPM;

        feedTime[pos].isActivated = feedTime[i].isActivated;
        feedTime[i].isActivated = isActivated;

        pos2 = i + 1;
      }
    } // End of AM

    // Start of PM
    for(int i = pos2; i < feedSchedCtr; i++){
      // Initialization of minimum values for comparison
      hourMinVal = 12;
      minuteMinVal = 59;
      for(int j = i; j < feedSchedCtr; j++){
        // Check if the schedule time is PM
        if(feedTime[j].isPM){
          if(feedTime[j].hour < hourMinVal || (feedTime[j].hour == hourMinVal && feedTime[j].minute < minuteMinVal)){
            // Temporary save the current minimum time value in variables
            hourMinVal = feedTime[j].hour;
            minuteMinVal = feedTime[j].minute;
            isPM = feedTime[j].isPM;
            isActivated = feedTime[j].isActivated;
            pos = j;
          }
        }
      }
        
      // Swap the schedule times
      if(hourMinVal != 12 || minuteMinVal != 59){
        feedTime[pos].hour = feedTime[i].hour;
        feedTime[i].hour = hourMinVal;

        feedTime[pos].minute = feedTime[i].minute;
        feedTime[i].minute = minuteMinVal;

        feedTime[pos].isPM = feedTime[i].isPM;
        feedTime[i].isPM = isPM;

        feedTime[pos].isActivated = feedTime[i].isActivated;
        feedTime[i].isActivated = isActivated;
      }
    } // End of PM
  }

  updateFeedSchedEEPROM();
}

#endif