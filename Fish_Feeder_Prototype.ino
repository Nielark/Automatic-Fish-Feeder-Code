#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>
#include <Keypad.h>
#include <Wire.h> 
#include <EEPROM.h>
#include "HX711.h"
#include "ultrasonic.h"

// Load cell variables
const int pinDT = 2;
const int pinSCK = 3;
HX711 scale;

// Ultrasonic sensor variables
const int  echoPin = 4;
const int  triggerPin = 5;
int inches = 0;
int cm = 0;

const int relayPin = 6;

// Servo variables
const int servoPin1 = 7;
const int servoPin2 = 8;
const int servoPin3 = 9;
const int buzzerPin = 10;
Servo servoM1;
Servo servoM2;
Servo servoM3;

// Motor A connections
int enA = 13;
int in1 = 12;
int in2 = 11;

// Array initialization for days
char daysOfTheWeek[7][12] = {
  "Sun",
  "Mon",
  "Tue",
  "Wed",
  "Thu",
  "Fri",
  "Sat"
};

// 4x4 matrix keypad set up
const byte ROWS = 4;  //Four rows
const byte COLS = 4;  //Four columns

// Define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {36, 38, 40, 42};  // Connect to the row pinouts of the keypad
byte colPins[COLS] = {44, 46, 48, 50};  // Connect to the column pinouts of the keypad

// Initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
LiquidCrystal_I2C lcd(0x27, 16, 4);

// Initialize DS1307RTC
RTC_DS1307 rtc;

const unsigned long displayTimer = 1000;
unsigned long displayPrevTime = 0;
unsigned long displayCurTime = 0;

// Declare array of structures for feed time schedules
struct time {
  int hour; 
  int minute;
  bool isPM;
  bool isActivated;
};

// Declare array of structures for feed weight
struct quantity {
  int feedType;
  double feedWeight;
  int weekDuration;
  int howManyTimesADay;
  int totalNumOfTimes;
  bool isActivated;
  bool isFinished;
};

quantity feedQuantity[8];
time feedTime[3];
int feedSchedCtr = 0, feedSchedPos = -1, feedWeightCtr = 0, feedDispenseCtr = 0;
int curHour;
bool deleteFlag = false, weightDeleteFlag = false, schedReturnFlag = false, weightReturnFlag = false, menuFlag = false, delMenuFlag = false;

void setup(){
  Serial.begin(9600);
  rtc.begin();
  scale.begin(pinDT, pinSCK);

  getDataFromEEPROM();  // Function call for getting the last updated values in the EEPROM
  
  // Set initial time (adjust as needed)
  // Uncomment this code and set the values if you want to set the date and time
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2024, 1, 4, 11, 37));

  // Use the obtained scale and offset values from calibration
  scale.set_scale(401.735168);  // Replace `calibrationScale` with your obtained scale value
  scale.set_offset(4294953227);  // Replace `calibrationOffset` with your obtained offset value

  lcd.init();
	lcd.backlight();
	lcd.clear();
	lcd.setCursor(0,0);

  // Set pins to output pins
  pinMode(13, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(relayPin, LOW);

  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);
  servoM3.attach(servoPin3);

  // Set the initial state of servo motor into 0 degree
  servoM1.write(0);
  servoM2.write(0);
  servoM3.write(90);

  delay(2000);
  servoM1.detach();
  servoM2.detach();
  servoM3.detach();

  // Set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
	pinMode(in1, OUTPUT);
	pinMode(in2, OUTPUT);

  // Turn off motors - Initial state
	digitalWrite(in1, LOW);
	digitalWrite(in2, LOW);
}

