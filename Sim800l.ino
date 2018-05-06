#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define resetPin 6
#define lcdBL 5
#define btnEnt 2
#define btnUp 3
#define btnDwn 7
#define MENU_LENGTH 6
#define MENU_ROW_HEIGHT 11
#define LCD_ROWS  4
#define RXLED 17
#define TXLED 30

Adafruit_PCD8544 display = Adafruit_PCD8544(15, 16, 9, 8, 4);

unsigned long startMillis, currentMillis;
const unsigned long period = 30000;  //the value is a number of milliseconds
bool Serial1Con, isItSleep, lights;
byte menuPos = 0;
byte menuScreen = 0;
byte markerPos = 0;
byte menuStartAt = 0;
String menu[6] = {"Connect", "Disconnect", "Request", "Sleep 24s", "Reset", "Light",};

void setup() {
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDwn, INPUT_PULLUP);
  pinMode(btnEnt, INPUT_PULLUP);
  pinMode(lcdBL, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(RXLED, OUTPUT);
  pinMode(TXLED, OUTPUT);
  digitalWrite(RXLED, HIGH);
  digitalWrite(TXLED, HIGH);
  digitalWrite(resetPin, HIGH);
  digitalWrite(lcdBL, LOW);
  lights = true;
  display.begin();
  display.setContrast(50);
  display.setRotation(2);
  display.clearDisplay();
  Serial1.begin(9600);
  //Serial.begin(9600);
  //while (!Serial);
  delay(10000);
  while (!checkSim800()) {
    display.clearDisplay();
    display.println(F("Sim800L is \nturned off or \nnot responding"));
    display.display();
    delay(2000);
    resetAll();
  }
  Serial1.write("AT+SAPBR=2,1\r");
  if (Serial1.readString().indexOf("0.0.0.0") == -1)
    Serial1Con = true;
  showMenu();
  startMillis = millis();
}

void loop() {
  currentMillis = millis();
  if (isButtonDown(btnDwn) == true) {
    if (isItSleep && !lights) {
      digitalWrite(lcdBL, LOW);
      lights = true;
    }
    if (menuPos < MENU_LENGTH - 1) {
      menuPos++;
      if (menuPos > 3) {
        menuStartAt++;
      }
      showMenu();
    }
    delay(100);
    startMillis = currentMillis;
  }
  if (isButtonDown(btnUp) == true) {
    if (isItSleep && !lights) {
      digitalWrite(lcdBL, LOW);
    }
    if (menuPos > 0) {
      menuPos--;
      if (menuStartAt > 0) {
        menuStartAt--;
      }
      showMenu();
    }
    delay(100);
    startMillis = currentMillis;
  }
  if (isButtonDown(btnEnt) == true) {
    if (isItSleep && !lights) {
      digitalWrite(lcdBL, LOW);
      lights = true;
    }
    if (menuPos == 0)
      connectGPRS();
    else if (menuPos == 1)
      disConnectGPRS();
    else if (menuPos == 2) {
      while (!Serial1Con)
        connectGPRS();
      String s = openURL("raw.githubusercontent.com/HA4ever37/Sim800l/master/Sim800.txt");
      if (s == "ERROR" || s == "")  {
        display.println(F("Something \nwent wrong!"));
        display.display();
        delay(1000);
        resetAll();
      }
      else {
        //Serial.print(F("Result: "));
        s = s.substring(s.indexOf("\r")+2, s.lastIndexOf("OK"));
        display.clearDisplay();
        digitalWrite(lcdBL, LOW);
        for (int i = 0; i < s.length(); i++) {
          Serial.print(s.charAt(i));
          display.print(s.charAt(i));
        }
        s="";
        display.display();
        digitalWrite(lcdBL, HIGH);
        delay(500);
        digitalWrite(lcdBL, LOW);
        while (!isButtonDown(btnEnt) && !isButtonDown(btnUp) && !isButtonDown(btnDwn));
        display.clearDisplay();
        display.display();
      }
    }
    else if (menuPos == 3)
      turnOff();
    else if (menuPos == 4)
      resetAll();
    else if (menuPos == 5)
      toggle();
    showMenu();
    delay(100);
    startMillis = currentMillis = millis();;
  }

  if (currentMillis - startMillis >= period) {
    isItSleep = true;
    Serial1Con = false;
    lights = false;
    digitalWrite(lcdBL, HIGH);
    digitalWrite(resetPin, LOW);
    attachInterrupt(digitalPinToInterrupt(btnEnt), pinInterrupt, RISING);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
    sleep_disable();
    startMillis = currentMillis;
  }
}

