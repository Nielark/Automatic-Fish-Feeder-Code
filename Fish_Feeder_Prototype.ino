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
  // servoM2.write(0);
  // delay(1000);

  // servoM2.write(180);
  
  // delay(1000);

  // M2 (Continuous Rotation)
  servoM2.writeMicroseconds(1500); // Set speed to the center position
  delay(1000); // Wait for a moment before changing direction

  servoM2.writeMicroseconds(2000); // Rotate in one direction (adjust speed and direction as needed)
  delay(2000); // Wait for a specific duration of movement

  servoM2.writeMicroseconds(1000); // Rotate in the opposite direction (adjust speed and direction as needed)
  delay(2000); // Wait for a specific duration of movement

  servoM2.writeMicroseconds(1500); // Set speed to the center position (stop)
  delay(1000); // Wait before the next movement
}