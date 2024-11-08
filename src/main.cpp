#include <Arduino.h>
#include "control.h"
#include <Preferences.h>

#include "PCF8575.h"
PCF8575 pcf8575(0x22);

Preferences Settings;

char *secondsToHHMMSS(int total_seconds)
{
  int hours, minutes, seconds;

  hours = total_seconds / 3600;         // Divide by number of seconds in an hour
  total_seconds = total_seconds % 3600; // Get the remaining seconds
  minutes = total_seconds / 60;         // Divide by number of seconds in a minute
  seconds = total_seconds % 60;         // Get the remaining seconds

  // Format the output string
  static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
  sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
  return hhmmss_str;
}

// Declaration for LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

byte enterChar[] = {
    B10000,
    B10000,
    B10100,
    B10110,
    B11111,
    B00110,
    B00100,
    B00000};

byte fastChar[] = {
    B00110,
    B01110,
    B00110,
    B00110,
    B01111,
    B00000,
    B00100,
    B01110};
byte slowChar[] = {
    B00011,
    B00111,
    B00011,
    B11011,
    B11011,
    B00000,
    B00100,
    B01110};

// Declaration of LCD Variables
const int NUM_MAIN_ITEMS = 3;
const int NUM_SETTING_ITEMS = 2;
const int NUM_TESTMACHINE_ITEMS = 5;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool settingFlag, settingEditFlag, testMenuFlag, runAutoFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"TEST MACHINE", "ENTER TO TEST"},
    {"RUN AUTO", "ENTER TO RUN AUTO"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"LENGTH", "SEC"},
    {"SAVE"}};

int parametersTimer[NUM_SETTING_ITEMS] = {1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {1200};

String testmachine_items[NUM_TESTMACHINE_ITEMS] = { // array with item names
    "CUTTER",
    "LINEAR F",
    "LINEAR R",
    "ROLLER",
    "EXIT"};

Control rCutter(0);
Control rLinearF(0);
Control rLinearR(0);
Control rRoller(0);

int cCutter = P2;
int cLinearF = P0;
int cLinearR = P1;
int cRoller = P3;

int endLinear = 32;
int resetChopper = 35;

void initRelays()
{
  pcf8575.pinMode(cCutter, OUTPUT);
  pcf8575.digitalWrite(cCutter, LOW);

  pcf8575.pinMode(cLinearF, OUTPUT);
  pcf8575.digitalWrite(cLinearF, LOW);

  pcf8575.pinMode(cLinearR, OUTPUT);
  pcf8575.digitalWrite(cLinearR, LOW);

  pcf8575.pinMode(cRoller, OUTPUT);
  pcf8575.digitalWrite(cRoller, LOW);

  pcf8575.begin();
}

void saveSettings()
{
  Settings.putInt("length", parametersTimer[0]);
  Serial.println("---- Saving Timer  Settings ----");
  Serial.println("Length Time : " + String(parametersTimer[0]));
  Serial.println("---- Saving Timer  Settings ----");
}
void loadSettings()
{
  Serial.println("---- Start Reading Settings ----");
  parametersTimer[0] = Settings.getInt("length");
  Serial.println("Length Timer : " + String(parametersTimer[0]));
  Serial.println("---- End Reading Settings ----");
}

static const int buttonPin = 13;
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 12;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 15;
int buttonStatePrevious3 = HIGH;

unsigned long minButtonLongPressDuration = 2000;
unsigned long buttonLongPressUpMillis;
unsigned long buttonLongPressDownMillis;
unsigned long buttonLongPressEnterMillis;
bool buttonStateLongPressUp = false;
bool buttonStateLongPressDown = false;
bool buttonStateLongPressEnter = false;

const int intervalButton = 50;
unsigned long previousButtonMillis;
unsigned long buttonPressDuration;
unsigned long currentMillis;

const int intervalButton2 = 50;
unsigned long previousButtonMillis2;
unsigned long buttonPressDuration2;
unsigned long currentMillis2;

const int intervalButton3 = 50;
unsigned long previousButtonMillis3;
unsigned long buttonPressDuration3;
unsigned long currentMillis3;

unsigned long currentMillisRunAuto;
unsigned long previousMillisRunAuto;
unsigned long intervalRunAuto = 1000;

void InitializeButtons()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  pinMode(endLinear, INPUT);
  pinMode(resetChopper, INPUT);
}

