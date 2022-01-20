/******************************************************************************
Program: Sad Cat Fixer (Laser Tower 3000)

Description: Program to control the Laser Tower 3000. This includes manual and
automatic modes, an LCD screen, a servo, a buzzer, a laser (an LED for our
purposes), and an infrared remote control.

Author: Pranav Rao

Date: January 9, 2021

Arduino Resources used: digital pins 3, 4, 6, 8, and analog pins 4 and 5
******************************************************************************/

/******************************************************************************
Libraries: this section contains the importing of various libraries, which are
files that contain several classes, structs, and functions that are essential to
controlling various components such as the remote, LCD, and servo.
******************************************************************************/

#include <IRremote.hpp>        // library to interact with the IR remote
#include <LiquidCrystal_I2C.h> // library to interact with the LCD
#include <Servo.h>             // library to interact with the servo

/******************************************************************************
Constants: these are constant values that will be used to denote important
and consistent information. They are GLOBAL variables, and therefore can be used
by any function in this program.
******************************************************************************/

// declare constants to represent pins of each of the circuit components (IR,
// laser, servo, buzzer)
const int IR_RECEIVE_PIN = 3, LASER_PIN = 4, SERVO_PIN = 6, BUZZER_PIN = 8;

// declare constant strings for the words automatic and manual (which are
// printed on the LCD)
const char WORD_AUTOMATIC[] = "AUTOMATIC";
const char WORD_MANUAL[] = "MANUAL";

// declare constants for the sizes of the words so that they can be printed on
// the LCD
const int WORD_AUTOMATIC_SIZE =
    sizeof(WORD_AUTOMATIC) / sizeof(WORD_AUTOMATIC[0]);
const int WORD_MANUAL_SIZE = sizeof(WORD_MANUAL) / sizeof(WORD_MANUAL[0]);

// declare a set of variables used to keep track of various factors in the
// program. these variables will change throughout the program
int currentTime, currentPreset, randomInterval, currentServoRotation = 0,
                                                automaticRotationSpeed = 15;

// declare two clock variables, which will be used to keep track of certain
// times (amount of milliseconds from the beginning of the program). These
// clocks allow for the asynchronous behaviour of the automatic mode.
// clock1 is used to keep track of the intervals between preset changes in
// automatic mode. clock2 is used to keep track of the intervals between blinks
// for the auto blink functions
unsigned long clock1, clock2;

// declare two bool variables that will be used to keep track of the mode
// and the status of the laser
bool automatic = true, laserOn = false;

/******************************************************************************
Objects: these variables are instances of classes (imported in the header files
above). By making these instances, we are able to access all of the attributes
and functions defined in those classes
******************************************************************************/

// declare an object to represent the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
// declare an object to represent the servo motor
Servo servo;

/******************************************************************************
writeText function: this function is called to write certain text to the
LCD. It takes a char pointer (a char array, essentially) and the length of the
char array, and returns void.
******************************************************************************/

void writeText(const char *text, int len) {
  Serial.println("LOG: Writing text to LCD.");

  lcd.clear(); // clear the LCD

  // start from the top left of the LCD. Then, sequencially print each character
  // in the given word, one character after the next
  for (int i = 0; i < len - 1; i++) {
    lcd.setCursor(i, 0); // set the cursor to the correct position
    lcd.print(text[i]);  // print the character
  }
}

/******************************************************************************
toggleLaser function: this function is called toggle the state of the laser. It
takes and returns nothing.
******************************************************************************/

void toggleLaser() {
  // if the laser is turned on
  if (laserOn) {
    digitalWrite(LASER_PIN, LOW);  // turn the laser off
  } else {                         // if the laser is off
    digitalWrite(LASER_PIN, HIGH); // turn the laser on
  }

  // if the laserOn boolean is false, set it to true else set it to false
  // (essentially toggle it)
  laserOn = laserOn ? false : true;
}

/******************************************************************************
Preset Functions: these functions are special functions that toggle different
presets of this machine. They all take nothing and return nothing.
******************************************************************************/

// this preset is used to keep the laser constantly on
void presetConstantOn() {
  // if the laser is not on
  if (!laserOn) {
    toggleLaser(); // turn on the laser
  }
}

