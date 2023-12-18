#include <Servo.h>

const int servoPin1 = 10;
const int servoPin2 = 11;
const int servoPin3 = 6;

Servo servoM1;
Servo servoM2;
Servo servoM3;

void setup()
{
  servoM1.attach(servoPin1);
  servoM2.attach(servoPin2);
  servoM3.attach(servoPin3);

  // Set the initial state of servo motor into 0 degree
  servoM1.write(0);
  //servoM2.write(90);
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

  // Drop feed from M2 to M3
  servoM2.write(90);
  delay(3000);

  servoM2.write(180);
  
  delay(10000);
}