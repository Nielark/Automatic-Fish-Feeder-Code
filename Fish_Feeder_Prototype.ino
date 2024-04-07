#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>
#include <Keypad.h>
#include <Wire.h> 
#include <EEPROM.h>
#include "HX711.h"
#include "ultrasonic.h"

/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6-P0buzgr"
#define BLYNK_TEMPLATE_NAME "Fish Feeder"
#define BLYNK_AUTH_TOKEN "L1yBXQglHRyFIaZfSNA3h3FV7dJjL_Y7"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

// Your WiFi credentials.
//Set password to "" for open networks.
char ssid[] = "GlobeAtHome_E719F";
char pass[] = "qdx4xD5SBoi";
// char ssid[] = "ICT";
// char pass[] = "ict.services";

// Hardware Serial on Mega, Leonardo, Micro...
#define EspSerial Serial1

// Your ESP8266 baud rate:
#define ESP8266_BAUD 38400

ESP8266 wifi(&EspSerial);

BlynkTimer timer;

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

// Array initialization for months
String months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

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

struct custom {
  int month;
  int day;
  int hour;
  int minute;
  bool isPM;
  double feedWeight;
  bool isActivated;
  bool isFinished;
};

time feedTime[10];
custom tempSched[10];
quantity feedQuantity[10];
int feedSchedCtr = 0, feedSchedPos = -1, feedWeightCtr = 0, feedDispenseCtr = 0;
int tempSchedCtr = 0, tempSchedPos = -1, tempFeedDispenseCtr = 0;
int curHour, curYear;
bool deleteFlag = false, weightDeleteFlag = false, schedReturnFlag = false, weightReturnFlag = false, menuFlag = false, delMenuFlag = false;

WidgetLCD LCD1(V7);
WidgetLCD LCD2(V20);
int blynkHr, blynkMin, blynkUpLCD1, blynkDownLCD1, blynkUpLCD2, blynkDownLCD2, blynkScrollCtr1 = 0, blynkScrollCtr2 = 0, blynkEditInput, blynkFeedType, blynkWeekDur, blynkNumOfDispense;
double blynkFeedWeight;
bool blynkIsPM;

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
  scale.set_scale(462);  // Replace `calibrationScale` with your obtained scale value
  scale.set_offset(462);  // Replace `calibrationOffset` with your obtained offset value

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

  // Set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  // Turn off motors - Initial state
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  delay(2000);
  servoM1.detach();
  servoM2.detach();
  servoM3.detach();

  EspSerial.begin(ESP8266_BAUD);
  delay(10);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass, "blynk.cloud", 80);

  timer.setInterval(1000L, myTimer);
  timer.setInterval(1000L, displayLCD2);

  // For the feed button
  Blynk.setProperty(V8, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V9, "isDisabled", true); // Disable the delete button widget

  // For the weight button
  Blynk.setProperty(V17, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V18, "isDisabled", true); // Disable the delete button widget
}