// this preset is used to keep the laser constantly off
void presetConstantOff() {
  // if the laser is on
  if (laserOn) {
    toggleLaser(); // turn off the laser
  }
}

// this preset is used only in auto mode to keep the laser blink slowly
void presetAutoSlowBlink() {
  unsigned long currentTime = millis();

  if (currentTime > clock2 + 1500) {
    toggleLaser();
    clock2 = currentTime;
  }
}

// this preset is used only in auto mode to make the laser blink fast
void presetAutoFastBlink() {
  unsigned long currentTime = millis();

  if (currentTime > clock2 + 200) {
    toggleLaser();
    clock2 = currentTime;
  }
}

// this reset is used only in manual mode to make the laser blink slow
void presetManualSlowBlink() {
  for (int i = 0; i < 5; i++) {
    toggleLaser();
    delay(2000);
    toggleLaser();
    delay(2000);
  }
}

// this reset is used only in manual mode to make the laser blink fast
void presetManualFastBlink() {
  for (int i = 0; i < 5; i++) {
    toggleLaser();
    delay(500);
    toggleLaser();
    delay(500);
  }
}

// this is an array containing pointers to each of the automatic presets. A
// function will be called from this list depending on the current preset number
void (*autoPresets[4])() = {presetConstantOff, presetConstantOn,
                            presetAutoSlowBlink, presetAutoFastBlink};

// this is an array containing pointers to each of the manual presets. A
// function will be called from this list depending on the button pressed
void (*manualPresets[4])() = {presetConstantOff, presetConstantOn,
                              presetManualSlowBlink, presetManualFastBlink};

/******************************************************************************
Rotate Functions: these functions are functions that rotate the servo motor.
They both take a number of degrees to rotate and return nothing.
******************************************************************************/

// this function is used to rotate the servo a certain number of degrees in
// manual mode
void rotateManual(int degrees) {
  // calculate the new position of the servo using the current position of the
  // servo (stored in a variable)
  int newPosition = currentServoRotation + degrees;

  // if the new position is greater than 180 (the max the servo can turn in one
  // direction)
  if (newPosition > 180)
    newPosition = 180; // set the calculated new position back to 180

  // if the new position is less than 0 (the min the servo can turn in one
  // direction)
  if (newPosition < 0)
    newPosition = 0; // set the calculated new position back to 0

  // update the value of the current servo rotation to the new calculated value
  currentServoRotation = newPosition;

  servo.write(currentServoRotation); // turn the servo to the given position
  delay(500); // wait for the servo to turn to the given position
}

// this function is used to rotate the servo a certain number of degrees in
// manual mode
void rotateAutomatic(int degrees) {
  // the automatic mdoe wokrs by always moving in one direction until it reaches
  // the max position (180), where it resets to the min position (0)

  // given how the mode works, if the degrees value is negative, it must be
  // changed to positive
  int fixedDegrees =
      degrees < 0 ? -1 * degrees
                  : degrees; // change the degrees value to positive if it's
                             // negative, and store in a new variable

  // calculate the new position using the fixed degrees value
  int newPosition = currentServoRotation + fixedDegrees;

  // divide the new calculated position by 180 and take the remainder, then
  // store it in the variable to keep track of the current servo rotation. This
  // ensures that the position never exceeds 180 (the max).
  currentServoRotation = newPosition % 180;

  servo.write(currentServoRotation);
  delay(500); // wait for the servo to turn to the given position
}

/******************************************************************************
Mode Functions: these functions change the mode of the machine (manual vs
automatic). This affects how the user gives input and interacts with the
machine, and what the machine does given said input.
******************************************************************************/

