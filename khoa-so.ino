#include <SPI.h>
#include <SD.h>
#include "Adafruit_Keypad.h"
#include <EEPROM.h>

const byte PIN_LOAD_PASSWORD = A0;
const byte PIN_BUZZER = A1;
const byte PIN_POWER = 9;
const byte PIN_DOOR_PWMB = 10;
const byte PIN_DOOR_INB1 = A2;
const byte PIN_DOOR_INB2 = A3;

const byte ROWS = 5;
const byte COLS = 3;
const char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'0', 'A', 'B'},
  {'X', 'E', 'P'}
};
const byte colPins[COLS] = {5, 6, 7};
const byte rowPins[ROWS] = {8, 3, 2, A5, A4};
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);

enum Mode {
  NORMAL,
  CHANGE_PWD_CHECK,
  CHANGE_PWD_NEW,
  CHANGE_PWD_CONFIRM,
  GOD_MODE
};

boolean isOpen = false;
String command = "";
String newPassword = "";
Mode mode = NORMAL;
unsigned long lastAct = 0;
unsigned long openTime = 0;
int errorCnt = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_LOAD_PASSWORD, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_POWER, OUTPUT);
  pinMode(PIN_DOOR_PWMB, OUTPUT);
  pinMode(PIN_DOOR_INB1, OUTPUT);
  pinMode(PIN_DOOR_INB2, OUTPUT);
  customKeypad.begin();
  if (analogRead(PIN_LOAD_PASSWORD) == LOW) {
    setupPasswordIfAvailable();
  }
  Serial.println(readEEPROMString(0));
  Serial.println(readEEPROMString(1));
  beeps(939.85, 200);
  delay(200);
  beeps(1318.51, 200);
  delay(200);
  beeps(1567.98, 200);
  delay(200);
  beeps(1975.53, 200);
}

void setupPasswordIfAvailable() {
  if (
    !SD.begin(4)) {
    return;
  }
  File passwordFile = SD.open("password.txt");
  if (passwordFile) {
    String password  = "";
    while (passwordFile.available()) {
      char letter = passwordFile.read();
      password =  password + letter;
    }
    passwordFile.close();
    writeEEPROMString(0, password);
    Serial.println("Password is set to " + password);
  } else {
    Serial.println("Unable to set password");
  }

  File recoveryFile = SD.open("recovery.txt");
  if (recoveryFile) {
    String password  = "";
    while (recoveryFile.available()) {
      char letter = recoveryFile.read();
      password =  password + letter;
    }
    recoveryFile.close();
    writeEEPROMString(1, password);
    Serial.println("Recovery is set to " + password);
  } else {
    Serial.println("Unable to set recovery");
  }
}

void writeEEPROMString(int address, String data) {
  int i;
  int length = data.length();
  for (i = 0; i < 20; i++)  {
    if (i > length - 1) {
      EEPROM.write(address * 20 + i, 0);
    } else {
      EEPROM.write(address * 20 + i, data[i]);
    }
  }
}

String readEEPROMString(char address) {
  char data[20];
  int i;
  for (i = 0; i < 20; i++)  {
    data[i] = EEPROM.read(address * 20 + i);
  }
  return String(data);
}

void turnOff() {
  Serial.println("Off");
  lastAct = millis();
  analogWrite(PIN_POWER, LOW);
}

void resetAll() {
  command = "";
  newPassword = "";
  mode = NORMAL;
}

void errorCntInc(String msg) {
  beeps(1318.51, 100);
  delay(150);
  beeps(1318.51, 200);
  Serial.println(msg);
  errorCnt++;
  resetAll();
}

void doOpen() {
  if (isOpen) {
    return;
  }
  errorCnt = 0;
  isOpen = true;
  openTime = millis();
  resetAll();
  beeps(939.85, 200);
  analogWrite(PIN_DOOR_PWMB, 140);
  digitalWrite(PIN_DOOR_INB1, LOW);
  digitalWrite(PIN_DOOR_INB2, HIGH);
  delay(250);
  analogWrite(PIN_DOOR_PWMB, LOW);
  digitalWrite(PIN_DOOR_INB1, LOW);
  digitalWrite(PIN_DOOR_INB2, LOW);
  beeps(1318.51, 200);
  delay(200);
  beeps(1567.98, 200);
  delay(200);
  beeps(1975.53, 200);
  Serial.println("Open");
}

