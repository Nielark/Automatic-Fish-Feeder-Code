#include <Servo.h>
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

Servo servoM1;
Servo servoM2;
Servo servoM3;

void setup()
{
  // Set pins to output pins
  pinMode(13, OUTPUT);

  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);
  servoM3.attach(servoPin3);

  // Set the initial state of servo motor into 0 degree
  servoM1.write(0);
  servoM2.write(0);
  servoM3.write(90);
}

void loop()
{
  // ULTRASONIC SENSOR READING
  // measure the ping time in cm
  cm = 0.01723 * readUltrasonicDistance(triggerPin, echoPin);
  
  // convert to inches by dividing by 2.54
  inches = (cm / 2.54);

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
  delay(3000);

  servoM2.write(0); // Return back to the initial position

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
  servoM3.write(90); // Stop
  digitalWrite(13, LOW); // Relay trigger to stop the blower

  delay(10000); // Interval for the next feeding
}