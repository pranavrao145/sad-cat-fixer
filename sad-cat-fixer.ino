/******************************************************************************
Program: Sad Cat Fixer

Description: Program to control the Laser Tower 3000. This includes manual and
automatic modes, an LCD screen, a buzzer, and an infrared remote control.

Author: Pranav Rao

Date: January 9, 2021

Arduino Resources used:
******************************************************************************/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

/******************************************************************************
Constants: these are constant values that will be used to denote important
and consistent information. They are GLOBAL variables, and therefore can be used
by any function in this program.
******************************************************************************/

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int DELAY_TIME = 300, DISPLAY_LENGTH = 16;

const char WORD_AUTOMATIC[] = "AUTOMATIC";
const char WORD_MANUAL[] = "MANUAL";

const int WORD_AUTOMATIC_SIZE =
    sizeof(WORD_AUTOMATIC) / sizeof(WORD_AUTOMATIC[0]);
const int WORD_MANUAL_SIZE = sizeof(WORD_MANUAL) / sizeof(WORD_MANUAL[0]);

int arrayPosition, screenPosition;

/******************************************************************************
Setup function: this function is automatically called once, and has a return
type of void.
******************************************************************************/

void setup() {
  lcd.init();
  lcd.backlight();
}

/******************************************************************************
Loop function: this function is called repeatedly for the lifespan of the
program and has a return type of void.
******************************************************************************/

void loop() {}

/******************************************************************************
writeText function: this function is called to write certain text to the
LCD. It takes a char pointer (a char array, essentially) and the length of the
char array, and returns void. A lot of the logic in this function was determined
by complex math.
******************************************************************************/

void writeText(const char *text, int len) {

  // this loop starts from zero and iterates to the length of the string, plus
  // the length of the display (16), plus one.
  for (int noOfCharsToPrint = 0; noOfCharsToPrint < len + DISPLAY_LENGTH + 1;
       noOfCharsToPrint++) {

    // wait a certain time before printing the character
    delay(DELAY_TIME);

    // a second loop which is used to determine where exactly the character must
    // be printed
    for (int charPositionToPrint = noOfCharsToPrint; charPositionToPrint > 0;
         charPositionToPrint--) {
      // find the position of the character in the text string
      arrayPosition = noOfCharsToPrint - charPositionToPrint;

      // find the position at which the character must be printed on the screen
      screenPosition = 16 - charPositionToPrint;

      // if the position on the screen is real (above 0)
      if (screenPosition >= 0) {
        // if the position of the character in the array is within the boudns of
        // the array
        if (arrayPosition < len - 1) {
          lcd.setCursor(screenPosition,
                        0); // set the cursor of the LCD to the correct position
          lcd.print(text[arrayPosition]); // print the correct character of text
                                          // to the LCD at the current position
        } else { // else if the position of the character is not within the
                 // mounds of the array (print a space)
          lcd.setCursor(screenPosition,
                        0); // set the cursor of the LCD to the correct position
          lcd.print(" ");   // print a space at the position of the curor
        }
      }
    }
  }
}