void StopAll()
{
  rCutter.stop();
  rLinearF.stop();
  rLinearR.stop();
  rRoller.stop();

  pcf8575.digitalWrite(cCutter, HIGH);
  pcf8575.digitalWrite(cLinearF, HIGH);
  pcf8575.digitalWrite(cLinearR, HIGH);
  pcf8575.digitalWrite(cRoller, HIGH);
}

bool RunAutoFlag = false;
int RunAutoStatus = 0;

void runAuto()
{
  /*
  Case 1: Move 1 inch Roller and Linear
  Case 2: Run the Cutter Until Limit Switch is Reached
  Case 3: Check if the Linear is Full Stroke if false move to Case 1
  */

  switch (RunAutoStatus)
  {
  case 1:
    /* code */
    break;
  case 2:
    /* code */
    break;
  case 3:
    /* code */
    break;

  default:
    break;
  }
}

void readButtonUpState()
{
  if (currentMillis - previousButtonMillis > intervalButton)
  {
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && buttonStatePrevious == HIGH && !buttonStateLongPressUp)
    {
      buttonLongPressUpMillis = currentMillis;
      buttonStatePrevious = LOW;
    }
    buttonPressDuration = currentMillis - buttonLongPressUpMillis;
    if (buttonState == LOW && !buttonStateLongPressUp && buttonPressDuration >= minButtonLongPressDuration)
    {
      buttonStateLongPressUp = true;
    }
    if (buttonStateLongPressUp == true)
    {
      // Insert Fast Scroll Up
      refreshScreen = true;
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
          {
            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
          }
          else
          {
            parametersTimer[currentSettingScreen] += 1;
          }
        }
        else
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            currentSettingScreen = 0;
          }
          else
          {
            currentSettingScreen++;
          }
        }
      }
      else if (testMenuFlag == true)
      {
        if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
        {
          currentTestMenuScreen = 0;
        }
        else
        {
          currentTestMenuScreen++;
        }
      }
      else
      {
        if (currentMainScreen == NUM_MAIN_ITEMS - 1)
        {
          currentMainScreen = 0;
        }
        else
        {
          currentMainScreen++;
        }
      }
    }

    if (buttonState == HIGH && buttonStatePrevious == LOW)
    {
      buttonStatePrevious = HIGH;
      buttonStateLongPressUp = false;
      if (buttonPressDuration < minButtonLongPressDuration)
      {
        // Short Scroll Up
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
            {
              parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
            }
            else
            {
              parametersTimer[currentSettingScreen] += 1;
            }
          }
          else
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              currentSettingScreen = 0;
            }
            else
            {
              currentSettingScreen++;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentTestMenuScreen = 0;
          }
          else
          {
            currentTestMenuScreen++;
          }
        }
        else
        {
          if (currentMainScreen == NUM_MAIN_ITEMS - 1)
          {
            currentMainScreen = 0;
          }
          else
          {
            currentMainScreen++;
          }
        }
      }
    }
    previousButtonMillis = currentMillis;
  }
}

void readButtonDownState()
{
  if (currentMillis2 - previousButtonMillis2 > intervalButton2)
  {
    int buttonState2 = digitalRead(buttonPin2);
    if (buttonState2 == LOW && buttonStatePrevious2 == HIGH && !buttonStateLongPressDown)
    {
      buttonLongPressDownMillis = currentMillis2;
      buttonStatePrevious2 = LOW;
    }
    buttonPressDuration2 = currentMillis2 - buttonLongPressDownMillis;
    if (buttonState2 == LOW && !buttonStateLongPressDown && buttonPressDuration2 >= minButtonLongPressDuration)
    {
      buttonStateLongPressDown = true;
    }
    if (buttonStateLongPressDown == true)
    {
      refreshScreen = true;
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] <= 0)
          {
            parametersTimer[currentSettingScreen] = 0;
          }
          else
          {
            parametersTimer[currentSettingScreen] -= 1;
          }
        }
        else
        {
          if (currentSettingScreen == 0)
          {
            currentSettingScreen = NUM_SETTING_ITEMS - 1;
          }
          else
          {
            currentSettingScreen--;
          }
        }
      }
      else if (testMenuFlag == true)
      {
        if (currentTestMenuScreen == 0)
        {
          currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
        }
        else
        {
          currentTestMenuScreen--;
        }
      }
      else
      {
        if (currentMainScreen == 0)
        {
          currentMainScreen = NUM_MAIN_ITEMS - 1;
        }
        else
        {
          currentMainScreen--;
        }
      }
    }

    if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
    {
      buttonStatePrevious2 = HIGH;
      buttonStateLongPressDown = false;
      if (buttonPressDuration2 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (currentSettingScreen == 2)
            {
              if (parametersTimer[currentSettingScreen] <= 2)
              {
                parametersTimer[currentSettingScreen] = 2;
              }
              else
              {
                parametersTimer[currentSettingScreen] -= 1;
              }
            }
            else
            {
              if (parametersTimer[currentSettingScreen] <= 0)
              {
                parametersTimer[currentSettingScreen] = 0;
              }
              else
              {
                parametersTimer[currentSettingScreen] -= 1;
              }
            }
          }
          else
          {
            if (currentSettingScreen == 0)
            {
              currentSettingScreen = NUM_SETTING_ITEMS - 1;
            }
            else
            {
              currentSettingScreen--;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == 0)
          {
            currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
          }
          else
          {
            currentTestMenuScreen--;
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            currentMainScreen = NUM_MAIN_ITEMS - 1;
          }
          else
          {
            currentMainScreen--;
          }
        }
      }
    }
    previousButtonMillis2 = currentMillis2;
  }
}