// this function is the manual mode function. When called repeatedly in the loop
// function (see below), it gives the user full manual control of the machine,
// essentially allowing them to trigger any feature at random. The user can also
// use the play/pause button to switch to automatic mode
void manualMode() {
  // if input is received from the IR remote
  if (IrReceiver.decode()) {
    Serial.println("LOG: Received input from IR remote. Attempting to parse.");

    // attempt to parse the data and get a readable integer
    uint32_t decoded = IrReceiver.decodedIRData.decodedRawData;

    // this switch statement compares the value of the decoded variable with
    // each of the cases described. In this case, it is checking for the codes
    // of each remote button; if a certain remote button is hit, the machine
    // will perform the associated operation. Tests were performed beforehand to
    // find the code for each key on the remote.
    switch (decoded) {
    case 4077715200: // if the 1 key is pressed on the remote
      Serial.println("LOG: Remote input is button 1.");
      (*manualPresets[0])(); // call the first manual preset function declared
                             // in the array above
      break;
    case 3877175040: // if the 2 key is pressed on the remote
      Serial.println("LOG: Remote input is button 2.");
      (*manualPresets[1])(); // call the second manual preset function declared
                             // in the array above
      break;
    case 2707357440: // if the 3 key is pressed on the remote
      Serial.println("LOG: Remote input is button 3.");
      (*manualPresets[2])(); // call the third manual preset function declared
                             // in the array above
      break;
    case 4144561920: // if the 4 key is pressed on the remote
      Serial.println("LOG: Remote input is button 4.");
      (*manualPresets[3])(); // call the fourth manual preset function declared
                             // in the array above
      break;
    case 3141861120: // if the back key is pressed on the remote
      Serial.println("LOG: Remote input is button BACK.");
      rotateManual(
          -30); // rotate the motor 30 degrees counter clockwise (if possible)
      break;
    case 3158572800: // if the forward key is pressed on the remote
      Serial.println("LOG: Remote input is button FORWARD.");
      rotateManual(30); // rotate the motor 30 degrees clockwise (if possible)
      break;
    case 3208707840: // if the play/pause key is pressed on the remote
      Serial.println("LOG: Remote input is button PLAY/PAUSE.");
      Serial.println("LOG: Switching to AUTO mode.");
      automatic = true; // set the global automatic flag to true (change to
                        // automatic mode)
      writeText(WORD_AUTOMATIC,
                WORD_AUTOMATIC_SIZE); // print AUTOMATIC on the LCD
      // tone(BUZZER_PIN, 300, 1000); // play the buzzer sound to give the user
      // an audio cue for mode change
      break;
    }

    IrReceiver.resume(); // continue collecting input
  }

  return;
}

// this function is the automatic mode function. When called repeatedly in the
// loop function (see below), the function causes the machine to run
// automatically and randomly, in that it will continuously call random presets.
// this function will also give the user the ability to increase or decrease the
// speed of rotation using the up/down buttons on the remote. The user will
// also be able to switch to manual mode at any time using the play/pause button
// on the remote.
void automaticMode() {
  rotateAutomatic(
      automaticRotationSpeed); // rotate at the speed declared by the
                               // automaticRotationSpeed global variable

  unsigned long currentTime =
      millis(); // collect the current time to run comparisons against the two
                // async clocks

  // if the current time collected is greater than 5 seconds later than clock1
  // (the last recorded checkpoint)
  if (currentTime > clock1 + 5000) {
    Serial.print("LOG: Selecting new random preset. New preset: ");
    currentPreset =
        random(4); // select a new random number from 0 to 3 inclusive and set
                   // it as the global currentPreset variable
    Serial.println(currentPreset);
    clock1 = currentTime; // update clock1 to represent now as the last marked
                          // time (checkpoint)
  }

  // if input is received from the IR remote
  if (IrReceiver.decode()) {
    Serial.println("LOG: Received input from IR remote. Attempting to parse.");

    // attempt to parse the data and get a readable integer
    uint32_t decoded = IrReceiver.decodedIRData.decodedRawData;

    // this switch statement compares the value of the decoded variable with
    // each of the cases described. In this case, it is checking for the codes
    // of each remote button; if a certain remote button is hit, the machine
    // will perform the associated operation. Tests were performed beforehand to
    // find the code for each key on the remote.
    switch (decoded) {
    case 3208707840: // if the play/pause key is pressed on the remote
      Serial.println("LOG: Remote input is button PLAY/PAUSE.");
      Serial.println("LOG: Switching to MANUAL mode.");
      automatic = false; // set the global automatic flag to false (change to
                         // manual mode)
      writeText(WORD_MANUAL, WORD_MANUAL_SIZE); // print MANUAL on the LCD
      // tone(BUZZER_PIN, 300, 1000); // play the buzzer sound to give the user
      // an audio cue for mode change
      break;
    case 4127850240: // if the up key is pressed on the remote
      Serial.println("LOG: Increasing automatic speed by 5.");
      automaticRotationSpeed +=
          5; // increase the current speed for automatic mode rotation by 5
      if (automaticRotationSpeed >
          45) { // if the automatic rotation speed is above 45 (max)
        Serial.println("WARN: Automatic speed is above 45. Resetting to 45.");
        automaticRotationSpeed =
            45; // set the automatic rotation speed back to 45
      }
      break;
    case 4161273600: // if the down key is pressed on the remote
      Serial.println("LOG: Decreasing automatic speed by 5.");
      automaticRotationSpeed -=
          5; // decrease the current speed for automatic mode rotation by 5
      if (automaticRotationSpeed <
          0) { // if the automatic rotation speed is below 45 (min)
        Serial.println("WARN: Automatic speed is below 0. Resetting to 0.");
        automaticRotationSpeed =
            0; // set the automatic rotation speed back to 0
      }
      break;
    }

    IrReceiver.resume(); // continue collecting input
  }

  (*autoPresets[currentPreset])(); // call the current preset function (as
                                   // determined by the global var
                                   // currentPreset)
}

