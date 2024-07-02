#ifndef STRUCK_H
#define STRUCK_H

// Declare array of structures for feed time schedules
struct Time {
  int hour; 
  int minute;
  bool isPM;
  bool isActivated;
};

// Declare array of structures for feed weight
struct Quantity {
  int feedType;
  double feedWeight;
  int weekDuration;
  int howManyTimesADay;
  int totalNumOfTimes;
  bool isActivated;
  bool isFinished;
};

struct Custom {
  int month;
  int day;
  int hour;
  int minute;
  bool isPM;
  double feedWeight;
  bool isActivated;
  bool isFinished;
};

#endif