String openURL(String string) {
  Serial1.write("AT+HTTPINIT\r");
  display.clearDisplay();
  display.print(F("HTTP \nIntialization"));
  display.display();
  display.print(Serial1.readString());
  display.display();
  Serial1.write("AT+HTTPPARA=\"CID\",1\r");
  Serial1.readString();
  display.print(F("Sending URL.."));
  display.display();
  Serial1.write("AT+HTTPPARA=\"URL\",\"");
  Serial1.print(string + "\"\r");
  display.print(Serial1.readString());
  display.display();
  display.clearDisplay();
  Serial1.write("AT+HTTPPARA =\"REDIR\",1\r");
  Serial1.readString();
  Serial1.write("AT+HTTPSSL=1 \r");
  Serial1.readString();
  //Serial.println(Serial1.readString());
  Serial1.write("AT+HTTPACTION=0\r");
  Serial1.readString();
  if (Serial1.readString().indexOf(",200,") != -1) {
    display.println(F("Request \failed!"));
    display.display();
    delay(500);
    return "ERROR";
  }
  else {
    display.println(F("Request \naccepted!"));
    display.display();
    //Serial.println(Serial1.readString());
    delay(3000);
    while (Serial1.available() > 0)
      Serial1.read();
    display.print(F("Downloading \ndata.."));
    display.display();
    Serial1.write("AT+HTTPREAD\r");
    string = Serial1.readString();
    string.trim();
    //Serial.println(string);
    Serial1.write("AT+HTTPTERM\r");
    Serial1.readString();
    //Serial1.readString();
    return string;
  }
}

void resetAll() {
  display.clearDisplay();
  display.println(F("Restarting.."));
  display.display();
  Serial1.flush();
  //Serial.flush();
  digitalWrite(resetPin, LOW);
  delay(100);
  digitalWrite(resetPin, HIGH);
  delay(10000);
  Serial1Con = false;
}

void connectGPRS() {
  if (isItSleep) {
    wakeUp();
    isItSleep = false;
  }
  if (Serial1Con) {
    display.clearDisplay();
    display.println(F("Already \nconnected!"));
    display.display();
    delay(2000);
  }
  else {
    display.clearDisplay();
    display.println(F("Initializing \nSim800L.."));
    display.display();
    Serial1.write("AT+CSCLK=0\r");
    Serial1.readString();
    //Serial1.readString();
    Serial1.write("ATE0\r");
    delay(100);
    if (Serial1.available() > 0) {
      Serial1.readString();
      //Serial1.readString();
      //Serial.println(F("Setting up \naccess point.."));
      Serial1.write("AT+SAPBR=3,1,\"APN\",\"freedompop.foggmobile.com\"\r");
      Serial1.readString();
      //Serial1.readString();
      display.println(F("Connecting to \naccess point.."));
      display.display();
      Serial1.write("AT+SAPBR=1,1\r");
      delay(1000);
      if (Serial1.readString().indexOf("OK") != -1) {
        display.print(F("Connected!"));
        Serial1Con = true;
      }
      else {
        display.print(F("Failed to \nconnect!"));
        display.display();
        delay(1000);
        resetAll();
      }
    }
    else {
      display.print(F("Failed to \nconnect!"));
      display.display();
      delay(1000);
      resetAll();
    }
  }
}

void disConnectGPRS() {
  if (!Serial1Con) {
    display.clearDisplay();
    display.println(F("Already \ndisconnected!"));
    display.display();
    delay(2000);
  }
  else {
    display.clearDisplay();
    display.println(F("Disconnect \nGPRS.."));
    Serial1.write("AT+SAPBR=0,1\r");
    display.print(Serial1.readString());
    display.display();
    //Serial1.readString();
    delay(100);
    Serial1.write("AT+CSCLK=2\r");
    display.clearDisplay();
    display.print(Serial1.readString());
    display.display();
    //Serial1.readString();
    Serial1Con = false;
  }
}