void doClose() {
  if (!isOpen) {
    return;
  }
  isOpen = false;
  beeps(1975.53, 200);
  analogWrite(PIN_DOOR_PWMB, 140);
  digitalWrite(PIN_DOOR_INB1, HIGH);
  digitalWrite(PIN_DOOR_INB2, LOW);
  delay(250);
  analogWrite(PIN_DOOR_PWMB, LOW);
  digitalWrite(PIN_DOOR_INB1, LOW);
  digitalWrite(PIN_DOOR_INB2, LOW);
  beeps(1567.98, 200);
  delay(200);
  beeps(1318.51, 200);
  delay(200);
  beeps(939.85, 200);
  Serial.println("Close");
}

void clicky() {
  beeps(939.85, 80);
}

void beeps(float frequency, int duration) {
  tone(PIN_BUZZER, frequency, duration);
}

void actionNormal(String cmd) {
  if (cmd == "A") {
    command = "";
    mode = CHANGE_PWD_CHECK;
    Serial.println("Change mode CHANGE_PWD_CHECK");
    return;
  }
  if (cmd == "B") {
    command = "";
    mode = GOD_MODE;
    Serial.println("Change mode GOD_MODE");
    return;
  }

  String oldPwd = readEEPROMString(0);
  if (cmd != oldPwd) {
    errorCntInc("Invalid password " + cmd + " " + oldPwd);
    return;
  }

  doOpen();
}

void actionPasswordCheck(String cmd) {
  String oldPwd = readEEPROMString(0);
  if (cmd != oldPwd) {
    errorCntInc("Invalid password " + cmd + " " + oldPwd);
    return;
  }
  mode = CHANGE_PWD_NEW;
  Serial.println("enter new pwd " + cmd);
}

void actionPasswordNew(String cmd) {
  newPassword = cmd;
  mode = CHANGE_PWD_CONFIRM;
  Serial.println("submit new pwd " + cmd);
}

void actionPasswordConfirm(String cmd) {
  if (cmd != newPassword) {
    errorCntInc("password not match " + cmd + " " + newPassword);
  }
  Serial.println("new pwd is saved " + cmd);
  writeEEPROMString(0, cmd);
}

void actionGodMode(String cmd) {
  String oldGodPwd = readEEPROMString(1);
  if (cmd != oldGodPwd) {
    errorCntInc("god mode invalid password " + cmd + " " + oldGodPwd);
    return;
  }
  mode = CHANGE_PWD_NEW;
  Serial.println("god mode allow enter new pwd " + cmd);
}

void action() {
  String cmd = command;
  command = "";
  switch (mode) {
    case NORMAL:
      actionNormal(cmd);
      break;
    case CHANGE_PWD_CHECK:
      actionPasswordCheck(cmd);
      break;
    case CHANGE_PWD_NEW:
      actionPasswordNew(cmd);
      break;
    case CHANGE_PWD_CONFIRM:
      actionPasswordConfirm(cmd);
      break;
    case GOD_MODE:
      actionGodMode(cmd);
      break;
    default:
      break;
  }
}

void loop() {
  if (errorCnt > 9) {
    delay(60000);
    errorCnt = errorCnt - 2;
    return;
  }
  if (millis() - openTime > 5000 && isOpen) {
    doClose();
  }
  if (millis() - lastAct > 20000) {
    turnOff();
    delay(5000);
  }
  if (command.length() > 30) {
    resetAll();
    return;
  }
  customKeypad.tick();
  while (customKeypad.available()) {
    keypadEvent e = customKeypad.read();
    if (e.bit.EVENT == KEY_JUST_RELEASED) {
      lastAct = millis();
      char c = (char)e.bit.KEY;
      clicky();
      switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
          command = command + c;
          break;
        case 'X':
          resetAll();
          break;
        case 'E':
          action();
          break;
        default:
          break;
      }
      Serial.println("command: " + command + " mode: " + mode + " newpwd: " + newPassword + " error: " + errorCnt);
    }
  }
  delay(10);
}