/******************************************************************************
Setup function: this function is automatically called once, and has a return
type of void.
******************************************************************************/

void setup() {
  Serial.begin(9600); // intialize the Serial monitor

  Serial.println("LOG: Starting init sequence.");

  // initialize the LCD
  Serial.println("LOG: Initializing LCD.");
  lcd.init();
  lcd.backlight(); // turn on the LCD backlight

  // initialize the IR Remote and bind it to the correct pin
  Serial.println("LOG: Initializing IR Remote.");
  IrReceiver.begin(IR_RECEIVE_PIN);

  // initialize the random seed (needed to use random functionality in automatic
  // function)
  Serial.println("LOG: Planting randomizer seed using empty analog input 0.");
  randomSeed(analogRead(0));

  // intialize clocks to current time to enable asynchronous functionality
  Serial.println("LOG: Initializing clocks.");
  clock1 = millis();
  clock2 = millis();

  // intialize currentPreset and randomInterval with random values from the
  // random seed
  Serial.println("LOG: Setting required random values.");
  randomInterval =
      random(5000, 15001);   // the random interval will always be a number from
                             // 5000 ms to 15000 ms (5-15 seconds)
  currentPreset = random(4); // the currentPreset will always be a number from
                             // 0-3 because there are four presets

  // set up laser
  Serial.println("LOG: Setting up laser.");
  pinMode(LASER_PIN, OUTPUT);   // set the laser to OUTPUT mode
  digitalWrite(LASER_PIN, LOW); // turn off the laser to begin

  // set up servo
  Serial.println("LOG: Setting up servo.");
  servo.attach(SERVO_PIN); // attach the servo object to the correct pin
  servo.write(currentServoRotation); // turn the servo to the correct position
                                     // (initially 0)

  Serial.println("LOG: Starting in AUTO mode.");
  writeText(WORD_AUTOMATIC,
            WORD_AUTOMATIC_SIZE); // print AUTOMATIC on the LCD (because it is
                                  // the starting mode)

  Serial.println("LOG: Playing initialization tone.");
  // tone(BUZZER_PIN, 300, 1000); // play a tone to notify the user the machine
  // has been initialized
}

/******************************************************************************
Loop function: this function is called repeatedly for the lifespan of the
program and has a return type of void.
******************************************************************************/

void loop() {
  // if the current mode is automatic (as determined by the automatic global
  // boolean), then call the automaticMode function. Else, call the manualMode
  // function. Since this check is in the loop function, this check will
  // continue to be run forever, meaning that the correct function (be it
  // automatic or manual) will be called many times in quick succession. The
  // program depends heavily on this mechanism to function.
  if (automatic) {
    automaticMode();
  } else {
    manualMode();
  }
}