void turnOff() {
  Serial1Con = false;
  display.clearDisplay();
  digitalWrite(lcdBL, HIGH);
  Serial1.write("AT+CPOWD=0\r");
  Serial1.readString();
  display.clearDisplay();
  display.display();
  //display.command( PCD8544_FUNCTIONSET | PCD8544_POWERDOWN);
  //Serial1.readString();
  for (byte i = 0; i < 4; i++)
    myWatchdogEnable (0b100001);  // 8 seconds
  //  myWatchdogEnable (0b100000);  // 4 seconds
  startMillis = currentMillis;
}

ISR(WDT_vect) {
  wdt_disable();  // disable watchdog
}

void myWatchdogEnable(const byte interval) {
  MCUSR = 0;                          // reset various flags
  WDTCSR |= 0b00011000;               // see docs, set WDCE, WDE
  WDTCSR =  0b01000000 | interval;    // set WDIE, and appropriate delay
  wdt_reset();
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_mode();            // now goes to Sleep and waits for the interrupt
}

void toggle() {
  digitalWrite(lcdBL, !digitalRead(lcdBL));
}

bool isButtonDown(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(30);
    if (digitalRead(pin) == LOW) {
      return true;
    }
    return false;
  }
  return false;
}

void showMenu() {
  startMillis = currentMillis;
  for (int i = menuStartAt; i < (menuStartAt + LCD_ROWS); i++) {
    int markerY = (i - menuStartAt) * MENU_ROW_HEIGHT;
    if (i == menuPos) {
      display.setTextColor(WHITE, BLACK);
      display.fillRect(0, markerY, display.width(), MENU_ROW_HEIGHT, BLACK);
    } else {
      display.setTextColor(BLACK, WHITE);
      display.fillRect(0, markerY, display.width(), MENU_ROW_HEIGHT, WHITE);
    }

    if (i >= MENU_LENGTH) {
      continue;
    }
    display.setCursor(2, markerY + 2);
    display.print(menu[i]);
  }
  display.display();
}

bool checkSim800() {
  while (Serial1.available() > 0)
    Serial1.read();
  Serial1.write("AT\r");
  delay(100);
  if (Serial1.available() > 0) {
    Serial1.read();
    return true;
  }
  else
    return false;
}

void wakeUp() {
  digitalWrite(resetPin, HIGH);
  display.clearDisplay();
  display.println(F("Waking up \nSim800L.."));
  display.display();
  delay(10000);
  while (!checkSim800()) {
    display.clearDisplay();
    display.println(F("Sim800L is \nturned off or \nnot responding"));
    display.display();
    delay(2000);
    resetAll();
  }
}

void pinInterrupt(void) {
  detachInterrupt(0);
}

void ledTx( boolean on)
{
  if ( on)
  {
    // led on. The led is connected to VCC. Make pin low to turn led on.
    pinMode( LED_BUILTIN_TX, OUTPUT);    // pin as output.
    digitalWrite( LED_BUILTIN_TX, LOW);  // pin low

    // These two lines will do the same:
    //    bitSet( DDRD, 5);
    //    bitClear( PORTD, 5);
  }
  else
  {
    // led off
    // turn it off, by setting it as input, so the serial activity can't turn it on.
    // If the internal pullup resistor is enabled or not, that does not matter,
    // since the led it connected to VCC.
    pinMode( LED_BUILTIN_TX, INPUT);

    // This line will do the same:
    //    bitClear( DDRD, 5);        // set pin as input
  }
}

void ledRx( boolean on)
{
  if ( on)
  {
    pinMode( LED_BUILTIN_RX, OUTPUT);
    digitalWrite( LED_BUILTIN_RX, LOW);
    //    bitSet( DDRB, 0);
    //    bitClear( PORTB, 0);
  }
  else
  {
    pinMode( LED_BUILTIN_RX, INPUT);
    //    bitClear( DDRB, 0);
  }
}