void loop(){
  Blynk.run();
  timer.run(); 

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

    // for(int i = 0; i < tempSchedCtr; i++) {
    //   Serial.println(tempSched[i].month);
    //   Serial.println(tempSched[i].day);
    //   Serial.println(tempSched[i].hour);
    //   Serial.println(tempSched[i].minute);
    //   Serial.println(tempSched[i].isPM);
    //   Serial.println(tempSched[i].feedWeight);
    //   Serial.println(tempSched[i].isActivated);
    //   Serial.println(tempSched[i].isFinished);
    // }

    //   Serial.println(tempSched[0].month);
    //   Serial.println(tempSched[0].day);
    //   Serial.println(tempSched[0].hour);
    //   Serial.println(tempSched[0].minute);
    //   Serial.println(tempSched[0].isPM);
    //   Serial.println(tempSched[0].feedWeight);
    //   Serial.println(tempSched[0].isActivated);
    //   Serial.println(tempSched[0].isFinished);

    //   Serial.println(tempSched[9].month);
    //   Serial.println(tempSched[9].day);
    //   Serial.println(tempSched[9].hour);
    //   Serial.println(tempSched[9].minute);
    //   Serial.println(tempSched[9].isPM);
    //   Serial.println(tempSched[9].feedWeight);
    //   Serial.println(tempSched[9].isActivated);
    //   Serial.println(tempSched[9].isFinished);
  }

  if(feedWeightCtr > 0 && feedQuantity[feedWeightCtr - 1].totalNumOfTimes == 0){
    for(int i = 0; i < feedWeightCtr; i++){
      feedQuantity[i].totalNumOfTimes = (feedQuantity[i].weekDuration * 1) * feedQuantity[i].howManyTimesADay;
      feedQuantity[i].isFinished = false;
      feedDispenseCtr = 0;

      EEPROM.update(3, feedDispenseCtr);
      updateWeightEEPROM();
    }
  }

  // Condition to check the feed level of the container
  if(cm < 60) {
    digitalWrite(relayPin, LOW);
    noTone(buzzerPin);

    if(feedWeightCtr > 0){
      // Checks if the current time is equal to set time schedule to start feeding
      for (int i = 0; i < feedSchedCtr; i++) {
        if (curHour == feedTime[i].hour && 
          now.minute() == feedTime[i].minute &&
          now.second() == 0 &&
          now.isPM() == feedTime[i].isPM &&
          feedTime[i].isActivated == true)
        {
          String schedType = "permanent";
          dispenseFeed(schedType);   // Calls the function to dispense Feed
        }
      }
    }

    // Checks if the current time is equal to set time schedule to start feeding (For custom of temporary schedule)
    for (int i = 0; i < tempSchedCtr; i++) {
      if (
          now.month() == tempSched[i].month &&
          now.day() == tempSched[i].day &&
          curHour == tempSched[i].hour && 
          now.minute() == tempSched[i].minute &&
          now.second() == 0 &&
          now.isPM() == tempSched[i].isPM &&
          tempSched[i].isActivated == true &&
          tempSched[i].isFinished == false)
      {
        String schedType = "custom";
        tempFeedDispenseCtr = i;
        dispenseFeed(schedType);   // Calls the function to dispense Feed
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
        //deleteFeedSched();    // Function call for deleting schedules
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
  // For setting a custom or temporary schedule
  else if(keyInput == 'B') {
    backToMenu2:
    menuFlag = true;

    customFeedMenu();  // Function call to display temporary scheduling options

    // Ask input
    lcd.setCursor(0, 3);
    lcd.print("Enter your choice:");
    int choice = getTimeInput(19, 3, 1, 0, 3);
    menuFlag = false;

    switch(choice) {
      case 1:
        viewTemporarySched();    // Function call to display the feeding schedules
        lcd.clear();
        goto backToMenu2;
        break;

      case 2:
        addTemporarySched();   // Function call for adding feeding schedules
        lcd.clear();
        goto backToMenu2;
        break;

      case 3:
        deleteAgain2:
        deleteFlag = true;
        delMenuFlag = true;
        viewTemporarySched();
        //deleteTemporarySched();    // Function call for deleting schedules
        delMenuFlag = false;

        if(tempSchedCtr == 0 || schedReturnFlag) {
          schedReturnFlag = false;
          lcd.clear();
          goto backToMenu2;
        }

        lcd.clear();
        goto deleteAgain2;
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

/* =======================================================================================
                                        BLYNK IOT CODE
========================================================================================*/

/* ==============================
          SET SCHEDULE
================================*/

// Getting the input for hour using number widget
BLYNK_WRITE(V1){
  // Assign incoming value from pin V1 to a variable
  blynkHr = param.asInt(); 

  Serial.print("V1: ");
  Serial.println(blynkHr);
}

// Getting the input for minute using number widget
BLYNK_WRITE(V2){
  // Assign incoming value from pin V2 to a variable
  blynkMin = param.asInt(); 

  Serial.print("V2: ");
  Serial.println(blynkMin);
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
  Serial.println(blynkIsPM);
}

// Set the input time schedule and save to EEPROM
BLYNK_WRITE(V4){
  int pinValue = param.asInt(); // assigning incoming value from pin V4 to a variable
  
  // When the set button is press save the input to the structure of array when feedSched is not full and hour input is not 0
  if (pinValue == 1){
    if(feedSchedCtr < 10){
      if(blynkHr != 0){
        feedTime[feedSchedCtr].hour = blynkHr;
        feedTime[feedSchedCtr].minute = blynkMin;
        feedTime[feedSchedCtr].isPM = blynkIsPM;
        feedTime[feedSchedCtr].isActivated = true;

        feedSchedCtr++;                   // Increment the number of schedule time
        EEPROM.update(0, feedSchedCtr);   // Update the number of schedule in the EEPROM

        sortFeedSched();                  // Sort the schedule in ascending order according to time preferences

        blynkAddSuccessfulMsg();
      }
      else{
        LCD1.clear();
        LCD1.print(0, 0, "Please set hour");
        delay(3000);
        LCD1.clear();
      }
    }
   // do something when button is pressed;
  } 
  else if (pinValue == 0) {
   // do something when button is released;
  }

  // Initialize the variables back into zero
  blynkHr = 0;
  blynkMin = 0;
  blynkIsPM = 0;
  
  // Display 0 as a default in the widgets
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 2);
  
  Serial.print("V4 button value is: "); // printing value to serial monitor
  Serial.println(pinValue);
}

// For scrolling up the content of LCD1 widget
BLYNK_WRITE(V5){
  int pinValue = param.asInt(); // assigning incoming value from pin V5 to a variable
  
  if (pinValue == 1){
   blynkUpLCD1 = pinValue;
  }
  
  Serial.print("V5 button value is: "); // printing value to serial monitor
  Serial.println(blynkUpLCD1);
}

// For scrolling down the content of the LCD1 widget
BLYNK_WRITE(V6){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
   blynkDownLCD1 = pinValue;
  }
  
  Serial.print("V6 button value is: "); // printing value to serial monitor
  Serial.println(blynkDownLCD1);
}

// Selecting the respective number of the schedule
BLYNK_WRITE(V10){
  int pinValue = param.asInt(); // assigning incoming value from pin V10 to a variable

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

// Deleting the value of selected schedule
BLYNK_WRITE(V8){
  int pinValue = param.asInt(); // assigning incoming value from pin V8 to a variable
  
  if (pinValue == 1){
    for(int i = blynkEditInput; i < feedSchedCtr; i++){
      // When the 10th schedule is selected, initialize all value into 0
      if(i == 9){
        feedTime[i] = {0, 0, 0, 0};
        // feedTime[i].hour = 0;
        // feedTime[i].minute = 0;
        // feedTime[i].isPM = 0;
        // feedTime[i].isActivated = 0;
      }
      else {
        feedTime[i] = feedTime[i + 1];  // Traverse the value starting from the index of deleted schedule
      }

      updateFeedSchedEEPROM();          // Update the changes into the EEPROM
    }

    feedSchedCtr--;                     // Decrement the number of schedule
    EEPROM.update(0, feedSchedCtr);     // Update the changes of the number of schedule in EEPROM

    blynkDeleteSuccessfulMsg();
  // do something when button is pressed;
  } 
  else if (pinValue == 0) {
   // do something when button is released;
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
  
  Serial.print("V8 button value is: "); // printing value to serial monitor
  Serial.println(pinValue);
}

// Editing the schedule
BLYNK_WRITE(V9){
  int pinValue = param.asInt(); // assigning incoming value from pin V9 to a variable
  
  if (pinValue == 1){
    if(blynkHr != 0){
      feedTime[blynkEditInput].hour = blynkHr;
      feedTime[blynkEditInput].minute = blynkMin;
      feedTime[blynkEditInput].isPM = blynkIsPM;
      feedTime[blynkEditInput].isActivated = true;

      sortFeedSched();    // Sort the schedue after editing
      
      blynkEditSuccessfulMsg();
    }
    else{
      LCD1.clear();
      LCD1.print(0, 0, "Please set hour");
      delay(3000);
      LCD1.clear();
    }
  // do something when button is pressed;
  } 
  else if (pinValue == 0) {
   // do something when button is released;
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
  
  Serial.print("V9 button value is: "); // printing value to serial monitor
  Serial.println(pinValue);
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
  Serial.println(blynkFeedType);
}

// Getting the input for feed weight using number widget
BLYNK_WRITE(V12){
  // Assign incoming value from pin V12 to a variable
  blynkFeedWeight = param.asDouble(); 

  Serial.print("V12: ");
  Serial.println(blynkFeedWeight);
}

/// Getting the input for week duration using number widget
BLYNK_WRITE(V13){
  // Assign incoming value from pin V13 to a variable
  blynkWeekDur = param.asInt(); 

  Serial.print("V13: ");
  Serial.println(blynkWeekDur);
}

// Getting the input for number of dispense per day using number widget
BLYNK_WRITE(V14){
  // Assign incoming value from pin V14 to a variable
  blynkNumOfDispense = param.asInt(); 

  Serial.print("V14: ");
  Serial.println(blynkNumOfDispense);
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
        feedQuantity[feedWeightCtr].totalNumOfTimes = (blynkWeekDur * 1) * blynkNumOfDispense;
        feedQuantity[feedWeightCtr].isActivated = true;
        feedQuantity[feedWeightCtr].isFinished = true;

        feedWeightCtr++;                   // Increment the number of feed weight list
        EEPROM.update(2, feedWeightCtr);   // Update the number of feed weight in the EEPROM

        updateWeightEEPROM();              // Update changes in the EEPROM

        //blynkAddSuccessfulMsg();           // Display a successful message when the data is save
      }
      else{
        // LCD2.clear();
        // LCD2.print(0, 0, "Please set hour");
        // delay(3000);
        // LCD2.clear();
      }
    }
   // do something when button is pressed;
  } 
  else if (pinValue == 0) {
   // do something when button is released;
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
  
  Serial.print("V16 button value is: "); // printing value to serial monitor
  Serial.println(pinValue);
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
      feedQuantity[blynkEditInput].totalNumOfTimes = (blynkWeekDur * 1) * blynkNumOfDispense;
      feedQuantity[blynkEditInput].isActivated = true;
      feedQuantity[blynkEditInput].isFinished = true;

      updateWeightEEPROM();              // Update changes in the EEPROM
      
      //blynkEditSuccessfulMsg();          // Display a successful message when the data is edited
    }
    else{
      // LCD2.clear();
      // LCD2.print(0, 0, "Please set hour");
      // delay(3000);
      // LCD2.clear();
    }
  // do something when button is pressed;
  } 
  else if (pinValue == 0) {
   // do something when button is released;
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
  
  Serial.print("V17 button value is: "); // printing value to serial monitor
  Serial.println(pinValue);
}

// Deleting the value of selected weight
BLYNK_WRITE(V18){
  int pinValue = param.asInt(); // assigning incoming value from pin V8 to a variable
  
  if (pinValue == 1){
    for(int i = blynkEditInput; i < feedWeightCtr; i++){
      // When the 10th schedule is selected, initialize all value into 0
      if(i == 9){
        feedQuantity[i] = {0, 0, 0, 0, 0, 0, 0};
        // feedTime[i].hour = 0;
        // feedTime[i].minute = 0;
        // feedTime[i].isPM = 0;
        // feedTime[i].isActivated = 0;
      }
      else {
        feedQuantity[i] = feedQuantity[i + 1];  // Traverse the value starting from the index of deleted schedule
      }

      updateWeightEEPROM();              // Update changes in the EEPROM
    }

    feedWeightCtr--;                   // Increment the number of feed weight list
    EEPROM.update(2, feedWeightCtr);   // Update the number of feed weight in the EEPROM

    blynkDeleteSuccessfulMsg();
  // do something when button is pressed;
  } 
  else if (pinValue == 0) {
   // do something when button is released;
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
  
  Serial.print("V18 button value is: "); // printing value to serial monitor
  Serial.println(pinValue);
}

// Selecting the respective number of the weight
BLYNK_WRITE(V19){
  int pinValue = param.asInt(); // assigning incoming value from pin V10 to a variable

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
    Blynk.virtualWrite(V23, String(feedQuantity[blynkEditInput].totalNumOfTimes) + " remaining out of " + String(feedQuantity[blynkEditInput].weekDuration * feedQuantity[blynkEditInput].howManyTimesADay));

    // Store the value in the variables
    blynkFeedType = feedQuantity[blynkEditInput].feedType;
    blynkFeedWeight = feedQuantity[blynkEditInput].feedWeight;
    blynkWeekDur = feedQuantity[blynkEditInput].weekDuration;
    blynkNumOfDispense = feedQuantity[blynkEditInput].howManyTimesADay;
  }
}

/* ==============================
        SET CUSTOM SCHEDULE
================================*/

void myTimer(){
  // This function describes what will happen with each timer tick
  // e.g. writing sensor value to datastream V5
  Blynk.virtualWrite(V0, cm);

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

  if(blynkUpLCD1 == 1){ // Scroll up
    LCD1.clear();
    if (blynkScrollCtr1 > 0) {
      blynkScrollCtr1--;
    }

    blynkUpLCD1 = 0;
  }
  // Press 'D' to scroll the list downward
  if(blynkDownLCD1 == 1){ // Scroll down
    LCD1.clear();
    if (blynkScrollCtr1 < (feedSchedCtr - 1)) {
      blynkScrollCtr1++;
    }

    blynkDownLCD1 = 0;
  }
}

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

    // Scroll up
    if(blynkUpLCD2 == 1 && blynkScrollCtr2 > 0) {
        LCD2.clear();
        blynkScrollCtr2--;
        blynkUpLCD2 = 0;
    }

    // Scroll down
    if(blynkDownLCD2 == 1 && blynkScrollCtr2 < (feedWeightCtr - 1)) {
        LCD2.clear();
        blynkScrollCtr2++;
        blynkDownLCD2 = 0;
    }
}

// For scrolling up the content of LCD2 widget
BLYNK_WRITE(V21){
  int pinValue = param.asInt(); // assigning incoming value from pin V5 to a variable
  
  if (pinValue == 1){
   blynkUpLCD2 = pinValue;
  }
  
  Serial.print("V21 button value is: "); // printing value to serial monitor
  Serial.println(blynkUpLCD2);
}

// For scrolling down the content of the LCD2 widget
BLYNK_WRITE(V22){
  int pinValue = param.asInt(); // assigning incoming value from pin V6 to a variable
  
  if (pinValue == 1){
   blynkDownLCD2 = pinValue;
  }
  
  Serial.print("V22 button value is: "); // printing value to serial monitor
  Serial.println(blynkDownLCD2);
}

/* =======================================================================================
                          ARDUINO CODE                   
========================================================================================*/

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
    curYear = currentTime.year();
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
  

  //sortFeedSched();  // Function call to sort the time schedule before displaying into LCD

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
    
    // break the loop and ask for delete input
    // if(deleteFlag == true) {
    //   break;
    // }
    
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
      Serial.print(doubleDigit);
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= feedSchedCtr && deleteFlag == false) {
      // Select the number to edit the sched
      editSchedFlag = true;
      editInput = exitKey - '0'; // Convert char to int
      editInput += doubleDigit;

      if(editInput > 0 && editInput <= feedSchedCtr) {
        editFeedSched(editInput);
      }

      //editSchedFlag = false;
      goto top;
      // editInput = editInput - 1; 
      //break;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= feedSchedCtr && deleteFlag == true) {
      // Select the number to delete the sched
      delInput = exitKey - '0'; // Convert char to int
      delInput += doubleDigit;

      if(delInput > 0 && delInput <= feedSchedCtr) {
        deleteFeedSched(delInput);
      }

      //editInput = editInput - 1; 
      //break;
      goto top;
    }
    // else if(deleteFlag == true){
    //   break;
    // }
  }

  // if(editSchedFlag) {
    
  // }
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
    feedTime[feedSchedCtr].hour = getTimeInput(12, 1, 2, 1, 12);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(feedSchedCtr + 1));

    lcd.setCursor(0, 1);
    lcd.print("Enter Minute:");
    feedTime[feedSchedCtr].minute = getTimeInput(14, 1, 2, 0, 59);
    feedTime[feedSchedCtr].isActivated = true;

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
      feedTime[feedSchedCtr].isPM = false;
    }
    else {
      feedTime[feedSchedCtr].isPM = true;
    }

    // int tmpCtnHour = feedTime[feedSchedPos].hour;
    // int tmpCtnMin = feedTime[feedSchedPos].minute;
    // EEPROM.update(0, tmpCtnHour);
    // EEPROM.update(1, tmpCtnMin);

    if(feedSchedCtr == 0){
      feedSchedCtr++;
      EEPROM.update(0, feedSchedCtr);
      Serial.println(feedSchedCtr);

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
      Serial.println(feedSchedCtr);

      sortFeedSched();
      addSuccessfulMsg();
    }
    else{
      feedTime[feedSchedCtr] = {};

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("The same time is already set");
      delay(3000);
    }
    
    // All three schedules have been set, display a message
    // if(feedSchedCtr >= 10){
    //   lcd.clear();
    //   lcd.setCursor(0, 0);
    //   lcd.print("All 10 schedules set");
    //   delay(3000);
    // }
  }
  // All three schedules have been set, display a message
  if(feedSchedCtr >= 10){
    lcd.clear();
    lcd.setCursor(0, 0);
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
  // feedTime[editInput].hour = getTimeInput(12, 2, 2, 1, 12);
  int checkHour = getTimeInput(12, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scheduled Time:");
  lcd.setCursor(0, 1);
  fHour = (checkHour < 10 ? "0" : "") + String(checkHour);
  lcd.print(String(editInput + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM);

  lcd.setCursor(0, 2);
  lcd.print("Enter Minute:");
  //feedTime[editInput].minute = getTimeInput(14, 2, 2, 0, 59);
  int checkMinute = getTimeInput(14, 2, 2, 0, 59);
  //feedTime[editInput].isActivated = true;
  bool checkIsActivated = true;

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice:");
  morningOrAfternoon = getTimeInput(14, 2, 1, 1, 2);

  bool checkIsPM;
  if(morningOrAfternoon == 1) {
    checkIsPM = false;
  }
  else {
    checkIsPM = true;
  }

  // int tmpCtnHour = feedTime[feedSchedPos].hour;
  // int tmpCtnMin = feedTime[feedSchedPos].minute;
  // EEPROM.update(0, tmpCtnHour);
  // EEPROM.update(1, tmpCtnMin);

  if(feedSchedCtr == 0){
    Serial.println(feedSchedCtr);
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
    Serial.println(feedSchedCtr);
    feedTime[editInput] = {checkHour, checkMinute, checkIsPM, checkIsActivated};
    
    sortFeedSched();
    addSuccessfulMsg();
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("The same time is already set");
    delay(3000);
  }
}

void deleteFeedSched(int delInput) {
  // Prompt if the schedule is empty and no data to delete
  // if(feedSchedCtr == 0){
  //   lcd.setCursor(0, 1);
  //   lcd.print("Schedule is Empty");
  //   lcd.setCursor(0, 2);
  //   lcd.print("No Data to Delete");
  //   deleteFlag = false;
  //   delay(2000);
  //   return;
  // }

  // lcd.setCursor(0, 3);
  // lcd.print("Enter the number:");
  // int delInput = getTimeInput(18, 3, 1, 0, 3);

  if(delInput == 0){
    schedReturnFlag = true;
    // deleteFlag = false;
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
  int choice = getTimeInput(19, 3, 1, 1, 2);

  if(choice == 1){
    delInput = delInput - 1; // Subtract one on the number to delete to locate it using array index

    for(int i = delInput; i < feedSchedCtr; i++){
      if(i == 9){
        feedTime[i] = {0, 0, 0, 0};
        // feedTime[i].hour = 0;
        // feedTime[i].minute = 0;
        // feedTime[i].isPM = 0;
        // feedTime[i].isActivated = 0;
      }
      else {
        feedTime[i] = feedTime[i + 1];
      }

      updateFeedSchedEEPROM();
    }

    feedSchedCtr--;   // Decrement the number of schedule
    EEPROM.update(0, feedSchedCtr);
    //deleteFlag = false;
    deleteSuccessfulMsg();
  }
}

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

  //sortFeedSched();  // Function call to sort the time schedule before displaying into LCD

  // Display the feeding shedules
  while(true){
    if(tempSched[scrollCtr].isActivated) {
      // Adds leading zero for time less than 10 and convert it into string
      String date = months[tempSched[scrollCtr].month - 1] + "/" + String(tempSched[scrollCtr].day) + "/" + String(curYear);
      String fHour = (tempSched[scrollCtr].hour < 10 ? "0" : "") + String(tempSched[scrollCtr].hour);
      String fMinute = (tempSched[scrollCtr].minute < 10 ? "0" : "") + String(tempSched[scrollCtr].minute);
      String AMorPM = (tempSched[scrollCtr].isPM ? "PM" : "AM");
      String feedWeight = String(tempSched[scrollCtr].feedWeight) + " Kg";

      // lcd.print(String(i + 1) + ") " + fHour + ":" + fMinute + " " + AMorPM + "  " + feedWeight);
      lcd.setCursor(0, 0);
      lcd.print("TEMPORARY SCHED " + String(scrollCtr + 1));
      lcd.setCursor(0, 1);
      lcd.print("Date: " + date);
      lcd.setCursor(0, 2);
      lcd.print("Time: " + fHour + ":" + fMinute + " " + AMorPM);
      lcd.setCursor(0, 3);
      lcd.print("Feed Weight: " + feedWeight);
    }
    
    // break the loop and ask for delete input
    // if(deleteFlag == true) {
    //   break;
    // }

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
      Serial.print(doubleDigit);
    }
    // else if(exitKey == 'C' && deleteFlag == true) {
    //   // Call the function to delete
    //   lcd.setCursor(0, 3);
    //   lcd.print("                    ");
    //   deleteTemporarySched();
    //   lcd.clear();
    //   goto top;
    // }
    else if(isDigit(exitKey) && (exitKey - '0') <= tempSchedCtr && deleteFlag == false) {
      // Select the number to edit the sched
      editSchedFlag = true;
      editInput = exitKey - '0'; // Convert char to int
      editInput += doubleDigit;

      if(editInput > 0 && editInput <= tempSchedCtr) {
        editTemporarySched(editInput);
      }

      //editSchedFlag = false;
      goto top;
      // editInput = editInput - 1; 
      //break;
    }
    else if(isDigit(exitKey) && (exitKey - '0') <= tempSchedCtr && deleteFlag == true) {
      // Select the number to delete the sched
      delInput = exitKey - '0'; // Convert char to int
      delInput += doubleDigit;

      if(delInput <= tempSchedCtr) {
        deleteTemporarySched(delInput);
      }

      //editInput = editInput - 1; 
      //break;
      goto top;
    }
    // else if(deleteFlag == true){
    //   break;
    // }
  }

  // if(editSchedFlag) {
  //   editInput = editInput - 1;

  //   if(editInput <= tempSchedCtr) {
  //     editTemporarySched(editInput);
  //   }

  //   editSchedFlag = false;
  //   goto top;
  // }
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
    tempSched[tempSchedCtr].month = getTimeInput(13, 1, 2, 1, 12);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));
    
    lcd.setCursor(0, 1);
    lcd.print("Enter Day:");
    tempSched[tempSchedCtr].day = getTimeInput(11, 1, 2, 1, 31);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));
    
    lcd.setCursor(0, 1);
    lcd.print("Enter Hour:");
    tempSched[tempSchedCtr].hour = getTimeInput(12, 1, 2, 1, 12);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Schedule " + String(tempSchedCtr + 1));

    lcd.setCursor(0, 1);
    lcd.print("Enter Minute:");
    tempSched[tempSchedCtr].minute = getTimeInput(14, 1, 2, 0, 59);

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
      Serial.println(tempSchedCtr);

      updateTemporarySchedEEPROM();
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
      Serial.println(tempSchedCtr);

      updateTemporarySchedEEPROM();
      addSuccessfulMsg();
    }
    else{
      tempSched[tempSchedCtr] = {};

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("The same time is already set");
      delay(3000);
    }

    // All ten schedules have been set, display a message
    // if(tempSchedCtr >= 10){
    //   lcd.clear();
    //   lcd.setCursor(0, 0);
    //   lcd.print("All 10 schedules set");
    //   delay(3000);
    // }
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
  //tempSched[editInput].month = getTimeInput(13, 2, 2, 1, 12);
  int checkMonth = getTimeInput(13, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  date = months[checkMonth - 1] + "/" + String(tempSched[editInput].day) + "/" + String(curYear);
  lcd.print("Date: " + date);

  // lcd.clear();
  // lcd.setCursor(0, 0);
  // lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  // lcd.setCursor(0, 1);
  // lcd.print("Date: " + date);

  lcd.setCursor(0, 2);
  lcd.print("Enter Day:");
  //tempSched[editInput].day = getTimeInput(11, 2, 2, 1, 31);
  int checkDay = getTimeInput(11, 2, 2, 1, 31);

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
  //tempSched[editInput].hour = getTimeInput(12, 2, 2, 1, 12);
  int checkHour = getTimeInput(12, 2, 2, 1, 12);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMPORARY SCHED " + String(editInput + 1));
  lcd.setCursor(0, 1);
  fHour = (checkHour < 10 ? "0" : "") + String(checkHour);
  lcd.print(String(editInput + 1) + ") " + checkHour + ":" + fMinute + " " + AMorPM);

  lcd.setCursor(0, 2);
  lcd.print("Enter Minute:");
  //tempSched[editInput].minute = getTimeInput(14, 2, 2, 0, 59);
  int checkMinute = getTimeInput(14, 2, 2, 0, 59);

  // Selection for AM and PM
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1] - AM");
  lcd.setCursor(0, 1);
  lcd.print("[2] - PM");
  lcd.setCursor(0, 2);
  lcd.print("Enter choice:");
  morningOrAfternoon = getTimeInput(14, 2, 1, 1, 2);

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
  //tempSched[editInput].feedWeight = getDecimalInput(0, 3, 4, 5);
  double checkFeedWeight = getDecimalInput(0, 3, 4, 5);

  // tempSched[editInput].isActivated = true;
  // tempSched[editInput].isFinished = false;
  bool checkIsActivated = true;
  bool checkIsFinished = false;


  // int tmpCtnHour = feedTime[feedSchedPos].hour;
  // int tmpCtnMin = feedTime[feedSchedPos].minute;
  // EEPROM.update(0, tmpCtnHour);
  // EEPROM.update(1, tmpCtnMin);

  if(tempSchedCtr == 0){
    Serial.println(tempSchedCtr);
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
    Serial.println(tempSchedCtr);
    tempSched[editInput] = {checkMonth, checkDay, checkHour, checkMinute, checkIsPM, checkFeedWeight, checkIsActivated, checkIsFinished};
    
    updateTemporarySchedEEPROM();
    editSuccessfulMsg();
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("The same time is already set");
    delay(3000);
  }
}