void loop(){
  char keyInput = customKeypad.getKey();  // For getting keypad inputs
  DateTime now = rtc.now();

  displayCurTime = millis();
  
  if((displayCurTime - displayPrevTime >= displayTimer) && cm < 60) {
    // ULTRASONIC SENSOR READING
    // measure the ping time in cm
    cm = 0.01723 * readUltrasonicDistance(triggerPin, echoPin);

    // convert to inches by dividing by 2.54
    inches = (cm / 2.54);
    displayTime(now); // Function call to display current time
    displayFeedLevel(); // Function call to display feed level
  }

  // Condition to check the feed level of the container
  if(cm < 60) {
    digitalWrite(relayPin, LOW);
    noTone(buzzerPin);
    // Checks if the current time is equal to set time schedule to start feeding
    for (int i = 0; i < 3; i++) {
      if (curHour == feedTime[i].hour && 
          now.minute() == feedTime[i].minute &&
          now.second() == 0 &&
          now.isPM() == feedTime[i].isPM &&
          feedTime[i].isActivated == true)
      {
        dispenseFeed();   // Calls the function to dispense Feed
      }
    }
  }
  else {
    lcd.clear();
    
    while(cm > 60) {
      cm = 0.01723 * readUltrasonicDistance(triggerPin, echoPin);
      digitalWrite(relayPin, HIGH);
      tone(buzzerPin, 300, 500);

      //lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("   LOW FEED LEVEL");
      lcd.setCursor(0, 2);
      lcd.print(" FEED LEVEL: ");
      lcd.print(cm);
      lcd.print(" cm");
      delay(1000);
    }

    lcd.clear();
  }
  

  if (keyInput == 'D'){
    setTime();  // Function call to set time 
  }
  else if(keyInput == 'A') {
    backToMenu:
    menuFlag = true;

    feedSchedMenu();  // Function call to display scheduling options

    // Ask input
    lcd.setCursor(0, 3);
    lcd.print("Enter your choice:");
    int choice = getTimeInput(19, 3, 1, 0, 3);
    menuFlag = false;

    switch(choice) {
      case 1:
        viewSched();    // Function call to display the feeding schedules
        lcd.clear();
        goto backToMenu;
        break;

      case 2:
        addFeedSched();   // Function call for adding feeding schedules
        lcd.clear();
        goto backToMenu;
        break;

      case 3:
        deleteAgain:
        deleteFlag = true;
        delMenuFlag = true;
        viewSched();
        deleteFeedSched();    // Function call for deleting schedules
        delMenuFlag = false;

        if(feedSchedCtr == 0 || schedReturnFlag) {
          schedReturnFlag = false;
          lcd.clear();
          goto backToMenu;
        }

        lcd.clear();
        goto deleteAgain;
        break;
      
      case 0:
        lcd.clear();
        return;   // Exit
        break;
    }
  }
  // For setting feed weight info
  else if(keyInput == 'C') {
    backToWeightMenu:
    menuFlag = true;

    setFeedWeightMenu();    // Function call to display the menu about setting feed weight

    // Ask input
    lcd.setCursor(0, 3);
    lcd.print("Enter your choice:");
    int choice = getTimeInput(19, 3, 1, 0, 3);
    menuFlag = false;

    switch(choice) {
      case 1:
        viewFeedWeight();   // Function call to view the setted list of feed weight
        lcd.clear();
        goto backToWeightMenu;
        break;

      case 2:
        menuFlag = true;
        addFeedWeight();    // Function call to add feed weight
        lcd.clear();
        goto backToWeightMenu;
        break;

      case 3:
        delMenuFlag = true;
        deleteWeightAgain:
        weightDeleteFlag = true;
        viewFeedWeight();

        if(feedWeightCtr == 0 || weightReturnFlag) {
          weightReturnFlag = false;
          lcd.clear();
          goto backToWeightMenu;
        }

        lcd.clear();
        goto deleteWeightAgain;
        break;
      
      case 0:
        lcd.clear();
        return;   // Exit
        break;
    }
  }
}

void displayTime(DateTime currentTime) {
  //displayCurTime = millis();
  
  //if(displayCurTime - displayPrevTime >= displayTimer) {
    //lcd.clear();

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
    curHour = currentTime.twelveHour();   // Convert the time into 12 hour format
    // lcd.setCursor(0, 3);
    // lcd.print(currentTime.hour());

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
  //}
}

