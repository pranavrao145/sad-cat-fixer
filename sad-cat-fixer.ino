/******************************************************************************
Program: Sad Cat Fixer

Description: Program to control the Laser Tower 3000. This includes manual and
automatic modes, an LCD screen, a buzzer, and an infrared remote control.

Author: Pranav Rao

Date: January 9, 2021

Arduino Resources used:
******************************************************************************/
// TODO: comments

#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Wire.h>

/******************************************************************************
Constants: these are constant values that will be used to denote important
and consistent information. They are GLOBAL variables, and therefore can be used
by any function in this program.
******************************************************************************/

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int DELAY_TIME = 300, DISPLAY_LENGTH = 16, IR_RECEIVE_PIN = 3,
          LASER_PIN = 4, SERVO_PIN = 6, BUZZER_PIN = 8;

const char WORD_AUTOMATIC[] = "AUTOMATIC";
const char WORD_MANUAL[] = "MANUAL";

const int WORD_AUTOMATIC_SIZE =
    sizeof(WORD_AUTOMATIC) / sizeof(WORD_AUTOMATIC[0]);
const int WORD_MANUAL_SIZE = sizeof(WORD_MANUAL) / sizeof(WORD_MANUAL[0]);

int arrayPosition, screenPosition, currentTime, currentPreset,
    currentServoRotation = 0, automaticSpeed = 15;

long randomInterval;

unsigned long clock1, clock2;

bool automatic = true, laserOn = false;

Servo servo;

/******************************************************************************
writeText function: this function is called to write certain text to the
LCD. It takes a char pointer (a char array, essentially) and the length of the
char array, and returns void. A lot of the logic in this function was
determined by complex math.
******************************************************************************/

void writeText(const char *text, int len) {
  Serial.println("LOG: Writing text to LCD.");

  lcd.clear();

  for (int i = 0; i < len - 1; i++) {
    lcd.setCursor(i, 0);
    lcd.print(text[i]);
  }
}

void toggleLaser() {
  if (laserOn) {
    digitalWrite(LASER_PIN, LOW);
  } else {
    digitalWrite(LASER_PIN, HIGH);
  }

  laserOn = laserOn ? false : true;
}

void presetConstantOn() {
  if (!laserOn) {
    toggleLaser();
  }
}

void presetConstantOff() {
  if (laserOn) {
    toggleLaser();
  }
}

void presetAutoSlowBlink() {
  unsigned long currentTime = millis();

  if (currentTime > clock2 + 1500) {
    toggleLaser();
    clock2 = currentTime;
  }
}

void presetAutoFastBlink() {
  unsigned long currentTime = millis();

  if (currentTime > clock2 + 200) {
    toggleLaser();
    clock2 = currentTime;
  }
}

void presetManualSlowBlink() {
  for (int i = 0; i < 5; i++) {
    toggleLaser();
    delay(2000);
    toggleLaser();
    delay(2000);
  }
}

void presetManualFastBlink() {
  for (int i = 0; i < 5; i++) {
    toggleLaser();
    delay(500);
    toggleLaser();
    delay(500);
  }
}

void (*autoPresets[4])() = {presetConstantOff, presetConstantOn,
                            presetAutoSlowBlink, presetAutoFastBlink};

void (*manualPresets[4])() = {presetConstantOff, presetConstantOn,
                              presetManualSlowBlink, presetManualFastBlink};

void rotateManual(int degrees) {
  int newPosition = currentServoRotation + degrees;

  if (newPosition > 180)
    newPosition = 180;

  if (newPosition < 0)
    newPosition = 0;

  servo.write(newPosition);
}

void rotateAutomatic(int degrees) {
  int fixedDegrees = degrees < 0 ? -1 * degrees : degrees;
  int newPosition = currentServoRotation + fixedDegrees;

  currentServoRotation = newPosition;

  servo.write(newPosition % 180);
  delay(500);
}