void deleteTemporarySched(int delInput) {
  Serial.print(delInput);
  // lcd.setCursor(0, 3);
  // lcd.print("Enter the number:");
  // int delInput = getTimeInput(18, 3, 2, 0, tempSchedCtr);

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
  int choice = getTimeInput(19, 3, 1, 1, 2);

  if(choice == 1){
    delInput = delInput - 1; // Subtract one on the number to delete to locate it using array index

    for(int i = delInput; i < tempSchedCtr; i++){
      if(i == 9){
        tempSched[i] = {0, 0, 0, 0, 0, 0, 0, 0};
        // tempSched[i].month = 0;
        // tempSched[i].day = 0;
        // tempSched[i].hour = 0;
        // tempSched[i].minute = 0;
        // tempSched[i].isPM = 0;
        // tempSched[i].feedWeight = 0;
        // tempSched[i].isActivated = 0;
        // tempSched[i].isFinished = 0;
      }
      else {
        tempSched[i] = tempSched[i + 1];
      }

      updateTemporarySchedEEPROM();
    }

    tempSchedCtr--;   // Decrement the number of schedule
    EEPROM.update(1, tempSchedCtr);
    //deleteFlag = false;
    deleteSuccessfulMsg();
  }
}

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
      Serial.print(doubleDigit);
    }
    else if(isDigit(inputKey) && (inputKey - '0') <= feedWeightCtr && weightDeleteFlag == false) {
      editWeightFlag = true;
      editInput = inputKey - '0'; // Convert char to int
      editInput -= 1;
      editInput += doubleDigit;
      editWeightSched(editInput);
      goto top;
    }
    else if(isDigit(inputKey) && (inputKey - '0') <= feedWeightCtr && weightDeleteFlag == true) {
      delInput = inputKey - '0'; // Convert char to int
      delInput += doubleDigit;

      if(delInput <= feedWeightCtr) {
        deleteFeedWeight(delInput);
      }

      goto top;
    }
  }

  // if(editWeightFlag && !weightDeleteFlag) {
  //   editWeightSched(editInput);
  //   editWeightFlag = false;
  //   goto top;
  // }
  // else if(!editWeightFlag && weightDeleteFlag) {
  //   deleteFeedWeight();   // Function call to delete a particular feed weight
  // }
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
  feedQuantity[editInput].feedType = getTimeInput(19, 3, 1, 1, 3);

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
  feedQuantity[editInput].weekDuration = getTimeInput(0, 3, 2, 1, 10);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current Set Weight:");
  lcd.setCursor(0, 1);
  lcd.print(String(editInput + 1) + ") " + feedType);
  lcd.setCursor(13, 1);
  lcd.print(String(feedQuantity[editInput].feedWeight) + " kg");
  lcd.setCursor(0, 2);
  lcd.print("Enter how many times");
  lcd.setCursor(0, 3);
  lcd.print("a day:");
  feedQuantity[editInput].howManyTimesADay = getTimeInput(7, 3, 1, 1,9);

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
    int input = getTimeInput(19, 3, 1, 0, 3);

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
    feedQuantity[feedWeightCtr].weekDuration = getTimeInput(0, 1, 2, 1, 10);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter how many times");
    lcd.setCursor(0, 1);
    lcd.print("a day:");
    feedQuantity[feedWeightCtr].howManyTimesADay = getTimeInput(7, 1, 1, 1,9);

    feedQuantity[feedWeightCtr].totalNumOfTimes = (feedQuantity[feedWeightCtr].weekDuration * 1) * feedQuantity[feedWeightCtr].howManyTimesADay;
    feedQuantity[feedWeightCtr].isActivated = true;
    feedQuantity[feedWeightCtr].isFinished = false;

    feedWeightCtr++;
    EEPROM.update(2, feedWeightCtr);

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
  // lcd.setCursor(0, 3);
  // lcd.print("Enter the number:");
  // int delInput = getTimeInput(18, 3, 1, 0, feedWeightCtr);

  if(delInput == 0){
    weightReturnFlag = true;
    weightDeleteFlag = false;
    delMenuFlag = false;
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
  int choice = getTimeInput(19, 3, 1, 1, 2);

  if(choice == 1){
    delInput = delInput - 1; // Subtract one on the number to delete to locate it using array index

    for(int i = delInput; i < feedWeightCtr; i++){
      if(i == 9){
        feedQuantity[i].feedType = 0;
        feedQuantity[i].feedWeight = 0;
        feedQuantity[i].weekDuration = 0;
        feedQuantity[i].howManyTimesADay = 0;
        feedQuantity[i].totalNumOfTimes = 0;
        feedQuantity[i].feedWeight = 0;
        feedQuantity[i].isActivated = 0;
        feedQuantity[i].isFinished = 0;
      }
      else {
        feedQuantity[i] = feedQuantity[i + 1];
      }

      updateWeightEEPROM();
    }

    feedWeightCtr--;   // Decrement the number of schedule
    EEPROM.update(2, feedWeightCtr);
    deleteSuccessfulMsg();
  }



  // for(int i = 0; i < feedWeightCtr; i++){
  //   // Condition to find if the information from the input match the selected weight list to delete
  //   if(feedQuantity[i].feedType == feedQuantity[delInput - 1].feedType && 
  //     feedQuantity[i].feedWeight == feedQuantity[delInput - 1].feedWeight &&
  //     feedQuantity[i].howManyTimesADay == feedQuantity[delInput - 1].howManyTimesADay &&
  //     feedQuantity[i].totalNumOfTimes == feedQuantity[delInput - 1].totalNumOfTimes &&
  //     feedQuantity[i].isActivated == feedQuantity[delInput - 1].isActivated &&
  //     feedQuantity[i].isFinished == feedQuantity[delInput - 1].isFinished)
  //   {
  //     // Traverse the position of array elements for deletion
  //     for(int j = i; j < feedWeightCtr; j++){
  //       feedQuantity[j].feedType = feedQuantity[j + 1].feedType;
  //       feedQuantity[j].feedWeight = feedQuantity[j + 1].feedWeight;
  //       feedQuantity[j].howManyTimesADay = feedQuantity[j + 1].howManyTimesADay;
  //       feedQuantity[j].totalNumOfTimes = feedQuantity[j + 1].totalNumOfTimes;
  //       feedQuantity[j].isActivated = feedQuantity[j + 1].isActivated;
  //       feedQuantity[j].isFinished = feedQuantity[j + 1].isFinished;
  //     }

  //     updateWeightEEPROM();

  //     feedWeightCtr--;  // Decrement the number of weight list
  //     EEPROM.update(100, feedWeightCtr);
  //     break;
  //   }
  // }

  weightDeleteFlag = true;
  delMenuFlag = true;
}