void setTime() {
  int morningOrAfternoon;

  // Set the date
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Month:");
  int month = getTimeInput(13, 0, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Day Of Week:");
  int day = getTimeInput(0, 1, 2, 1, 31);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Year:");
  int year = getTimeInput(12, 0, 4, 1, 9000);

  // Set the time
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Hours:");
  int hour = getTimeInput(13, 0, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Minutes:");
  int minute = getTimeInput(15, 0, 2, 0, 59);

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice:");
  morningOrAfternoon = getTimeInput(14, 2, 1, 1, 2);

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
  // int second = getTimeInput();

  // Set the date and time from the input values
  rtc.adjust(DateTime(year, month, day, hour, minute));
}

void viewSched(){
  top:
  bool editSchedFlag = false;
  int editInput;

  lcd.clear();

  // Prompt if schedule list is empty
  if(feedSchedCtr == 0 && deleteFlag == false){
    lcd.setCursor(0, 1);
    lcd.print(" Schedule is Empty");
    delay(2000);
    return;
  }

  //sortFeedSched();  // Function call to sort the time schedule before displaying into LCD

  // Display the feeding shedules
  while(true){
    for(int i = 0; i < feedSchedCtr; i++){
      if(feedTime[i].isActivated) {
        lcd.setCursor(0, i);
        // Adds leading zero for time less than 10 and convert it into string
        String fHour = (feedTime[i].hour < 10 ? "0" : "") + String(feedTime[i].hour);
        String fMinute = (feedTime[i].minute < 10 ? "0" : "") + String(feedTime[i].minute);
        String AMorPM = (feedTime[i].isPM ? "PM" : "AM");
        lcd.print(String(i + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);
      }
    }
    
    // break the loop and ask for delete input
    if(deleteFlag == true) {
      break;
    }

    // Exit and back to menu
    char exitKey = customKeypad.getKey();
    if(exitKey == 'B' && deleteFlag == false) {
      break;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= feedSchedCtr && (exitKey - '0') > 0  && deleteFlag == false) {
      editSchedFlag = true;
      editInput = exitKey - '0'; // Convert char to int
      break;
    }
    else if(deleteFlag == true){
      break;
    }
  }

  if(editSchedFlag) {
    editFeedSched(editInput);
    editSchedFlag = false;
    goto top;
  }
}

void editFeedSched(int editInput) {
  int morningOrAfternoon;

  String fHour = (feedTime[editInput - 1].hour < 10 ? "0" : "") + String(feedTime[editInput - 1].hour);
  String fMinute = (feedTime[editInput - 1].minute < 10 ? "0" : "") + String(feedTime[editInput - 1].minute);
  String AMorPM = (feedTime[editInput - 1].isPM ? "PM" : "AM");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scheduled Time:");
  lcd.setCursor(0, 1);
  lcd.print(String(editInput) + ") " + fHour + ":" + fMinute + " " + AMorPM);
  
  lcd.setCursor(0, 2);
  lcd.print("Enter Hour:");
  feedTime[editInput - 1].hour = getTimeInput(12, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scheduled Time:");
  lcd.setCursor(0, 1);
  fHour = (feedTime[editInput - 1].hour < 10 ? "0" : "") + String(feedTime[editInput - 1].hour);
  lcd.print(String(editInput) + ") " + fHour + ":" + fMinute + " " + AMorPM);

  lcd.setCursor(0, 2);
  lcd.print("Enter Minute:");
  feedTime[editInput - 1].minute = getTimeInput(14, 2, 2, 0, 59);
  feedTime[editInput - 1].isActivated = true;

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice:");
  morningOrAfternoon = getTimeInput(14, 2, 1, 1, 2);

  if(morningOrAfternoon == 1) {
    feedTime[editInput - 1].isPM = false;
  }
  else {
    feedTime[editInput - 1].isPM = true;
  }

  // int tmpCtnHour = feedTime[feedSchedPos].hour;
  // int tmpCtnMin = feedTime[feedSchedPos].minute;
  // EEPROM.update(0, tmpCtnHour);
  // EEPROM.update(1, tmpCtnMin);

  sortFeedSched();
}

void addFeedSched() {
  int morningOrAfternoon;

  if(feedSchedCtr < 3){
    for(int i = 0; i < 3; i++) {
      feedSchedPos++;

      if(feedTime[i].isActivated == false){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Schedule " + String(feedSchedPos + 1));
        
        lcd.setCursor(0, 1);
        lcd.print("Enter Hour:");
        feedTime[feedSchedPos].hour = getTimeInput(12, 1, 2, 1, 12);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Schedule " + String(feedSchedPos + 1));

        lcd.setCursor(0, 1);
        lcd.print("Enter Minute:");
        feedTime[feedSchedPos].minute = getTimeInput(14, 1, 2, 0, 59);
        feedTime[feedSchedPos].isActivated = true;

        // Selection for AM and PM
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("[1] - AM");
        lcd.setCursor(0, 1);
        lcd.print("[2] - PM");
        lcd.setCursor(0, 2);
        lcd.print("Enter choice:");
        morningOrAfternoon = getTimeInput(14, 2, 1, 1, 2);

        if(morningOrAfternoon == 1) {
          feedTime[feedSchedPos].isPM = false;
        }
        else {
          feedTime[feedSchedPos].isPM = true;
        }

        // int tmpCtnHour = feedTime[feedSchedPos].hour;
        // int tmpCtnMin = feedTime[feedSchedPos].minute;
        // EEPROM.update(0, tmpCtnHour);
        // EEPROM.update(1, tmpCtnMin);

        feedSchedCtr++;
        EEPROM.update(12, feedSchedCtr);
        break;
      }
    }

    sortFeedSched();
    
    // All three schedules have been set, display a message
    if (feedSchedCtr >= 3) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("All 3 schedules set");
      delay(3000);
    }
  }
  // All three schedules have been set, display a message
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All 3 schedules set");
    delay(3000);
  }

  feedSchedPos = -1;
}

void deleteFeedSched() {
  // Prompt if the schedule is empty and no data to delete
  if(feedSchedCtr == 0){
    lcd.setCursor(0, 1);
    lcd.print("Schedule is Empty");
    lcd.setCursor(0, 2);
    lcd.print("No Data to Delete");
    deleteFlag = false;
    delay(2000);
    return;
  }

  lcd.setCursor(0, 3);
  lcd.print("Enter the number:");
  int delInput = getTimeInput(18, 3, 1, 0, 3);

  if(delInput == 0){
    schedReturnFlag = true;
    deleteFlag = false;
    return;
  }

  for(int i = 0; i < feedSchedCtr; i++){
    if(feedTime[i].hour == feedTime[delInput - 1].hour && 
      feedTime[i].minute == feedTime[delInput - 1].minute)
    {
      // Traverse the position of array elements to delete
      for(int j = i; j < feedSchedCtr; j++){
        feedTime[j].hour = feedTime[j + 1].hour;
        feedTime[j].minute = feedTime[j + 1].minute;
        feedTime[j].isPM = feedTime[j + 1].isPM;
        feedTime[j].isActivated = feedTime[j + 1].isActivated;

        // Initialize the last elements into 0 and not activated
        if(j == 2) {
          feedTime[j].hour = 0;
          feedTime[j].minute = 0;
          feedTime[j].isActivated = false;
        }
      }

      updateFeedSchedEEPROM();

      feedSchedCtr--;   // Decrement the number of schedule
      EEPROM.update(12, feedSchedCtr);
      break;
    }
  }
  
  deleteFlag = false;
}

int getTimeInput(int cursorPosCols, int cursorPosRow, int charLen, int minVal, int maxVal) {
  int cursorPos = cursorPosCols - 1;
  int cursorPosCtr = 0;
  String container = "";
  top:
  lcd.setCursor(cursorPosCols, cursorPosRow);

  while(true) {
    char keyInput = customKeypad.getKey();
    
    // Enter key
    // Press '#' the loop and saves the input
    if(keyInput == '#') {
      break;
    }
    // Condition to accept digit inputs only
    else if(isDigit(keyInput) && cursorPosCtr < charLen) {
        container += keyInput;
        lcd.print(keyInput);
        cursorPosCtr++;
    }
    //Press '*' to delete the last character
    else if(keyInput == '*' && cursorPosCtr > 0) {
      lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
      lcd.print(' ');
      lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
      container.remove(container.length() - 1);
      cursorPosCtr--;
    }
  }
  
  // Save the value when it meets the requirements
  if(container != "" && container.toInt() >= minVal && container.toInt() <= maxVal) {
    return container.toInt();
  }
  // Input does not meet the requirements
  else {
    lcd.setCursor(0, 3);
    lcd.print("   Invalid Input!  ");

    // Automatically deletes the incorrect input
    if(cursorPosCtr <= charLen) {
      while(cursorPosCtr != 0) {
        lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
        lcd.print(' ');
        lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
        container.remove(container.length() - 1);
        cursorPosCtr--;
      }
    }

    // Deletes back the prompt "Invalid Input!" and ask for user input again.
    if(menuFlag) {
      delay(1000);
      lcd.setCursor(0, 3);
      lcd.print("                   ");
      
      lcd.setCursor(0, 3);
      lcd.print("Enter your choice:");
    }
    else if(delMenuFlag) {
      delay(1000);
      lcd.setCursor(0, 3);
      lcd.print("                   ");
      
      lcd.setCursor(0, 3);
      lcd.print("Enter the number:");
    }
    else {
      delay(1000);
      lcd.setCursor(0, 3);
      lcd.print("                   ");
    }

    goto top;
  }
}

void sortFeedSched(){
  int pos, pos2 = 0;
  int len = 3;
  int hourMinVal, minuteMinVal;
  bool isPM;
  bool isActivated;

  // Sort the schedule list base on time and in terms of AM and PM
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
          if(feedTime[j].hour < hourMinVal || feedTime[j].minute < minuteMinVal){
            // Temporary save the current minimum time value in a variables
            hourMinVal = feedTime[j].hour;
            minuteMinVal = feedTime[j].minute;
            isPM = feedTime[j].isPM;
            isActivated = feedTime[j].isActivated;
            pos = j;
          }

          // Start to sort the time by changing their position in the array
          feedTime[pos].hour = feedTime[i].hour;    // Substitute the current hour into the minimum time
          feedTime[i].hour = hourMinVal;            // Substitute the final minimum hour to the current hour from the temporary variable

          feedTime[pos].minute = feedTime[i].minute;
          feedTime[i].minute = minuteMinVal;

          feedTime[pos].isPM = feedTime[i].isPM;
          feedTime[i].isPM = isPM;

          feedTime[pos].isActivated = feedTime[i].isActivated;
          feedTime[i].isActivated = isActivated;

          pos2 = i + 1;
        }
      } // End of loop
    } // End of AM

    // Start of PM
    for(int i = pos2; i < feedSchedCtr; i++){
      // Initialization of minimum values for comparison
      hourMinVal = 12;
      minuteMinVal = 59;
      for(int j = i; j < feedSchedCtr; j++){
        // Check if the schedule time is PM
        if(feedTime[j].isPM){
          if(feedTime[j].hour < hourMinVal || feedTime[j].minute < minuteMinVal){
            // Temporary save the current minimum time value in a variables
            hourMinVal = feedTime[j].hour;
            minuteMinVal = feedTime[j].minute;
            isPM = feedTime[j].isPM;
            isActivated = feedTime[j].isActivated;
            pos = j;
          }

          // Start to sort the time by changing their position in the array
          feedTime[pos].hour = feedTime[i].hour;    // Substitute the current hour into the minimum time
          feedTime[i].hour = hourMinVal;            // Substitute the final minimum hour to the current hour from the temporary variable

          feedTime[pos].minute = feedTime[i].minute;
          feedTime[i].minute = minuteMinVal;

          feedTime[pos].isPM = feedTime[i].isPM;
          feedTime[i].isPM = isPM;

          feedTime[pos].isActivated = feedTime[i].isActivated;
          feedTime[i].isActivated = isActivated;
        }
      } // End of loop
    } // End of PM
  }

  updateFeedSchedEEPROM();
}

void feedSchedMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1]-View Schedule");
  lcd.setCursor(0, 1);
  lcd.print("[2]-Add Schedule");
  lcd.setCursor(0, 2);
  lcd.print("[3]-Delete Schedule");
}

void setFeedWeightMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1]-View");
  lcd.setCursor(0, 1);
  lcd.print("[2]-Add");
  lcd.setCursor(0, 2);
  lcd.print("[3]-Delete");
}

int LCD_ROWS = 4; // Define the number of rows in your LCD

int scrollPosition = 0; // Initialize the scroll position

void viewFeedWeight() {
  top:
  bool editWeightFlag = false;
  int editInput;
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

  LCD_ROWS = (weightDeleteFlag? 3 : 4);

  // Display the weight List
  while (true) {
    for (int i = scrollPosition; i < min(feedWeightCtr, scrollPosition + LCD_ROWS); i++) {
      lcd.setCursor(0, i - scrollPosition);

      if(feedQuantity[i].feedType == 1) {
        feedType = "Mash";
      }
      else if(feedQuantity[i].feedType == 2) {
        feedType = "Starter";
      }
      else if(feedQuantity[i].feedType == 3) {
        feedType = "Grower";
      }

      lcd.print(String(i + 1) + ") " + feedType);
      lcd.setCursor(13, i - scrollPosition);
      lcd.print(String(feedQuantity[i].feedWeight) + " kg");
    }

    char inputKey = customKeypad.getKey();

    // Press 'A' to scroll the list upward
    if (inputKey == 'A') { // Scroll up
      lcd.clear();
      if (scrollPosition > 0) {
        scrollPosition--;
      }
    } 
    // Press 'D' to scroll the list downward
    else if (inputKey == 'D') { // Scroll down
      lcd.clear();
      if (scrollPosition < feedWeightCtr - LCD_ROWS) {
        scrollPosition++;
      }
    }
    else if(isDigit(inputKey) && (inputKey - '0') <= feedWeightCtr && (inputKey - '0') > 0) {
      editWeightFlag = true;
      editInput = inputKey - '0'; // Convert char to int
      break;
    }
    // Press 'B' to back
    else if (inputKey == 'B') {
      break; // Start deletion if 'D' is press
    }
  }

  if(editWeightFlag && !weightDeleteFlag) {
    editWeightSched(editInput);
    editWeightFlag = false;
    goto top;
  }
  else if(!editWeightFlag && weightDeleteFlag) {
    deleteFeedWeight();   // Function call to delete a particular feed weight
  }
}

