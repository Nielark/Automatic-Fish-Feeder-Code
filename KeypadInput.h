#ifndef KEYPADINPUT_H
#define KEYPADINPUT_H

#include "variables.h";

int getInput(int cursorPosCols, int cursorPosRow, int charLen, int minVal, int maxVal) {
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

#endif