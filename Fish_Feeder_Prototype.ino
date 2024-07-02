#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <RTClib.h>
#include <Keypad.h>
#include <Wire.h> 
#include <EEPROM.h>
#include "HX711.h"
#include "ultrasonic.h"
#include "DOsensor.h"

#include "KeypadInput.h"
#include "struct.h"
#include "blynkCode.h"
#include "sortSched.h"
#include "eeprom1.h"
#include "feedSchedule.h"
#include "customSchedule.h"
#include "feedWeight.h"
#include "timeSetUp.h"

#include "Tone440.h"      // A library for tone pitch that uses 440 Hz frequency
#define TONE_USE_DOUBLE   // Define to use decimal type frequencies from the library

/* Fill-in information from Blynk Device Info here */
// #define BLYNK_TEMPLATE_ID "TMPL6-P0buzgr"
// #define BLYNK_TEMPLATE_NAME "Fish Feeder"
#define BLYNK_AUTH_TOKEN "L1yBXQglHRyFIaZfSNA3h3FV7dJjL_Y7"

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#include <ESP8266_Lib.h>
// #include <BlynkSimpleShieldEsp8266.h>

// Your WiFi credentials.
//Set password to "" for open networks.
// char ssid[] = "GlobeAtHome_E719F";
// char pass[] = "qdx4xD5SBoi";
// char ssid[] = "GlobeAtHome_E719F_EXT";
// char pass[] = "qdx4xD5SBoi";
// char ssid[] = "ICT";
// char pass[] = "ict.services";

// char ssid[] = "iCARE";
// char pass[] = "ICARE2023";

char ssid[] = "FishFeeder";
char pass[] = "fishfeeder"; 

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
double DOlevel = 0;

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

byte arrowUp[8] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000
};

byte arrowDown[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00000
};

const unsigned long displayTimer = 1000;
unsigned long displayPrevTime = 0;
unsigned long displayCurTime = 0;

#define DO_PIN A1
 
#define VREF 5000    //VREF (mv)
#define ADC_RES 1024 //ADC Resolution
 
#define READ_TEMP (25) //Current water temperature ℃, Or temperature sensor function
 
uint8_t Temperaturet;
uint16_t ADC_Raw;
uint16_t ADC_Voltage;
uint16_t DO;

Time feedTime[10];
Custom tempSched[10];
Quantity feedQuantity[10];
int feedSchedCtr = 0, feedWeightCtr = 0, feedDispenseCtr = 0;
int tempSchedCtr = 0, tempFeedDispenseCtr = 0;
int curHour, curYear;
int feedWarningCtr = 0;
bool deleteFlag = false, weightDeleteFlag = false, schedReturnFlag = false, weightReturnFlag = false, menuFlag = false, delMenuFlag = false;

WidgetLCD LCD1(V7);
WidgetLCD LCD2(V20);
WidgetLCD LCD3(V34);

// Variable declaration for time schedule in blynk
int blynkHr, blynkMin, blynkUpLCD1, blynkDownLCD1, blynkScrollCtr1 = 0, blynkEditInput;
bool blynkIsPM;

// Variable declaration for weight in blynk
double blynkFeedWeight;
int blynkUpLCD2, blynkDownLCD2, blynkScrollCtr2 = 0, blynkFeedType, blynkWeekDur, blynkNumOfDispense;

// Variable declaration for custom schedule in blynk
int blynkUpLCD3, blynkDownLCD3, blynkScrollCtr3 = 0, blynkMonth = 0, blynkDate = 0, blynkCustomHr = 0, blynkCustomMin = 0;
bool blynkCustomIsPM;
double blynkCustomFeedWeight = 0;

double customWeight = 0, tot = 0;
int option;

void setup(){
  Serial.begin(38400);
  rtc.begin();
  scale.begin(pinDT, pinSCK);

  getDataFromEEPROM();  // Function call for getting the last updated values in the EEPROM
  
  // Set initial time (adjust as needed)
  // Uncomment this code and set the values if you want to set the date and time
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2024, 1, 4, 11, 37));

  // Use the obtained scale and offset values from calibration
  scale.set_scale(407.699493);  // Replace `calibrationScale` with your obtained scale value
  scale.set_offset(4294949407);  // Replace `calibrationOffset` with your obtained offset value

  componentsSetUp();

  lcd.init();
	lcd.backlight();
	lcd.clear();
  //startBuzzerSound();
  showDeviceName();
  playPacman();
  //askBlynkConnection();

  // EspSerial.begin(ESP8266_BAUD);
  // delay(10);
  
  // if(option == 1){
  //   Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass, "blynk.cloud", 80);
  // }

  // blynkButtonSetUp();

  // timer.setInterval(1000L, myTimer);

  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);

  lcd.clear();
}