void editWeightSched(int editInput) {
  menuFlag = true;
  String feedType;

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
  feedQuantity[editInput - 1].feedType = getTimeInput(19, 3, 1, 1, 3);

  menuFlag = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  if(feedQuantity[editInput - 1].feedType == 1) {
    feedType = "Mash";
  }
  else if(feedQuantity[editInput - 1].feedType == 2) {
    feedType = "Starter";
  }
  else if(feedQuantity[editInput - 1].feedType == 3) {
    feedType = "Grower";
  }
  lcd.print(String(editInput) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput - 1].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("Enter Feed Weight:");
  feedQuantity[editInput - 1].feedWeight = getDecimalInput(0, 3, 4, 5);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  if(feedQuantity[editInput - 1].feedType == 1) {
    feedType = "Mash";
  }
  else if(feedQuantity[editInput - 1].feedType == 2) {
    feedType = "Starter";
  }
  else if(feedQuantity[editInput - 1].feedType == 3) {
    feedType = "Grower";
  }
  lcd.print(String(editInput) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput - 1].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("Enter week duration:");
  feedQuantity[editInput - 1].weekDuration = getTimeInput(0, 3, 2, 1, 10);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  lcd.print(String(editInput) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput - 1].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("Enter how many times");
  lcd.setCursor(0, 3);
  lcd.print("a day:");
  feedQuantity[editInput - 1].howManyTimesADay = getTimeInput(7, 3, 1, 1,9);

  feedQuantity[editInput - 1].totalNumOfTimes = (feedQuantity[editInput - 1].weekDuration * 7) * feedQuantity[editInput - 1].howManyTimesADay;
  feedQuantity[editInput - 1].isActivated = true;
  feedQuantity[editInput - 1].isFinished = false;

  updateWeightEEPROM();
}