void dispenseFeed(String schedType) {
  servoM1.attach(servoPin1);
  int feedDivideCtr = 0;
  int feedDivisor = 1;

  anotherDispense:
  double weight = scale.get_units();  // Get weight measurement in the desired unit

  // M1
  // Drop feed from M1 to M2
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("M1 PROCESSING");
  servoM1.write(180);
  
  if(schedType == "permanent") {
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

      if(feedQuantity[feedDispenseCtr].feedType == 3) {
        feedDivisor = 2;
        feedDivideCtr++;
      }

      while(weight < feedQuantity[feedDispenseCtr].feedWeight / feedDivisor) {
        weightCurrTime = millis();
        weight = scale.get_units();  // Get weight measurement in the desired unit
        if(weightCurrTime - weightPrevTime >= weightTimer) {
          displayWeightLCD(weight, feedDivisor, schedType);   // Function call to display the current feed weight on the LCD

          weightPrevTime = weightCurrTime;
        }
      }

      servoM2.attach(servoPin2);
      servoM3.attach(servoPin3);
      // EEPROM.update(101, feedDispenseCtr);

      // feedQuantity[feedDispenseCtr].totalNumOfTimes--;

      // updateWeightEEPROM();
      
      servoM1.write(0);
    }
  }
  else {
    // TODO:
    //    1. Condition to stop M1 to drop feeds when load cell limit set weight was acheive
    servoM2.detach();
    const unsigned long weightTimer = 1000;
    unsigned long weightPrevTime = 0;
    unsigned long weightCurrTime = 0;

    // if(tempSched[tempSchedCtr].feedType == 3) {
    //   feedDivisor = 2;
    //   feedDivideCtr++;
    // }

    while(weight < tempSched[tempFeedDispenseCtr].feedWeight / feedDivisor) {
      weightCurrTime = millis();
      weight = scale.get_units();  // Get weight measurement in the desired unit
      if(weightCurrTime - weightPrevTime >= weightTimer) {
        displayWeightLCD(weight, feedDivisor, schedType);   // Function call to display the current feed weight on the LCD

        weightPrevTime = weightCurrTime;
      }
    }

    servoM2.attach(servoPin2);
    servoM3.attach(servoPin3);
    // EEPROM.update(101, tempSchedCtr);

    // tempSched[tempSchedCtr].totalNumOfTimes--;

    // updateWeightEEPROM();
    
    servoM1.write(0);
  }
  
  //delay(5000);
  
  displayWeightLCD(weight, feedDivisor, schedType);   // Function call to display the current feed weight on the LCD
  
  delay(3000);

  // TODO: Blower code at this part
  digitalWrite(13, HIGH); // Relay trigger to start the blower

  lcd.setCursor(0, 0);
  lcd.println("M2 PROCESSING");
  // Drop feed from M2 to M3
  //servoM2.write(180); // Drop the feeds to M3
  int servoM2Pos = 0;
  int servoM3Pos = 0;
  const unsigned long dropTimer = 10;
  unsigned long dropPrevTime = 0;
  unsigned long dropCurrTime = 0;

  // M3
  // Dispensed feed and swing left and right
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("M3 PROCESSING");
  for (int i = 0; i < 20; i++) {
    dropCurrTime = millis();
    if (dropCurrTime - dropPrevTime >= dropTimer && servoM2Pos != 180) {
        servoM2Pos += 20;
        servoM2.write(servoM2Pos);
        dropPrevTime = dropCurrTime;
    }

    if (i % 2 == 0) {
        servoM3Pos = 0; //Spin in one direction
    }
    else {
        servoM3Pos = 180; // Spin in opposite direction
    }

    servoM3.write(servoM3Pos);
    delay(500);
  }

  servoM2.write(0); // Return back to the initial position
  servoM3.write(90); // Stop
  digitalWrite(13, LOW); // Relay trigger to stop the blower
  
  delay(2000);
  servoM1.detach();
  servoM2.detach();
  servoM3.detach();

  if(feedDivideCtr != 2 && feedDivisor == 2) {
    goto anotherDispense;
  }

  if(schedType == "permanent") {
    EEPROM.update(3, feedDispenseCtr);

    feedQuantity[feedDispenseCtr].totalNumOfTimes--;

    updateWeightEEPROM();
  }
  else{
    for(int i = tempFeedDispenseCtr; i < tempSchedCtr; i++){
      //updateFeedSchedEEPROM();
      if(i == 9){
        tempSched[i].month = 0;
        tempSched[i].day = 0;
        tempSched[i].hour = 0;
        tempSched[i].minute = 0;
        tempSched[i].isPM = 0;
        tempSched[i].feedWeight = 0;
        tempSched[i].isActivated = 0;
        tempSched[i].isFinished = 0;
      }
      else {
        tempSched[i] = tempSched[i + 1];
      }
    }

    tempSchedCtr--;   // Decrement the number of schedule
    //EEPROM.update(12, tempSchedCtr);
  }

  lcd.clear();
}