void readButtonEnterState()
{
  if (currentMillis3 - previousButtonMillis3 > intervalButton3)
  {
    int buttonState3 = digitalRead(buttonPin3);
    if (buttonState3 == LOW && buttonStatePrevious3 == HIGH && !buttonStateLongPressEnter)
    {
      buttonLongPressEnterMillis = currentMillis3;
      buttonStatePrevious3 = LOW;
    }
    buttonPressDuration3 = currentMillis3 - buttonLongPressEnterMillis;
    if (buttonState3 == LOW && !buttonStateLongPressEnter && buttonPressDuration3 >= minButtonLongPressDuration)
    {
      buttonStateLongPressEnter = true;
    }
    if (buttonStateLongPressEnter == true)
    {
      // Insert Fast Scroll Enter
      Serial.println("Long Press Enter");
    }

    if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
    {
      buttonStatePrevious3 = HIGH;
      buttonStateLongPressEnter = false;
      if (buttonPressDuration3 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (currentMainScreen == 0 && settingFlag == true)
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            settingFlag = false;
            saveSettings();
            loadSettings();
            currentSettingScreen = 0;
            // setTimers();
          }
          else
          {
            if (settingEditFlag == true)
            {
              settingEditFlag = false;
            }
            else
            {
              settingEditFlag = true;
            }
          }
        }
        else if (currentMainScreen == 1 && testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentMainScreen = 0;
            currentTestMenuScreen = 0;
            testMenuFlag = false;
            // stopAllMotors();
          }
          else if (currentTestMenuScreen == 0)
          {
            if (rCutter.getMotorState() == false)
            {
              rCutter.relayOn();
              pcf8575.digitalWrite(cCutter, false);
            }
            else
            {
              rCutter.relayOff();
              pcf8575.digitalWrite(cCutter, true);
            }
          }
          else if (currentTestMenuScreen == 1)
          {
            if (rLinearF.getMotorState() == false)
            {
              rLinearF.relayOn();
              pcf8575.digitalWrite(cLinearF, false);
            }
            else
            {
              rLinearF.relayOff();
              pcf8575.digitalWrite(cLinearF, true);
            }
          }
          else if (currentTestMenuScreen == 2)
          {
            if (rLinearR.getMotorState() == false)
            {
              rLinearR.relayOn();
              pcf8575.digitalWrite(cLinearR, false);
            }
            else
            {
              rLinearR.relayOff();
              pcf8575.digitalWrite(cLinearR, true);
            }
          }
          else if (currentTestMenuScreen == 3)
          {
            if (rRoller.getMotorState() == false)
            {
              rRoller.relayOn();
              pcf8575.digitalWrite(cRoller, false);
            }
            else
            {
              rRoller.relayOff();
              pcf8575.digitalWrite(cRoller, true);
            }
          }
        }
        else if (currentMainScreen == 2 && runAutoFlag == true)
        {
          // stopAllMotors();
          runAutoFlag = false;
          // runAutoStatus = 0;
          // runCookingStatus = 0;
        }
        else
        {
          if (currentMainScreen == 0)
          {
            settingFlag = true;
          }
          else if (currentMainScreen == 1)
          {
            testMenuFlag = true;
          }
          else if (currentMainScreen == 2)
          {
            runAutoFlag = true;
            // runAutoStatus = 1;
            // runCookingStatus = 1;
            // TimerCooking.start();
          }
        }
      }
    }
    previousButtonMillis3 = currentMillis3;
  }
}