void addFeedWeight() {
  if(feedWeightCtr < 8) {
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
    int input = getTimeInput(19, 3, 1, 0, 3);

    if(input == 0){
      return;
    }

    feedQuantity[feedWeightCtr].feedType = input;
  }

  menuFlag = false;
  
  // Ask user input for setting up feed weight to be dispense

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Feed Weight:");
  feedQuantity[feedWeightCtr].feedWeight = getDecimalInput(0, 1, 4, 5);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter week duration:");
  feedQuantity[feedWeightCtr].weekDuration = getTimeInput(0, 1, 2, 1, 10);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter how many times");
  lcd.setCursor(0, 1);
  lcd.print("a day:");
  feedQuantity[feedWeightCtr].howManyTimesADay = getTimeInput(7, 1, 1, 1,9);

  feedQuantity[feedWeightCtr].totalNumOfTimes = (feedQuantity[feedWeightCtr].weekDuration * 7) * feedQuantity[feedWeightCtr].howManyTimesADay;
  feedQuantity[feedWeightCtr].isActivated = true;
  feedQuantity[feedWeightCtr].isFinished = false;

  feedWeightCtr++;
  EEPROM.update(100, feedWeightCtr);

  updateWeightEEPROM();
}

void deleteFeedWeight() {
  lcd.setCursor(0, 3);
  lcd.print("Enter the number:");
  int delInput = getTimeInput(18, 3, 1, 0, feedWeightCtr);

  if(delInput == 0){
    weightReturnFlag = true;
    weightDeleteFlag = false;
    delMenuFlag = false;
    return;
  }

  for(int i = 0; i < feedWeightCtr; i++){
    // Condition to find if the information from the input match the selected weight list to delete
    if(feedQuantity[i].feedType == feedQuantity[delInput - 1].feedType && 
      feedQuantity[i].feedWeight == feedQuantity[delInput - 1].feedWeight &&
      feedQuantity[i].howManyTimesADay == feedQuantity[delInput - 1].howManyTimesADay &&
      feedQuantity[i].totalNumOfTimes == feedQuantity[delInput - 1].totalNumOfTimes &&
      feedQuantity[i].isActivated == feedQuantity[delInput - 1].isActivated &&
      feedQuantity[i].isFinished == feedQuantity[delInput - 1].isFinished)
    {
      // Traverse the position of array elements for deletion
      for(int j = i; j < feedWeightCtr; j++){
        feedQuantity[j].feedType = feedQuantity[j + 1].feedType;
        feedQuantity[j].feedWeight = feedQuantity[j + 1].feedWeight;
        feedQuantity[j].howManyTimesADay = feedQuantity[j + 1].howManyTimesADay;
        feedQuantity[j].totalNumOfTimes = feedQuantity[j + 1].totalNumOfTimes;
        feedQuantity[j].isActivated = feedQuantity[j + 1].isActivated;
        feedQuantity[j].isFinished = feedQuantity[j + 1].isFinished;
      }

      updateWeightEEPROM();

      feedWeightCtr--;  // Decrement the number of weight list
      EEPROM.update(100, feedWeightCtr);
      break;
    }
  }

  weightDeleteFlag = false;
  delMenuFlag = false;
}