void manualMode() {
  if (IrReceiver.decode()) {
    Serial.println("LOG: Received input from IR remote. Attempting to parse.");

    uint32_t decoded = IrReceiver.decodedIRData.decodedRawData;

    switch (decoded) {
    case 4077715200:
      Serial.println("LOG: Remote input is button 1.");
      (*manualPresets[0])();
      break;
    case 3877175040:
      Serial.println("LOG: Remote input is button 2.");
      (*manualPresets[1])();
      break;
    case 2707357440:
      Serial.println("LOG: Remote input is button 3.");
      (*manualPresets[2])();
      break;
    case 4144561920:
      Serial.println("LOG: Remote input is button 4.");
      (*manualPresets[3])();
      break;
    case 3141861120:
      Serial.println("LOG: Remote input is button BACK.");
      rotateManual(-30);
      break;
    case 3158572800:
      Serial.println("LOG: Remote input is button FORWARD.");
      rotateManual(30);
      break;
    case 3208707840:
      Serial.println("LOG: Remote input is button PLAY/PAUSE.");
      Serial.println("LOG: Switching to AUTO mode.");
      automatic = true;
      writeText(WORD_AUTOMATIC, WORD_AUTOMATIC_SIZE);
      // tone(BUZZER_PIN, 300, 1000);
      break;
    }

    IrReceiver.resume();
  }

  return;
}

void automaticMode() {
  rotateAutomatic(automaticSpeed);

  unsigned long currentTime = millis();

  if (currentTime > clock1 + 5000) {
    Serial.print("LOG: Selecting new random preset. New preset: ");
    currentPreset = random(4);
    Serial.println(currentPreset);
    clock1 = currentTime;
  }

  if (IrReceiver.decode()) {
    Serial.println("LOG: Received input from IR remote. Attempting to parse.");

    uint32_t decoded = IrReceiver.decodedIRData.decodedRawData;

    switch (decoded) {
    case 3208707840:
      Serial.println("LOG: Remote input is button PLAY/PAUSE.");
      Serial.println("LOG: Switching to MANUAL mode.");
      writeText(WORD_MANUAL, WORD_MANUAL_SIZE);
      automatic = false;
      // tone(BUZZER_PIN, 300, 1000);
      break;
    case 4127850240:
      Serial.println("LOG: Increasing automatic speed by 5.");
      automaticSpeed += 5;
      if (automaticSpeed > 45) {
        Serial.println("WARN: Automatic speed is above 45. Resetting to 45.");
        automaticSpeed = 45;
      }
      break;
    case 4161273600:
      Serial.println("LOG: Decreasing automatic speed by 5.");
      automaticSpeed -= 5;
      if (automaticSpeed < 0) {
        Serial.println("WARN: Automatic speed is below 0. Resetting to 0.");
        automaticSpeed = 0;
      }
      break;
    }

    IrReceiver.resume();
  }

  (*autoPresets[currentPreset])();
}

/******************************************************************************
Setup function: this function is automatically called once, and has a return
type of void.
******************************************************************************/

void setup() {
  Serial.begin(9600);

  Serial.println("LOG: Starting init sequence.");

  // initialize the LCD
  Serial.println("LOG: Initializing LCD.");
  lcd.init();
  lcd.backlight();

  Serial.println("LOG: Initializing IR Remote.");
  // initialize the IR Remote
  IrReceiver.begin(IR_RECEIVE_PIN);

  // initialize the random seed
  Serial.println("LOG: Planting randomizer seed using empty analog input 0.");
  randomSeed(analogRead(0));

  // initialize the clocks
  Serial.println("LOG: Initializing clocks.");
  clock1 = millis();
  clock2 = millis();

  // set random variables
  Serial.println("LOG: Setting required random values.");
  randomInterval = random(5000, 15001);
  currentPreset = random(4);

  // set up laser
  Serial.println("LOG: Setting up laser.");
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, LOW);

  // set up servo
  Serial.println("LOG: Setting up servo.");
  servo.attach(SERVO_PIN);
  servo.write(currentServoRotation);

  Serial.println("LOG: Switching to AUTO mode.");
  writeText(WORD_AUTOMATIC, WORD_AUTOMATIC_SIZE);

  Serial.println("LOG: Playing mode switch tone.");
  // tone(BUZZER_PIN, 300, 1000);
}

/******************************************************************************
Loop function: this function is called repeatedly for the lifespan of the
program and has a return type of void.
******************************************************************************/

void loop() {
  if (automatic) {
    automaticMode();
  } else {
    manualMode();
  }
}