void loop(){
  // if(option == 1){
  //   Blynk.run();
  //   timer.run();
  // }

  Temperaturet = (uint8_t)READ_TEMP;
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = uint32_t(VREF) * ADC_Raw / ADC_RES;
  
  char keyInput = customKeypad.getKey();  // For getting keypad inputs
  DateTime now = rtc.now();

  displayCurTime = millis();
  
  if(displayCurTime - displayPrevTime >= displayTimer) {
    // ULTRASONIC SENSOR READING
    // measure the ping time in cm
    cm = 0.01723 * readUltrasonicDistance(triggerPin, echoPin);
    cm = 50 - cm;
    // convert to inches by dividing by 2.54
    inches = (cm / 2.54);

    DOlevel = readDO(ADC_Voltage, Temperaturet) / 1000;

    // if(DOlevel > 5 && DOlevel < 5.5){
    //   Blynk.logEvent("dissolve_oxygen_status", "Warning: DO level is at " + String(DOlevel) + " ppm. Ensure DO level stays above 5 ppm.");
    // }
    // else if(DOlevel < 5){
    //   Blynk.logEvent("dissolve_oxygen_status", "Alert: DO level is below 5 ppm. Oxygen depletion can be harmful to aquatic life. Take immediate action to aerate the water.");
    // }

    if(cm <= 5 && cm > 0 && feedWarningCtr != 5){
      // Blynk.logEvent("low_feed_level", "Feed level is at " + String(cm) + " cm, Please Refill Soon.");
      tone(buzzerPin, 700);
      delay(100);
      noTone(buzzerPin);
      delay(100);
      tone(buzzerPin, 700);
      delay(100);
      noTone(buzzerPin);

      feedWarningCtr++;
    }
    else if(cm > 5){
      feedWarningCtr = 0;
    }

    if(cm > 0){
      displayTime(now); // Function call to display current time
      displayFeedLevel(); // Function call to display feed level
      displayDissolvedOxygenLevel();  // Function call to display dissolved oxygen level
    }
    else{
      lcd.clear();
      // Blynk.logEvent("feed_container_is_empty", "🐟 Please refill the container so that your fish receive their scheduled feedings.");
      tone(buzzerPin, 300, 500);

      lcd.setCursor(0, 1);
      lcd.print("   LOW FEED LEVEL");
      lcd.setCursor(0, 2);
      lcd.print(" FEED LEVEL: ");
      lcd.print(cm);
      lcd.print(" cm");
      delay(1000);
      lcd.clear();
    }
  }

  if(feedWeightCtr > 0 && feedQuantity[feedWeightCtr - 1].totalNumOfTimes == 0){
    for(int i = 0; i < feedWeightCtr; i++){
      feedQuantity[i].totalNumOfTimes = (feedQuantity[i].weekDuration * 7) * feedQuantity[i].howManyTimesADay;
      feedQuantity[i].isFinished = false;
      feedDispenseCtr = 0;

      EEPROM.update(3, feedDispenseCtr);
      updateWeightEEPROM();
    }
  }

  // Condition to check the feed level of the container
  if(cm > 0) {
    noTone(buzzerPin);

    if(feedWeightCtr > 0){
      // Checks if the current time is equal to set time schedule to start feeding
      for (int i = 0; i < feedSchedCtr; i++) {
        if (now.twelveHour() == feedTime[i].hour && 
          now.minute() == feedTime[i].minute &&
          now.second() >= 0 && now.second() <= 10 &&
          now.isPM() == feedTime[i].isPM &&
          feedTime[i].isActivated == true)
        {
          String schedType = "permanent";
          tot = feedQuantity[feedDispenseCtr].feedWeight;
          dispenseFeed(schedType);   // Calls the function to dispense Feed
          break;
        }
      }
    }

    // Checks if the current time is equal to set time schedule to start feeding (For custom of temporary schedule)
    for (int i = 0; i < tempSchedCtr; i++) {
      if (
          now.month() == tempSched[i].month &&
          now.day() == tempSched[i].day &&
          now.twelveHour() == tempSched[i].hour && 
          now.minute() == tempSched[i].minute &&
          now.second() >= 0 && now.second() <= 10 &&
          now.isPM() == tempSched[i].isPM &&
          tempSched[i].isActivated == true &&
          tempSched[i].isFinished == false)
      {
        String schedType = "custom";
        tempFeedDispenseCtr = i;
        customWeight = tempSched[tempFeedDispenseCtr].feedWeight;
        dispenseFeed(schedType);   // Calls the function to dispense Feed
        break;
      }
    }
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
    int choice = getInput(19, 3, 1, 0, 3);
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
    int choice = getInput(19, 3, 1, 0, 3);
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
    int choice = getInput(19, 3, 1, 0, 3);
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

void componentsSetUp(){

  // Set pins to output pins
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  pinMode(buzzerPin, OUTPUT);

  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);
  servoM3.attach(servoPin3);

  // Set the initial state of servo motor into 0 degree
  servoM1.write(90);
  servoM2.write(180);
  servoM3.write(60);

  delay(2000);
  servoM1.detach();
  servoM2.detach();
  servoM3.detach();
}

void showDeviceName(){
  lcd.setCursor(3, 0);
  lcd.print("SAX'S CREATION");
  lcd.setCursor(6, 2);
  lcd.print("F2MATIC");
  delay(3000);
}

void startBuzzerSound(){
  // Buzzer sound when the system start or restart
  tone(buzzerPin, 700);
  delay(100);
  noTone(buzzerPin);
  delay(100);
  tone(buzzerPin, 700);
  delay(100);
  noTone(buzzerPin);
}

void askBlynkConnection(){
  menuFlag = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connect to blynk now");
  lcd.setCursor(0, 1);
  lcd.print("[1] - YES");
  lcd.setCursor(0, 2);
  lcd.print("[2] - NO");
  lcd.setCursor(0, 3);
  lcd.print("Enter your choice:");
  option = getInput(19, 3, 1, 1, 2);
  menuFlag = false;

  if(option == 1){
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Connecting to blynk");
  }
}

void blynkButtonSetUp(){
  // For the feed button
  Blynk.setProperty(V8, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V9, "isDisabled", true); // Disable the delete button widget

  // For the weight button
  Blynk.setProperty(V17, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V18, "isDisabled", true); // Disable the delete button widget

  // For the custom schedule button
  Blynk.setProperty(V32, "isDisabled", true); // Disable the edit button widget
  Blynk.setProperty(V33, "isDisabled", true); // Disable the delete button widget

  Blynk.virtualWrite(V37, String(feedSchedCtr) + " / 10");
  Blynk.virtualWrite(V38, String(tempSchedCtr) + " / 10");
  Blynk.virtualWrite(V39, String(feedWeightCtr) + " / 10");
}

void playPacman(){
  double noteFrequency[] = {
    NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5,
    NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5,
    NOTE_C6, NOTE_G6, NOTE_E6, NOTE_C6, NOTE_G6, NOTE_E6,
    
    NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5,
    NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5,
    NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5
  };

  int noteDurations[] = {
    16, 16, 16, 16,
    32, 16, 8, 16,
    16, 16, 16, 32, 16, 8,
    
    16, 16, 16, 16, 32,
    16, 8, 32, 32, 32,
    32, 32, 32, 32, 32, 16, 8
  };

  int frequencyLen = sizeof(noteFrequency) / sizeof(noteFrequency[0]);  // get the length of the note frequency arrary
    
  // Iterate through the note frequencies
  for (int i = 0; i < frequencyLen; i++) {
    int noteDuration = 2000 / noteDurations[i];   // Computation for duration
    int notePause = noteDuration * 1.30;      // Compute for the value of pause, multiplying each durations by 30%
    
    tone(buzzerPin, noteFrequency[i], noteDuration);  // Buzzer initialization
    delay(notePause);                                 // A delay between notes using the result after multiplication

    noTone(buzzerPin);
  }
}

void playNeverGonnaGiveYouUp(){
  double noteFrequency[] = {
    NOTE_G4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_E5, NOTE_E5, 0,
    NOTE_D5, 0,

    NOTE_G4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_D5, NOTE_D5, 0,
    NOTE_C5, 0, NOTE_B4, NOTE_A4, 0,

    NOTE_G4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_D5, 0,
    NOTE_B4, NOTE_A4, NOTE_G4, 0, NOTE_G4, 0, NOTE_D5, 0, NOTE_C5, 0,
  };

  int noteDurations[] = {
    4, 4, 4, 4, 1, 4, 4,
    1, 4,

    4, 4, 4, 4, 1, 4, 4,
    2, 4, 4, 4, 4,

    4, 4, 4, 4, 1, 4, 4,
    1, 4, 2, 4, 4, 4, 4, 4, 2, 2,
  };

  int frequencyLen = sizeof(noteFrequency) / sizeof(noteFrequency[0]);  // get the length of the note frequency arrary
    
  // Iterate through the note frequencies
  for (int i = 0; i < frequencyLen; i++) {
    int noteDuration = 350 / noteDurations[i];   // Computation for duration
    int notePause = noteDuration * 1.30;      // Compute for the value of pause, multiplying each durations by 30%
    
    tone(buzzerPin, noteFrequency[i], noteDuration);  // Buzzer initialization
    delay(notePause);                                 // A delay between notes using the result after multiplication

    noTone(buzzerPin);
  }
}

void dispenseFeed(String schedType) {
  String feedType;
  double feedMax = 0.3;

  anotherDispense:
  servoM1.attach(servoPin1);
  double weight = scale.get_units();  // Get weight measurement in the desired unit
  weight /= 1000;

  // M1
  // Drop feed from M1 to M2
  lcd.clear();
  servoM1.write(0);
  
  if(schedType == "permanent") {
    if(feedQuantity[feedDispenseCtr].totalNumOfTimes == 0) {
      feedQuantity[feedDispenseCtr].isFinished = true;
      feedDispenseCtr++;
      tot = feedQuantity[feedDispenseCtr].feedWeight;
    }
    
    if(feedQuantity[feedDispenseCtr].totalNumOfTimes != 0) {
      servoM2.detach();

      // if(feedQuantity[feedDispenseCtr].feedType == 1) {
      //   feedType = "Mash";
      // }
      // else if(feedQuantity[feedDispenseCtr].feedType == 2) {
      //   feedType = "Starter";
      // }
      // else if(feedQuantity[feedDispenseCtr].feedType == 3) {
      //   feedType = "Grower";
      // }

      // Blynk.logEvent("started_feeding", "Start Feeding " + feedType + "  " + String(feedQuantity[feedDispenseCtr].feedWeight) + " Kg");

      const unsigned long weightTimer = 1000;
      unsigned long weightPrevTime = 0;
      unsigned long weightCurrTime = 0;

      if(feedQuantity[feedDispenseCtr].feedWeight <= 0.3){
        tot = 0;

        while(weight < feedQuantity[feedDispenseCtr].feedWeight) {
          weightCurrTime = millis();
          weight = scale.get_units();  // Get weight measurement in the desired unit
          weight /= 1000;
          if(weightCurrTime - weightPrevTime >= weightTimer) {
            displayWeightLCD(weight, feedMax, schedType);   // Function call to display the current feed weight on the LCD

            weightPrevTime = weightCurrTime;
          }
        }
      }
      else{
        if(tot <= feedMax){
          feedMax = tot;
          tot -= tot;
        }

        while(weight < feedMax) {
          weightCurrTime = millis();
          weight = scale.get_units();  // Get weight measurement in the desired unit
          weight /= 1000;
          if(weightCurrTime - weightPrevTime >= weightTimer) {
            displayWeightLCD(weight, feedMax, schedType);   // Function call to display the current feed weight on the LCD

            weightPrevTime = weightCurrTime;
          }
        }

        if(tot >= feedMax){
          tot -= 0.3;
        }
      }

      servoM2.attach(servoPin2);
      servoM3.attach(servoPin3);

      servoM1.write(90);
    }
  }
  else {
    servoM2.detach();
    const unsigned long weightTimer = 1000;
    unsigned long weightPrevTime = 0;
    unsigned long weightCurrTime = 0;

    // Blynk.logEvent("started_feeding", "Start Feeding" + feedType + "  " + String(tempSched[tempFeedDispenseCtr].feedWeight) + " Kg");

    if(tempSched[tempFeedDispenseCtr].feedWeight <= 0.3){
      customWeight = 0;

      while(weight < tempSched[tempFeedDispenseCtr].feedWeight) {
        weightCurrTime = millis();
        weight = scale.get_units();  // Get weight measurement in the desired unit
        weight /= 1000;
        if(weightCurrTime - weightPrevTime >= weightTimer) {
          displayWeightLCD(weight, feedMax, schedType);   // Function call to display the current feed weight on the LCD

          weightPrevTime = weightCurrTime;
        }
      }
    }
    else{
      if(customWeight <= feedMax){
        feedMax = customWeight;
        customWeight -= customWeight;
      }

      while(weight < feedMax) {
        weightCurrTime = millis();
        weight = scale.get_units();  // Get weight measurement in the desired unit
        weight /= 1000;
        if(weightCurrTime - weightPrevTime >= weightTimer) {
          displayWeightLCD(weight, feedMax, schedType);   // Function call to display the current feed weight on the LCD

          weightPrevTime = weightCurrTime;
        }
      }

      if(customWeight >= feedMax){
        customWeight -= 0.3;
      }
    }
  
    servoM2.attach(servoPin2);
    servoM3.attach(servoPin3);

    servoM1.write(90);
  }
  digitalWrite(6, HIGH); // Relay trigger to start the blower
  displayWeightLCD(weight, feedMax, schedType);   // Function call to display the current feed weight on the LCD
  
  delay(1000);

  // M3
  // Dispensed feed and swing left and right
  lcd.clear();
  lcd.setCursor(2, 2);
  lcd.println("Dispensing Feed");

  // int servoM2Pos = 180;
  // int servoM3Pos = 0;
  // const unsigned long dropTimer = 10;
  // unsigned long dropPrevTime = 0;
  // unsigned long dropCurrTime = 0;
  servoM2.write(0);
  for(int i = 0; i < 15; i++){
    servoM3.write(40);
    delay(800);
    servoM3.write(85);
    delay(800);
  }
  
  // for (int i = 0; i < 20; i++) {
  //   dropCurrTime = millis();
  //   if (dropCurrTime - dropPrevTime >= dropTimer && servoM2Pos != 0) {
  //     servoM2Pos -= 20;
  //     servoM2.write(servoM2Pos);
  //     dropPrevTime = dropCurrTime;
  //   }

  //   if (i % 2 == 0) {
  //     servoM3Pos = 20; //Spin in one direction
  //   }
  //   else {
  //     servoM3Pos = 100; // Spin in opposite direction
  //   }

  //   servoM3.write(servoM3Pos);
  //   delay(500);
  // }

  servoM2.write(180); // Return back to the initial position
  servoM3.write(60); // Return back to the initial position
  digitalWrite(6, LOW); // Relay trigger to stop the blower
  
  delay(2000);
  servoM1.detach();
  servoM2.detach();
  servoM3.detach();

  if(tot != 0 || customWeight != 0) {
    digitalWrite(6, LOW); // Relay trigger to stop the blower
    servoM3.write(60); // Return back to the initial position
    goto anotherDispense;
  }

  if(schedType == "permanent") {
    EEPROM.update(3, feedDispenseCtr);

    feedQuantity[feedDispenseCtr].totalNumOfTimes--;

    updateWeightEEPROM();

    // Blynk.logEvent("done_feeding", "Done Feeding " + feedType + "  " + String(feedQuantity[feedDispenseCtr].feedWeight) + " Kg");
  }
  else{
    // Blynk.logEvent("done_feeding", "Done Feeding " + feedType + "  " + String(tempSched[tempFeedDispenseCtr].feedWeight) + " Kg");

    for(int i = tempFeedDispenseCtr; i < tempSchedCtr; i++){
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
  }

  playNeverGonnaGiveYouUp();

  lcd.clear();
}

void displayWeightLCD(double weight, double feedMax, String schedType) {
  lcd.setCursor(0, 0);
  lcd.print("Setted Weight: ");
  lcd.setCursor(0, 1);
  if(schedType == "permanent") {
    if(feedQuantity[feedDispenseCtr].feedWeight <= 0.3) {
      lcd.print(String(feedQuantity[feedDispenseCtr].feedWeight) + " Kg");
    }
    else {
      lcd.print(String(feedMax) + " Kg");
    }
  }
  else if (schedType == "custom") {
    if(tempSched[tempFeedDispenseCtr].feedWeight <= 0.3) {
      lcd.print(String(tempSched[tempFeedDispenseCtr].feedWeight) + " Kg");
    }
    else{
      lcd.print(String(feedMax) + " Kg");
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
  
  lcd.setCursor(18, 2);
  if (cm <= 60 && cm >= 30) {
    lcd.write(byte(0));  // Display arrow up
  }
  else if (cm <= 10 && cm >= 1) {
    lcd.write(byte(1));  // Display arrow down
  }
}

void displayDissolvedOxygenLevel(){
  lcd.setCursor(0, 3);
  lcd.print("DO LEVEL: ");
  lcd.print(DOlevel);
  lcd.print(" ppm");
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