void dispenseFeed() {
  servoM1.attach(servoPin1);

  double weight = scale.get_units();  // Get weight measurement in the desired unit

  // M1
  // Drop feed from M1 to M2
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("M1 PROCESSING");
  servoM1.write(180);

  // TODO:
  //    1. Condition to stop M1 to drop feeds when load cell limit set weight was acheive
  if(feedQuantity[feedDispenseCtr].totalNumOfTimes == 0) {
    feedQuantity[feedDispenseCtr].isFinished = true;
    feedDispenseCtr++;
  }
  
  if(feedQuantity[feedDispenseCtr].totalNumOfTimes != 0) {
    servoM2.detach();
    const unsigned long weightTimer = 1000;
    unsigned long weightPrevTime = 0;
    unsigned long weightCurrTime = 0;

    while(weight < feedQuantity[feedDispenseCtr].feedWeight) {
      weightCurrTime = millis();
      weight = scale.get_units();  // Get weight measurement in the desired unit
      if(weightCurrTime - weightPrevTime >= weightTimer) {
        displayWeightLCD(weight);   // Function call to display the current feed weight on the LCD

        weightPrevTime = weightCurrTime;
      }
    }

    servoM2.attach(servoPin2);
    servoM3.attach(servoPin3);
    EEPROM.update(101, feedDispenseCtr);

    feedQuantity[feedDispenseCtr].totalNumOfTimes--;

    updateWeightEEPROM();
    
    servoM1.write(0);
  }
  
  //delay(5000);
  
  displayWeightLCD(weight);   // Function call to display the current feed weight on the LCD
  
  delay(3000);

  // TODO: Blower code at this part
  digitalWrite(13, HIGH); // Relay trigger to start the blower
  analogWrite(enA, 255);
  digitalWrite(in1, HIGH);
	digitalWrite(in2, LOW);

  lcd.setCursor(0, 0);
  lcd.println("M2 PROCESSING");
  // Drop feed from M2 to M3
  servoM2.write(180); // Drop the feeds to M3

  // M3
  // Dispensed feed and swing left and right
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("M3 PROCESSING");
  for(int i = 0; i < 20; i++) {
    servoM3.write(0); //Spin in one direction
    delay(200);
    servoM3.write(90); // Stop
    delay(230);
    servoM3.write(180); // Spin in opposite direction
    delay(200);
    servoM3.write(90); // Stop
    delay(230);
  }

  servoM2.write(0); // Return back to the initial position
  servoM3.write(90); // Stop
  digitalWrite(13, LOW); // Relay trigger to stop the blower
  
  delay(2000);
  servoM1.detach();
  servoM2.detach();
  servoM3.detach();
}

void displayWeightLCD(double weight) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Setted Weight: ");
  lcd.setCursor(0, 1);
  lcd.print(String(feedQuantity[feedDispenseCtr].feedWeight) + " Kg");
  lcd.setCursor(0, 2);
  lcd.print("Current Weight: ");
  lcd.setCursor(0, 3);
  lcd.print(String(weight) + " Kg");
}

void displayFeedLevel() {
  lcd.setCursor(0, 2);
  lcd.print("FEED LEVEL: ");
  lcd.print(cm);
  lcd.print(" cm");
}

