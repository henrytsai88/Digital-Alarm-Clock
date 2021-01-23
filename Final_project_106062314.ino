#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include "DS1302.h"
#define KEY_ROWS 4
#define KEY_COLS 4
#define CLOCK 0
#define SETTING 1
#define RINGING 2
#define buzzer 13
#define microphone A1
#define C 523
#define D 587
#define E 659
#define F 698
#define G 783
#define A 880
#define B 987

/* Init DS1302 */
DS1302 rtc(4, 3, 2);

/* Init LCD */
LiquidCrystal_I2C lcd(0x3F, 16, 2);

/* Init Keypad */
char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte colPins[KEY_COLS] = {8, 7, 6, 5};
byte rowPins[KEY_ROWS] = {12, 11, 10, 9};

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);

/* Global Variables */
char WeekdayStr[10] = "";
char DateStr[11] = "";
char TimeStr[10] = "";
char key;
int index = 0;
int page = 0;
bool isMasking = false;
long currentMillis = 0;
long alarmMillis = 0;
int state = CLOCK;
int volume = 0;
int brightness = 0;
char *msg = "Alarm time";
char alarmTimeStr[9] = "09:30:00";
const char maximumTime[9] = "29:59:59";
const byte whiteblock[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
const byte hollowblock[8] = {B11111, B10001, B10001, B10001, B10001, B10001, B10001, B11111};
const int music[16] = {C, D, E, F, G, G, A, F, E, E, D, D, C, C, C, C};
int i;

/* Define tasks */
void ControlTask(void);
void DisplayTask(void);
void BuzzerTask(void);

void setup() {
  Serial.begin(9600);
  rtc.halt(false);
  pinMode(buzzer, OUTPUT);
  pinMode(microphone, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, whiteblock);
  lcd.createChar(1, hollowblock);
  /* Set up tasks to run independently */
  xTaskCreate(ControlTask, (const portCHAR *)"CONTROL", 110, NULL, 1, NULL);
  xTaskCreate(DisplayTask, (const portCHAR *)"DISPLAY", 120, NULL, 1, NULL);
  xTaskCreate(BuzzerTask, (const portCHAR *)"BUZZER", 45, NULL, 1, NULL);
}

void loop() {}

void ControlTask() {
  /* Handle all inputs(sensors) */
  for (;;) {
    vTaskDelay(10);
    /* Photo Resistor */
    brightness = analogRead(A0) / 100;
    if (brightness >= 5) lcd.backlight();
    else lcd.noBacklight();
    /* Microphone */
    volume = analogRead(microphone) / 12;
    Serial.println(analogRead(microphone));
    /* Keypad */
    key = myKeypad.getKey();
    switch (state) {
      case CLOCK:
        strcpy(WeekdayStr, rtc.getDOWStr());
        strcpy(DateStr, rtc.getDateStr());
        strcpy(TimeStr, rtc.getTimeStr());
        if (key == '#') {
          state = SETTING;
        }
        if (strcmp(TimeStr, alarmTimeStr) == 0 || key == '*') {
          state = RINGING;
          alarmMillis = millis();
        }
        break;
      case SETTING:
        if (millis() > currentMillis) {
          isMasking = !isMasking;
          if (isMasking) currentMillis = millis() + 800;
          else currentMillis = millis() + 800;
        }
        if (key) {
          if (key == 'B') {
            // Next page
            page = (page + 2) % 3;
            if (page == 0) msg = "Alarm time";
            else if (page == 1) msg = "Volume";
            else if (page == 2) msg = "Brightness";
          }
          else if (key == 'C') {
            // Previous page
            page = (page + 1) % 3;
            if (page == 0) msg = "Alarm time";
            else if (page == 1) msg = "Volume";
            else if (page == 2) msg = "Brightness";
          }
          if (page == 0) {
            if (key == '2') {
              alarmTimeStr[index] = (alarmTimeStr[index] == maximumTime[index]) ? '0' : alarmTimeStr[index] + 1;
            }
            else if (key == '8') {
              alarmTimeStr[index] = (alarmTimeStr[index] == '0') ? maximumTime[index] : alarmTimeStr[index] - 1;
            }
            else if (key == '4' && index > 0) {
              if (index == 3 || index == 6) index -= 2;
              else index--;
              isMasking = true;
              currentMillis = millis() + 800;
            }
            else if (key == '6' && index < 7) {
              if (index == 1 || index == 4) index += 2;
              else index++;
              isMasking = true;
              currentMillis = millis() + 800;
            }
          }
        }
        if (key == '#') {
          state = CLOCK;
        }
        break;
      case RINGING:
        if (key == '#') {
          state = CLOCK;
        }
        break;
    }
  }
}
void DisplayTask() {
  for (;;) {
    vTaskDelay(10);
    switch (state) {
      case CLOCK:
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(DateStr);
        lcd.setCursor(13, 1);
        lcd.print(WeekdayStr);
        lcd.setCursor(4, 0);
        lcd.print(TimeStr);
        break;
      case SETTING:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(msg);
        lcd.setCursor(0, 1);
        if (page == 0) lcd.print(alarmTimeStr);
        else if (page == 1) {
          for (int i = 0; i < 10; i++) {
            if (i < volume) lcd.write(0);
            else lcd.write(1);
          }
        }
        else if (page == 2) {
          for (int i = 0; i < 10; i++) {
            if (i < brightness) lcd.write(0);
            else lcd.write(1);
          }
        }
        if (page == 0 && isMasking) {
          lcd.setCursor(index, 1);
          lcd.write(0);
        }
        lcd.setCursor(13, 1);
        lcd.print(page + 1);
        lcd.print("/3");
        break;
      case RINGING:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wake up!(^_^)");
        break;
    }
  }
}
void BuzzerTask() {
  for (;;) {
    vTaskDelay(10);
    noTone(buzzer);
    i = 0;
    if(volume > 5) {
      while(i < 16 && state == RINGING){
        if (millis() > alarmMillis + 500){
          tone(buzzer, music[i++]);
          alarmMillis = millis();
        }
      }
    } else {
      while(i < 16 && state == RINGING){
        if (millis() > alarmMillis + 500){
          tone(buzzer, music[i++] / 2);
          alarmMillis = millis();
        }
      }
    }
  }
}