void displayWeightLCD(double weight, int feedDivisor, String schedType) {
  lcd.setCursor(0, 0);
  lcd.print("Setted Weight: ");
  lcd.setCursor(0, 1);
  if(schedType == "permanent") {
    if(feedDivisor == 1) {
      lcd.print(String(feedQuantity[feedDispenseCtr].feedWeight) + " Kg");
    }
    else {
      lcd.print(String(feedQuantity[feedDispenseCtr].feedWeight / feedDivisor) + " Kg");
    }
  }
  else if (schedType == "custom") {
    if(feedDivisor == 1) {
      lcd.print(String(tempSched[tempFeedDispenseCtr].feedWeight) + " Kg");
    }
    else{
      lcd.print(String(tempSched[tempFeedDispenseCtr].feedWeight / feedDivisor) + " Kg");
    }
  }
  lcd.setCursor(0, 2);
  lcd.print("Current Weight: ");
  lcd.setCursor(0, 3);
  lcd.print(weight);

  if(weight < 0.00) {
    lcd.setCursor(5, 3);
    lcd.print(" ");
    lcd.setCursor(6, 3);
  }
  else {
    lcd.setCursor(4, 3);
    lcd.print(" ");
    lcd.setCursor(7, 3);
    lcd.print(" ");
    lcd.setCursor(5, 3);  
  }
  
  lcd.print("Kg");
}

