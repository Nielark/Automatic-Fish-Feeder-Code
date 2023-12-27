#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>
#include <Keypad.h>
#include <Wire.h> 
#include <EEPROM.h>
#include "ultrasonic.h"

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

const byte ROWS = 4;  //Four rows
const byte COLS = 4;  //Four columns

char daysOfTheWeek[7][12] = {
  "Sun",
  "Mon",
  "Tue",
  "Wed",
  "Thu",
  "Fri",
  "Sat"
};

// Define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {22, 24, 26, 28};  // Connect to the row pinouts of the keypad
byte colPins[COLS] = {30, 32, 34, 36};  // Connect to the column pinouts of the keypad

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

time feedTime[3];
int feedSchedCtr = 0, feedSchedPos = -1;
int curHour;
bool deleteFlag = false;

void setup(){
  Serial.begin(9600);
  rtc.begin();
  
  // Set initial time (adjust as needed)
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  lcd.init();
	lcd.backlight();
	lcd.clear();
	lcd.setCursor(0,0);

  // Set pins to output pins
  pinMode(13, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);
  servoM3.attach(servoPin3);

  // Set the initial state of servo motor into 0 degree
  servoM1.write(0);
  servoM2.write(0);
  servoM3.write(90);
}

void loop(){
  // feedTime[0].hour = EEPROM.read(0);
  // feedTime[0].minute = EEPROM.read(1);
  char keyInput = customKeypad.getKey();  // For getting keypad inputs
  DateTime now = rtc.now();

  // ULTRASONIC SENSOR READING
  // measure the ping time in cm
  cm = 0.01723 * readUltrasonicDistance(triggerPin, echoPin);
  
  // convert to inches by dividing by 2.54
  inches = (cm / 2.54);

  displayTime(now); // Function call to display current time
  displayFeedLevel(); // Function call to display feed level

  // Condition to check the feed level of the container
  if(cm < 60) {
    noTone(buzzerPin);
    // Checks if the current time is equal to set time schedule to start feeding
    for (int i = 0; i < 3; i++) {
      if (curHour == feedTime[i].hour && 
          now.minute() == feedTime[i].minute &&
          now.second() == 0 &&
          now.isPM() == feedTime[i].isPM &&
          feedTime[i].isActivated == true)
      {
        dispenseFeed();
      }
    }
  }
  else {
    tone(buzzerPin, 250);
  }
  

  if (keyInput == 'D'){
    setTime();  // Function call to set time 
  }
  else if(keyInput == 'A') {
    backToMenu:
    feedSchedMenu();  // Function call to display scheduling options

    // Ask input
    lcd.setCursor(0, 3);
    lcd.print("Enter your choice:");
    int choice = getTimeInput(19, 3, 1, 3);

    switch(choice) {
      case 1:
        viewSched();    // Function call to display the feeding schedules
        goto backToMenu;
        break;

      case 2:
        addFeedSched();   // Function call for adding feeding schedules 
        goto backToMenu;
        break;

      case 3:
        deleteFlag = true;
        viewSched();
        deleteFeedSched();    // Function call for deleting schedules
        goto backToMenu;
        break;
      
      case 0:
        return;
        break;
    }
  }
}

void displayTime(DateTime currentTime) {
  displayCurTime = millis();
  
  if(displayCurTime - displayPrevTime >= displayTimer) {
    lcd.clear();

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
    curHour = currentTime.twelveHour();
    // lcd.setCursor(0, 3);
    // lcd.print(currentTime.hour());

    lcd.setCursor(0, 1);
    lcd.print("TIME:");

    lcd.setCursor(6, 1);
    if(curHour < 10) {
      lcd.print("0");
    }
    lcd.print(curHour, DEC);
    lcd.print(":");


    lcd.setCursor(9, 1);
    if(currentTime.minute() < 10) {
      lcd.print("0");
    }
    lcd.print(currentTime.minute(), DEC);
    lcd.print(":");

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
}

void setTime() {
  int morningOrAfternoon;
  // Set the date
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Month: ");
  int month = getTimeInput(13, 0, 2, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Day Of Week: ");
  int day = getTimeInput(0, 1, 2, 31);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Year: ");
  int year = getTimeInput(12, 0, 4, 9000);

  // Set the time
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Hours: ");
  int hour = getTimeInput(13, 0, 2, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Minutes: ");
  int minute = getTimeInput(15, 0, 2, 59);

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice: ");
  morningOrAfternoon = getTimeInput(14, 2, 1, 2);

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

  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("Enter Seconds: ");
  // int second = getTimeInput();

  // Set the date and time
  rtc.adjust(DateTime(year, month, day, hour, minute));
}

void viewSched(){
  lcd.clear();

  sortFeedSched();

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
    else if(deleteFlag == true){
      break;
    }
  }
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
        feedTime[feedSchedPos].hour = getTimeInput(12, 1, 2, 12);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Schedule " + String(feedSchedPos + 1));

        lcd.setCursor(0, 1);
        lcd.print("Enter Minute:");
        feedTime[feedSchedPos].minute = getTimeInput(14, 1, 2, 59);
        feedTime[feedSchedPos].isActivated = true;

        // Selection for AM and PM
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("[1] - AM");
        lcd.setCursor(0, 1);
        lcd.print("[2] - PM");
        lcd.setCursor(0, 2);
        lcd.print("Enter choice: ");
        morningOrAfternoon = getTimeInput(14, 2, 1, 2);

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
        break;
      }
    }
    
    if (feedSchedCtr >= 3) {
      // All three schedules have been set, display a message
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("All 3 schedules set");
      delay(3000);
    }
  }
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All 3 schedules set");
    delay(3000);
  }

  feedSchedPos = -1;
}