void ReadButtons()
{
  currentMillis = millis();
  currentMillis2 = millis();
  currentMillis3 = millis();
  readButtonEnterState();
  readButtonUpState();
  readButtonDownState();
}

void initializeLCD()
{
  lcd.init();
  lcd.clear();
  lcd.createChar(0, enterChar);
  lcd.createChar(1, fastChar);
  lcd.createChar(2, slowChar);
  lcd.backlight();
  refreshScreen = true;
}

void printTestScreen(String TestMenuTitle, String Job, bool Status, bool ExitFlag)
{
  lcd.clear();
  lcd.print(TestMenuTitle);
  if (ExitFlag == false)
  {
    lcd.setCursor(0, 2);
    lcd.print(Job);
    lcd.print(" : ");
    if (Status == true)
    {
      lcd.print("ON");
    }
    else
    {
      lcd.print("OFF");
    }
  }

  if (ExitFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Exit Test");
  }
  else
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Run Test");
  }
  refreshScreen = false;
}

void printMainMenu(String MenuItem, String Action)
{
  lcd.clear();
  lcd.print(MenuItem);
  lcd.setCursor(0, 3);
  lcd.write(0);
  lcd.setCursor(2, 3);
  lcd.print(Action);
  refreshScreen = false;
}

void printSettingScreen(String SettingTitle, String Unit, int Value, bool EditFlag, bool SaveFlag)
{
  lcd.clear();
  lcd.print(SettingTitle);
  lcd.setCursor(0, 1);

  if (SaveFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    lcd.print("ENTER TO SAVE ALL");
  }
  else
  {
    lcd.print(Value);
    lcd.print(" ");
    lcd.print(Unit);
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    if (EditFlag == false)
    {
      lcd.print("ENTER TO EDIT");
    }
    else
    {
      lcd.print("ENTER TO SAVE");
    }
  }
  refreshScreen = false;
}

void printRunAuto(String SettingTitle, String Process, String TimeRemaining)
{
  lcd.clear();
  lcd.print(SettingTitle);
  lcd.setCursor(0, 1);
  lcd.print(Process);
  lcd.setCursor(0, 2);
  lcd.print(TimeRemaining);
  refreshScreen = false;
}

void printScreen()
{
  if (settingFlag == true)
  {
    if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
    {
      printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, true);
    }
    else
    {
      printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, false);
    }
  }
  else if (testMenuFlag == true)
  {
    switch (currentTestMenuScreen)
    {
    case 0:
      // printTestScreen(testmachine_items[currentTestMenuScreen], "Status", ContactorVFD.getMotorState(), false);
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", rCutter.getMotorState(), false);
      break;
    case 1:
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", rLinearF.getMotorState(), false);
      break;
    case 2:
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", rLinearR.getMotorState(), false);
      break;
    case 3:
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", rRoller.getMotorState(), false);
      break;
    case 4:
      printTestScreen(testmachine_items[currentTestMenuScreen], "", true, true);
      break;
    default:
      break;
    }
  }
  else if (runAutoFlag == true)
  {
    // switch (runAutoStatus)
    // {
    // case 1:
    //   switch (runCookingStatus)
    //   {
    //   case 1:
    //     printRunAuto("Cooking", "Pouring Water", TimerCooking.getTimeRemaining());
    //     break;
    //   case 2:
    //     printRunAuto("Cooking", "Boiling", TimerCooking.getTimeRemaining());
    //     break;
    //   default:
    //     break;
    //   }
    //   break;

    // case 2:
    //   printRunAuto("Drying", "N/A", TimerDrying.getTimeRemaining());
    //   break;
    // default:
    //   break;
    // }
  }
  else
  {
    printMainMenu(menu_items[currentMainScreen][0], menu_items[currentMainScreen][1]);
  }
}

void setup()
{
  Serial.begin(9600);
  initializeLCD();
  InitializeButtons();
  Settings.begin("timerSetting", false);
  initRelays();
  // saveSettings();
  // loadSettings();
}

void loop()
{
  ReadButtons();
  if (refreshScreen == true)
  {
    printScreen();
    refreshScreen = false;
  }

  if (runAutoFlag == true)
  {
    // runAuto();

    unsigned long currentMillisRunAuto = millis();
    if (currentMillisRunAuto - previousMillisRunAuto >= intervalRunAuto)
    {
      previousMillisRunAuto = currentMillisRunAuto;
      refreshScreen = true;
    }
  }
}