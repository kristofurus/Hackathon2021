#include "Adafruit_Si7021.h"
#include "MQ7.h"
#include "Adafruit_Keypad.h"
#include <LiquidCrystal.h>

// keypad config 
#define KEYPAD_PID3844
#define R1    9
#define R2    10
#define R3    11
#define R4    12
#define C1    A5
#define C2    A4
#define C3    A3
#define C4    A2

#include "keypad_config.h"

// MQ-7 sensor conig
#define A_PIN A0
#define VOLTAGE 13

// motor config
#define IN1 7
#define IN2 8
#define ENA 6

// lcd config
#define RS  0
#define EN  1
#define D4  2
#define D5  3
#define D6  4
#define D7  5

// buzzer config
#define BUZZER A1

bool changeSpeed = 0, isMotorOn = 0;
int Engspeed = 255, dispOption = 1, engSpeedTextCount = 0;
char engSpeedText[3] = {'2', '5', '5'};
double COppm, hum, temp;
unsigned long actualTime = 0, rememberTimeHum = 0, rememberedTimePrint = 0, rememberedTimeKeyboard = 0, rememberTimeCO = 0, rememberTimeVent = 0;

MQ7 mq7(A_PIN, VOLTAGE);
Adafruit_Si7021 sensor = Adafruit_Si7021();
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void setup() {

  //lcd initialization
  lcd.begin(16,2);

  //SI7021 sensor initialization
  if (!sensor.begin()) {
    lcd.setCursor(0,0);
    lcd.print("Sensor not found");
    while (true);
  }

  //MQ calibration
  lcd.setCursor(0,0);
  lcd.print("Calibrating MQ7");
  mq7.calibrate();
  lcd.clear();

  //motor initialization
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  //keyboard initialization
  customKeypad.begin();

  // buzzer initialization
  pinMode(BUZZER, OUTPUT);

  // first measurement
  hum = sensor.readHumidity();
  temp = sensor.readTemperature();
  COppm = mq7.readPpm();
  
}

void loop() {
  actualTime = millis();
  
  // reading data from measurement
  if (actualTime - rememberTimeHum >= 60000UL) {
    hum = sensor.readHumidity();
    temp = sensor.readTemperature();
    rememberTimeHum = actualTime;
  }

  if(actualTime - rememberTimeCO >= 5000UL) {
    COppm = mq7.readPpm();
    if (COppm >= 100.0) {
      digitalWrite(BUZZER, HIGH);
    } else {
      digitalWrite(BUZZER, LOW);
    }
  }

  //reading information from keyboard
  if (actualTime - rememberedTimePrint >= 50UL) {
    customKeypad.tick();
    if(customKeypad.available()) {
      keypadEvent e = customKeypad.read();
      if(e.bit.EVENT == KEY_JUST_PRESSED) {
        displayLCD(e.bit.KEY);
      }
    }
    rememberedTimeKeyboard = actualTime;
  }

  if(actualTime - rememberedTimePrint >= 1000UL) {
    showData();
    rememberedTimePrint = actualTime;
  }

  if(actualTime - rememberTimeVent >= 1000UL) {
    if(hum > 60 || COppm >= 100) {
      steerMotor(1, 255);
    } else if (isMotorOn) {
      steerMotor(1, Engspeed);
    } else {
      steerMotor(0, Engspeed);
    }
    rememberTimeVent = actualTime;
  }
  
}

void displayLCD(char pressedButton) {
  switch(pressedButton) {
    case 'A':
      // show humidity
      dispOption = 1;
      break;
    case 'B':
      // show temperature
      dispOption = 2;
      break;
    case 'C':
      // show CO ppm
      dispOption = 3;
      break;
    case 'D':
      // show ventilation options 
      dispOption = 4;
      break;
    case '*':
      isMotorOn = !isMotorOn;
      break;
    case '#':
      if(dispOption == 4) {
        if(!changeSpeed) {
          changeSpeed = 1;
          engSpeedText[0] = {'0'};
          engSpeedText[1] = {'0'};
          engSpeedText[2] = {'0'};
          engSpeedTextCount = 0;
        } else {
          Engspeed = atoi(engSpeedText);
          if(Engspeed > 255) {
            Engspeed = 255;
            engSpeedText[0] = {'2'};
            engSpeedText[1] = {'5'};
            engSpeedText[2] = {'5'};
          }
          if(Engspeed < 0) {
            Engspeed = 0;
            engSpeedText[0] = {'0'};
            engSpeedText[1] = {'0'};
            engSpeedText[2] = {'0'};
          }
        }
      }
      break;
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
      if(dispOption == 4 && changeSpeed) {
        engSpeedText[engSpeedTextCount++] = pressedButton;
        if(engSpeedTextCount > 2) {
          engSpeedTextCount = 0;
          Engspeed = atoi(engSpeedText);
          if(Engspeed > 255) {
            Engspeed = 255;
            engSpeedText[0] = {'2'};
            engSpeedText[1] = {'5'};
            engSpeedText[2] = {'5'};
          }
          if(Engspeed < 0) {
            Engspeed = 0;
            engSpeedText[0] = {'0'};
            engSpeedText[1] = {'0'};
            engSpeedText[2] = {'0'};
            changeSpeed = 0;
          }
          changeSpeed = 0;
        }
      }
      break;
  }
  showData();
}

void showData() {
  lcd.clear();
  if(COppm >= 100.0) {
    lcd.setCursor(0,0);
    lcd.print("ALERT!");
    lcd.setCursor(0,1);
    lcd.print("high CO level");
  } else {
    switch(dispOption) {
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("Humidity:         ");
        lcd.setCursor(0, 1);
        lcd.print(hum); 
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("Temperature:     ");
        lcd.setCursor(0, 1);
        lcd.print(temp);
        break;
      case 3:
        lcd.setCursor(0, 0);
        lcd.print("CO ppm:         ");
        lcd.setCursor(0, 1);
        lcd.print(COppm);
        break;
      case 4:
        lcd.setCursor(0, 0);
        lcd.print("Motor settings: ");
        lcd.setCursor(0, 1);
        lcd.print(engSpeedText);
        lcd.setCursor(3,1);
        lcd.print("         ");
        break;
    }
  }
}

//function for managing
void steerMotor(int funct, int Engspeed) {
  switch (funct) {
    case 0:
      //stop rotating
      analogWrite(ENA, Engspeed);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      break;
    case 1:
      // rotate forward
      analogWrite(ENA, Engspeed);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      break;
    case 2:
      // rotate backward
      analogWrite(ENA, Engspeed);
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      break;
    default:
      break;
  }
}