double getDecimalInput(int cursorPosCols, int cursorPosRow, int charLen, float maxVal) {
  int cursorPos = cursorPosCols - 1;
  int cursorPosCtr = 0;
  String container = "";
  top:
  lcd.setCursor(cursorPosCols, cursorPosRow);

  while (true) {
    char keyInput = customKeypad.getKey();

    if (keyInput == '#') {
      break; // Exit the loop if '#' is pressed
    }
    else if (isdigit(keyInput) && cursorPosCtr < charLen) {
      // Add digit to container and display on LCD
      container += keyInput;
      lcd.print(keyInput);
      cursorPosCtr++;
    }
    else if (keyInput == 'D' && cursorPosCtr < charLen) {
      // Treat 'D' as a decimal point
      container += '.';
      lcd.print('.');
      cursorPosCtr++;
    }
    else if (keyInput == '*' && cursorPosCtr > 0) {
      // Delete last input character
      lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
      lcd.print(' ');
      lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
      container.remove(container.length() - 1);
      cursorPosCtr--;
    }
  }

  double inputValue = container.toDouble();

  // Save the value when it meets the requirements
  if(inputValue > 0 && inputValue <= maxVal) {
    return inputValue;
  }
  // Input does not meet the requirements
  else {
    lcd.setCursor(0, 3);
    lcd.print("   Invalid Input!  ");

    if(cursorPosCtr <= charLen) {
      while(cursorPosCtr != 0) {
        // Deletes all the input in the LCD when input is invalid
        lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
        lcd.print(' ');
        lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
        container.remove(container.length() - 1);
        cursorPosCtr--;
      }
    }

    // Deletes back the prompt "Invalid Input!" and ask for user input again.
    if(menuFlag) {
      delay(1000);
      lcd.setCursor(0, 3);
      lcd.print("                   ");
      
      lcd.setCursor(0, 3);
      lcd.print("Enter your choice:");
    }
    else {
      delay(1000);
      lcd.setCursor(0, 3);
      lcd.print("                   ");
    }

    goto top;
  }
}

void getDataFromEEPROM() {
  /*===========================================================//
        // GETTING THE LAST UPDATED VALUES FROM THE EEPROM
  //===========================================================*/

  // The following code get the last updated values from the eeprom and store them to the designated variables
  // The following code are added so that if sudden power issues happens, the data will not be fully erase in the system
  
  // Checks if the address for the fishSchedCtr is not equal to 255, then initialized the past values in the variable
  // If flase the feedSched counter will be equal to  0
  if(EEPROM.read(12) != 255) {
    feedSchedCtr = EEPROM.read(12);
  }
  else {
    feedSchedCtr = 0;
  }

  // The following code is for retrieving the scheduled time save from the EEPROM address
  int ctr1 = 0;   // For traversing the address of EEPROM

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

  // Checks if the address for the fishScfeedWeight is not equal to 255, then initialized the past values in the variable
  // If flase the feedWeightCtr counter will be equal to  0
  if(EEPROM.read(100) != 255) {
    feedWeightCtr = EEPROM.read(100);
  }
  else {
    feedWeightCtr = 0;
  }

  // The following code is for retrieving the set weight save from the EEPROM address
  int ctr2 = 13;  // For traversing the address of EEPROM

  // Each address in the EEPROM will be read and store it in the struct
  for(int i = 0; i < feedWeightCtr; i++) {
    feedQuantity[i].feedType = EEPROM.read(ctr2);
    ctr2++;   // Increment after each retrieval to succesfully get back the updated information for all setted weight
    EEPROM.get(ctr2, feedQuantity[i].feedWeight);
    ctr2 += sizeof(double);
    feedQuantity[i].weekDuration = EEPROM.read(ctr2);
    ctr2++;
    feedQuantity[i].howManyTimesADay = EEPROM.read(ctr2);
    ctr2++;
    feedQuantity[i].totalNumOfTimes = EEPROM.read(ctr2);
    ctr2++;
    feedQuantity[i].isActivated = EEPROM.read(ctr2);
    ctr2++;
    feedQuantity[i].isFinished = EEPROM.read(ctr2);
    ctr2++;
  }

  // Checks if the address for the feedDispenseCtr is not equal to 255, then initialized the past values in the variable
  // If flase the feedDispenseCtr counter will be equal to  0
  if(EEPROM.read(101) != 255) {
    feedDispenseCtr = EEPROM.read(101);
  }
  else {
    feedDispenseCtr = 0;
  }
}

void updateFeedSchedEEPROM() {
  int addressCtr = 0;
  
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

void updateWeightEEPROM() {
  int addressCtr = 13;
  
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