void displayFeedLevel() {
  lcd.setCursor(0, 2);
  lcd.print("FEED LEVEL: ");
  lcd.print(cm);
  lcd.print(" cm");
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

void customFeedMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[1]-View");
  lcd.setCursor(0, 1);
  lcd.print("[2]-Add");
  lcd.setCursor(0, 2);
  lcd.print("[3]-Delete");
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

void blynkAddSuccessfulMsg(){
  LCD1.clear();
  LCD1.print(0, 0, "Added Successfully");
  delay(800);
  LCD1.clear();
}

void blynkEditSuccessfulMsg(){
  LCD1.clear();
  LCD1.print(0, 0, "edited Successfully");
  delay(800);
  LCD1.clear();
}

void blynkDeleteSuccessfulMsg(){
  LCD1.clear();
  LCD1.print(0, 0, "Deleted Successfully");
  delay(800);
  LCD1.clear();
}

void addSuccessfulMsg(){
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("Added Successfully");
  delay(800);
}

void editSuccessfulMsg(){
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("edited Successfully");
  delay(800);
}

void deleteSuccessfulMsg(){
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Deleted Successfully");
  delay(800);
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
    else if (keyInput == 'B') {
      // Treat 'B' as a back or return
      keyInput = '0';
      container = "";
      container += keyInput;
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
  Serial.println("ctr: " + String(ctr1));

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
    EEPROM.get(ctr2, feedQuantity[i].feedWeight);
    ctr2 += sizeof(double);
    tempSched[i].isActivated = EEPROM.read(ctr2);
    ctr2++;
    tempSched[i].isFinished = EEPROM.read(ctr2);
    ctr2++;
  }
  Serial.println("ctr: " + String(ctr2));

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
    Serial.println("ctr: " + String(ctr3));
  }
  Serial.println("ctr: " + String(ctr3));
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
