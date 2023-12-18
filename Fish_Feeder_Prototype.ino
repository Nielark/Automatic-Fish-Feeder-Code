#include <Servo.h>

// Motor A connections
const int enA = 9;
const int in1 = 8;
const int in2 = 7;

// Motor B connections
const int enB = 3;
const int in3 = 5;
const int in4 = 4;

const int servoPin1 = 10;
const int servoPin2 = 11;
const int servoPin3 = 6;

Servo servoM1;
Servo servoM2;
Servo servoM3;

void setup()
{
  // Set all the motor control pins to outputs
	pinMode(enA, OUTPUT);
	pinMode(enB, OUTPUT);
	pinMode(in1, OUTPUT);
	pinMode(in2, OUTPUT);
	pinMode(in3, OUTPUT);
	pinMode(in4, OUTPUT);
	// pinMode(13, OUTPUT);
	// Turn off motors - Initial state
	digitalWrite(in1, LOW);
	digitalWrite(in2, LOW);
	digitalWrite(in3, LOW);
	digitalWrite(in4, LOW);

  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);
  servoM3.attach(servoPin3);

  // Set the initial state of servo motor into 0 degree
  servoM1.write(0);
  servoM2.write(0);
  servoM3.write(0);
}

void loop()
{
  // M1
  // Drop feed from M1 to M2
  servoM1.write(180);
  delay(5000);
  servoM1.write(0);
  delay(3000);
  
  // M2 AND M3
  // Set motors to maximum speed
	// For PWM maximum possible values are 0 to 255
  // digitalWrite(13, HIGH);
	analogWrite(enA, 255);    // Blower speed
  analogWrite(enB, 255);    // Gearbox Motor speed

  // Blower rotate in clockwise direction 
  digitalWrite(in1, HIGH);
	digitalWrite(in2, LOW);

  // Drop feed from M2 to M3
  servoM2.write(180);

  // Gear Box motor swing left and right direction
  for(int i = 0; i < 10; i++){
    digitalWrite(in3, HIGH);
	  digitalWrite(in4, LOW);
    delay(1000);
    digitalWrite(in3, LOW);
	  digitalWrite(in4, HIGH);
    delay(1000);
  }

  // Servo motor swing left and right
  // for(int i = 0; i < 10; i++){
  //   servoM3.write(30);
  //   delay(1000);
  //   servoM3.write(0);
  //   delay(1000);
  // }
  servoM2.write(0);  

  // Turn off motors
  digitalWrite(in1, LOW);
	digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
	digitalWrite(in4, LOW);
  // digitalWrite(13, LOW);
  
  delay(10000);
}