void deleteFeedSched() {
  lcd.setCursor(0, 3);
  lcd.print("Enter the number: ");
  int delInput = getTimeInput(18, 3, 1, 3);

  for(int i = 0; i < feedSchedCtr; i++){
    if(feedTime[i].hour == feedTime[delInput - 1].hour && 
      feedTime[i].minute == feedTime[delInput - 1].minute)
    {
      // Traverse the position of array elements after deletion
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

      feedSchedCtr--;
      break;
    }
  }
  
  deleteFlag = false;
}

int getTimeInput(int cursorPosCols, int cursorPosRow, int charLen, int maxVal) {
  int cursorPos = cursorPosCols - 1;
  int cursorPosCtr = 0;
  String container = "";
  top:
  lcd.setCursor(cursorPosCols, cursorPosRow);

  while(true) {
    char keyInput = customKeypad.getKey();
    
    // Enter the Input
    if(keyInput == '#') {
      break;
    } 
    else if(isDigit(keyInput) && cursorPosCtr < charLen) {
        container += keyInput;
        lcd.print(keyInput);
        cursorPosCtr++;
    }
    // Delete input characters 
    else if(keyInput == '*' && cursorPosCtr > 0) {
      lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
      lcd.print(' ');
      lcd.setCursor(cursorPos + cursorPosCtr, cursorPosRow);
      container.remove(container.length() - 1);
      cursorPosCtr--;
    }
  }
  
  // Save the value when it meets the requirements
  if(container != "" && container.toInt() >= 0 && container.toInt() <= maxVal) {
    return container.toInt();
  }
  // Input does not meet the requirements
  else {
    lcd.setCursor(0, 3);
    lcd.print("Invalid Input");
    goto top;
  }
}

void sortFeedSched(){
  int pos, pos2;
  int len = 3;
  int hourMinVal, minuteMinVal;
  bool isPM;
  bool isActivated;

  if(feedSchedCtr > 1){
    // Start of AM
    for(int i = 0; i < feedSchedCtr; i++){
      hourMinVal = 12;
      minuteMinVal = 59;
      for(int j = i; j < feedSchedCtr; j++){
        if(!feedTime[j].isPM){
          if(feedTime[j].hour < hourMinVal || feedTime[j].minute < minuteMinVal){
            hourMinVal = feedTime[j].hour;
            minuteMinVal = feedTime[j].minute;
            isPM = feedTime[j].isPM;
            isActivated = feedTime[j].isActivated;
            pos = j;
          }

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
      }
    } // End of AM

    // Start of PM
    for(int i = pos2; i < feedSchedCtr; i++){
      hourMinVal = 12;
      minuteMinVal = 59;
      for(int j = i; j < feedSchedCtr; j++){
        if(feedTime[j].isPM){
          if(feedTime[j].hour < hourMinVal || feedTime[j].minute < minuteMinVal){
            hourMinVal = feedTime[j].hour;
            minuteMinVal = feedTime[j].minute;
            isPM = feedTime[j].isPM;
            isActivated = feedTime[j].isActivated;
            pos = j;
          }

          feedTime[pos].hour = feedTime[i].hour;
          feedTime[i].hour = hourMinVal;

          feedTime[pos].minute = feedTime[i].minute;
          feedTime[i].minute = minuteMinVal;

          feedTime[pos].isPM = feedTime[i].isPM;
          feedTime[i].isPM = isPM;

          feedTime[pos].isActivated = feedTime[i].isActivated;
          feedTime[i].isActivated = isActivated;
        }
      }
    } // End of PM
  }
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

void dispenseFeed() {
  // M1
  // Drop feed from M1 to M2
  servoM1.write(180);
  delay(5000);
  servoM1.write(0);
  delay(3000);

  // TODO: Blower code at this part
  digitalWrite(13, HIGH); // Relay trigger to start the blower

  // Drop feed from M2 to M3
  servoM2.write(180); // Drop the feeds to M3

  // M3
  // Dispensed feed and swing left and right
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
}

void displayFeedLevel() {
  lcd.setCursor(0, 2);
  lcd.print("FEED LEVEL: ");
  lcd.print(cm);
  lcd.print(" cm");
}