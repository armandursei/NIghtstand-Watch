/*
 * Nightstand Watch
 *
 * Bedside clock with a date/time display, 12/24-hour format, a one-shot
 * buzzer alarm and the room temperature, controlled by three push buttons
 * or an IR remote.
 *
 * Hardware:
 *   - Arduino Uno or compatible board
 *   - Nokia 5110 LCD (PCD8544, 84x48), driven over software SPI
 *   - DS3231 RTC module (I2C: SDA/SCL), also used as the temperature sensor
 *   - IR receiver (NEC remote)
 *   - Passive buzzer
 *   - 3 push buttons (to GND, internal pull-ups)
 *   - Analog humidity sensor (sampled, not shown on the display)
 *
 * Pin map:
 *   D2  buzzer           D8  button 1 (mode / next field)
 *   D3  LCD RST          D9  button 2 (increment / 12-24h toggle)
 *   D4  LCD CE (CS)      D10 button 3 (decrement / set alarm)
 *   D5  LCD DC           D11 IR receiver
 *   D6  LCD DIN          A1  humidity sensor
 *   D7  LCD CLK          SDA/SCL  DS3231
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <RTClib.h>
#include <IRremote.h>

// Nokia 5110 LCD pins (software SPI)
const int lcdClkPin = 7;
const int lcdDinPin = 6;
const int lcdDcPin  = 5;
const int lcdCsPin  = 4;
const int lcdRstPin = 3;

Adafruit_PCD8544 display = Adafruit_PCD8544(lcdClkPin, lcdDinPin, lcdDcPin, lcdCsPin, lcdRstPin);
RTC_DS3231 rtc;

const int buzzerPin = 2;
const int button1Pin = 8;   // Enter time editing / cycle through fields
const int button2Pin = 9;   // Increment the current field / toggle 12-24h
const int button3Pin = 10;  // Decrement the current field / set the alarm
const int irReceiverPin = 11;

// Raw NEC codes of the remote keys used by the clock
const uint32_t irKey1    = 0xBA45FF00;
const uint32_t irKey2    = 0xB946FF00;
const uint32_t irKey3    = 0xB847FF00;
const uint32_t irKeyUp   = 0xE718FF00;
const uint32_t irKeyDown = 0xAD52FF00;

int sensorValue = 0;  // Raw reading from the humidity sensor
int humidity;

IRrecv irrecv(irReceiverPin);

bool editingMode = false;  // Editing the current date/time
bool settingAlarm = false; // Editing the alarm date/time
bool ringAlarm = false;    // Alarm armed
int editField = 0;         // Field being edited: 1=year ... 6=seconds
int editFieldAlarm = 0;
bool is24HourFormat = true;

DateTime currentTime;
int dayOfTheWeek;
DateTime editTime;
DateTime alarmTime;

float temperature;

const char dayNames[7][10] PROGMEM = {
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

// Warm-temperature icon (campfire), 21x20 pixels
static const unsigned char PROGMEM fire_bitmap[] = {
    0b00000000, 0b00101000, 0b00000000,  // Detailed flame tips
    0b00000000, 0b01111100, 0b00000000,
    0b00000001, 0b11010110, 0b00000000,  // Flickering flames
    0b00000011, 0b10111101, 0b10000000,  // with gaps
    0b00000110, 0b11111111, 0b01000000,
    0b00001111, 0b01111110, 0b11100000,  // Middle flames
    0b00011101, 0b11111111, 0b01110000,  // with texture
    0b00111110, 0b11111111, 0b11011000,
    0b01110111, 0b11111111, 0b11101100,  // Lower flames
    0b11111011, 0b11111111, 0b10111110,  // with variations
    0b11101111, 0b11111111, 0b11110110,
    0b11111101, 0b11111111, 0b01111110,  // Base of flames
    0b01110000, 0b11111111, 0b00001110,  // Air gaps above logs
    0b11111001, 0b10000001, 0b11011111,  // Top log with bark texture
    0b11100111, 0b11111111, 0b11111111,  // and knots
    0b00000000, 0b11111111, 0b00000000,  // Large gap between logs
    0b11111101, 0b10000001, 0b10111110,  // Middle log with texture
    0b11100111, 0b11000111, 0b11111111,  // and split pattern
    0b00000001, 0b11111111, 0b10000000,  // Bottom gap
    0b01111111, 0b10000001, 0b11111110   // Bottom log with texture
};

// Cold-temperature icon (snowflake), 20x20 pixels
static const unsigned char PROGMEM snow_bitmap[] = {
  0b00000001, 0b00000100, 0b00000000,
  0b00000000, 0b10001000, 0b00000000,
  0b00000000, 0b11111000, 0b00000000,
  0b00000000, 0b01110000, 0b00000000,
  0b00010001, 0b01110100, 0b01000000,
  0b00001001, 0b01110100, 0b10000000,
  0b00000111, 0b11111110, 0b00000000,
  0b00000011, 0b11111100, 0b00000000,
  0b00000001, 0b11111100, 0b00000000,
  0b00100000, 0b11111000, 0b10000000,
  0b00111111, 0b01110111, 0b11000000,
  0b00100000, 0b11111000, 0b10000000,
  0b00000001, 0b11111100, 0b00000000,
  0b00000011, 0b11111100, 0b00000000,
  0b00000111, 0b11111110, 0b00000000,
  0b00001001, 0b01110100, 0b10000000,
  0b00010001, 0b01110100, 0b01000000,
  0b00000000, 0b01110000, 0b00000000,
  0b00000000, 0b11111000, 0b00000000,
  0b00000000, 0b10001000, 0b00000000
};


void setup() {
  Serial.begin(9600);
  display.begin();
  display.setContrast(57);
  display.clearDisplay();

  pinMode(buzzerPin, OUTPUT);
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);
  pinMode(A1, INPUT);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  irrecv.enableIRIn();
}

void loop() {
  bool ir1 = false;
  bool ir2 = false;
  bool ir3 = false;
  bool irDown = false;
  bool irUp = false;

  // Read a pending IR key press (the raw code is logged to help map new remotes)
  if (irrecv.decode()) {
    Serial.print("IR code: 0x");
    Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
    if (irrecv.decodedIRData.decodedRawData == irKey1)
      ir1 = true;
    if (irrecv.decodedIRData.decodedRawData == irKey2)
      ir2 = true;
    if (irrecv.decodedIRData.decodedRawData == irKey3)
      ir3 = true;
    if (irrecv.decodedIRData.decodedRawData == irKeyUp)
      irUp = true;
    if (irrecv.decodedIRData.decodedRawData == irKeyDown)
      irDown = true;
    delay(100);
    irrecv.resume();
  }

  currentTime = rtc.now();
  dayOfTheWeek = currentTime.dayOfTheWeek();
  temperature = rtc.getTemperature();  // DS3231 on-chip sensor, in degrees C

  // Sample the humidity sensor (A1) every 5 seconds, scaled to 0-40
  if (currentTime.second() % 5 == 0) {
    sensorValue = analogRead(1);
    humidity = map(sensorValue, 0, 1023, 0, 40);
  }

  handleButtons(ir1, ir2, ir3, irUp, irDown);

  display.clearDisplay();
  if (!editingMode && !settingAlarm)
    displayDateTime();
  else if (editingMode)
    displayEditTime();
  else if (settingAlarm)
    displayAlarmTime();

  if (ringAlarm && currentTime >= alarmTime) {
    soundAlarm();  // Alarm time reached: sound the buzzer, then disarm
    ringAlarm = false;
  }

  display.display();
  delay(60);
}

/*
 * Button / remote handling.
 *   Normal view:  B1/IR 1 -> edit date/time, B2/IR 2 -> toggle 12/24h,
 *                 B3/IR 3 -> set (and arm) the alarm.
 *   Edit views:   B1/IR 1 -> next field (after seconds: save and exit),
 *                 B2/IR Up -> increment, B3/IR Down -> decrement.
 */
void handleButtons(int ir1, int ir2, int ir3, int irUp, int irDown) {
  // Button 1: enter editing mode or cycle through edit fields
  if ((digitalRead(button1Pin) == LOW || ir1) && !settingAlarm) {
    delay(70);  // Debounce delay
    if (!editingMode) {
      editingMode = true;
      editField = 1;           // Start with the year
      editTime = currentTime;
    } else {
      editField++;
      if (editField > 6) {     // Past the seconds field: save and exit
        editingMode = false;
        editField = 0;
        rtc.adjust(editTime);  // Write the new date/time to the RTC
      }
    }
  }

  // Button 2: increment the selected field
  if ((digitalRead(button2Pin) == LOW || irUp) && editingMode) {
    delay(70);
    if (editingMode) {
      incrementField();
      displayEditTime();
    }
  }

  if ((digitalRead(button2Pin) == LOW || irUp) && settingAlarm) {
    delay(70);
    if (settingAlarm) {
      incrementFieldAlarm();
      displayAlarmTime();
    }
  }

  // Button 2 in normal view: toggle 12/24-hour format
  if ((digitalRead(button2Pin) == LOW || ir2) && !editingMode && !settingAlarm) {
    delay(70);
    is24HourFormat = !is24HourFormat;
  }

  // Button 3: decrement the selected field
  if ((digitalRead(button3Pin) == LOW || irDown) && editingMode) {
    delay(70);
    if (editingMode) {
      decrementField();
      displayEditTime();
    }
  }

  if ((digitalRead(button3Pin) == LOW || irDown) && settingAlarm) {
    delay(70);
    if (settingAlarm) {
      decrementFieldAlarm();
      displayAlarmTime();
    }
  }

  // Button 3 in normal view: arm the alarm and start editing it (default: one minute from now)
  if ((digitalRead(button3Pin) == LOW || ir3) && !editingMode) {
    delay(70);  // Debounce delay
    if (!settingAlarm) {
      settingAlarm = true;
      editFieldAlarm = 1;  // Start with the year
      ringAlarm = true;
      alarmTime = DateTime(currentTime.year(), currentTime.month(), currentTime.day(), currentTime.hour(), currentTime.minute() + 1, currentTime.second());
    }
  }

  // Button 1 while setting the alarm: next field (after seconds: exit)
  if ((digitalRead(button1Pin) == LOW || ir1) && settingAlarm) {
    delay(70);  // Debounce delay

    editFieldAlarm++;
    if (editFieldAlarm > 6) {
      settingAlarm = false;
      editFieldAlarm = 0;
    }
  }
}

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Number of days in the given month (1-12) of the given year
int daysInMonth(int month, int year) {
  if (month == 2) {
    return isLeapYear(year) ? 29 : 28;
  }
  else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;
  }
  return 31;
}

void displayDayOfWeek(int dayOfTheWeek) {
  char buffer[10];
  strcpy_P(buffer, dayNames[dayOfTheWeek]);
  display.println(buffer);
}

// Increment the selected field of the time being edited, wrapping around
void incrementField() {
  switch (editField) {
    case 1: editTime = DateTime(editTime.year() + 1, editTime.month(), editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    case 2: editTime = DateTime(editTime.year(), (editTime.month() % 12) + 1, editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    case 3: {
      int day = editTime.day();
      int maxDays = daysInMonth(editTime.month(), editTime.year());
      day = (day % maxDays) + 1;
      editTime = DateTime(editTime.year(), editTime.month(), day, editTime.hour(), editTime.minute(), editTime.second());
      break;
    }
    case 4: editTime = DateTime(editTime.year(), editTime.month(), editTime.day(), (editTime.hour() + 1) % 24, editTime.minute(), editTime.second()); break;
    case 5: editTime = DateTime(editTime.year(), editTime.month(), editTime.day(), editTime.hour(), (editTime.minute() + 1) % 60, editTime.second()); break;
    case 6: editTime = DateTime(editTime.year(), editTime.month(), editTime.day(), editTime.hour(), editTime.minute(), (editTime.second() + 1) % 60); break;
  }
}

// Decrement the selected field of the time being edited, wrapping around
void decrementField() {
  switch (editField) {
    case 1: editTime = DateTime(editTime.year() - 1, editTime.month(), editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    case 2: editTime = DateTime(editTime.year(), (editTime.month() == 1 ? 12 : editTime.month() - 1), editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    case 3: {
      int day = (editTime.day() == 1) ? daysInMonth(editTime.month(), editTime.year()) : editTime.day() - 1;
      editTime = DateTime(editTime.year(), editTime.month(), day, editTime.hour(), editTime.minute(), editTime.second());
      break;
    }
    case 4: editTime = DateTime(editTime.year(), editTime.month(), editTime.day(), (editTime.hour() == 0 ? 23 : editTime.hour() - 1), editTime.minute(), editTime.second()); break;
    case 5: editTime = DateTime(editTime.year(), editTime.month(), editTime.day(), editTime.hour(), (editTime.minute() == 0 ? 59 : editTime.minute() - 1), editTime.second()); break;
    case 6: editTime = DateTime(editTime.year(), editTime.month(), editTime.day(), editTime.hour(), editTime.minute(), (editTime.second() == 0 ? 59 : editTime.second() - 1)); break;
  }
}

// Same as incrementField(), applied to the alarm time
void incrementFieldAlarm() {
  switch (editFieldAlarm) {
    case 1: alarmTime = DateTime(alarmTime.year() + 1, alarmTime.month(), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    case 2: alarmTime = DateTime(alarmTime.year(), (alarmTime.month() % 12) + 1, alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    case 3: {
      int day = alarmTime.day();
      int maxDays = daysInMonth(alarmTime.month(), alarmTime.year());
      day = (day % maxDays) + 1;
      alarmTime = DateTime(alarmTime.year(), alarmTime.month(), day, alarmTime.hour(), alarmTime.minute(), alarmTime.second());
      break;
    }
    case 4: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), alarmTime.day(), (alarmTime.hour() + 1) % 24, alarmTime.minute(), alarmTime.second()); break;
    case 5: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), alarmTime.day(), alarmTime.hour(), (alarmTime.minute() + 1) % 60, alarmTime.second()); break;
    case 6: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), (alarmTime.second() + 1) % 60); break;
  }
}

// Same as decrementField(), applied to the alarm time
void decrementFieldAlarm() {
  switch (editFieldAlarm) {
    case 1: alarmTime = DateTime(alarmTime.year() - 1, alarmTime.month(), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    case 2: alarmTime = DateTime(alarmTime.year(), (alarmTime.month() == 1 ? 12 : alarmTime.month() - 1), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    case 3: {
      int day = (alarmTime.day() == 1) ? daysInMonth(alarmTime.month(), alarmTime.year()) : alarmTime.day() - 1;
      alarmTime = DateTime(alarmTime.year(), alarmTime.month(), day, alarmTime.hour(), alarmTime.minute(), alarmTime.second());
      break;
    }
    case 4: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), alarmTime.day(), (alarmTime.hour() == 0 ? 23 : alarmTime.hour() - 1), alarmTime.minute(), alarmTime.second()); break;
    case 5: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), alarmTime.day(), alarmTime.hour(), (alarmTime.minute() == 0 ? 59 : alarmTime.minute() - 1), alarmTime.second()); break;
    case 6: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), (alarmTime.second() == 0 ? 59 : alarmTime.second() - 1)); break;
  }
}

// Main screen: date, large time, day of the week, temperature and weather icon
void displayDateTime() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(1, 1);
  display.print(currentTime.year(), DEC);
  display.print("/");
  printDigits(currentTime.month());
  display.print('/');
  printDigits(currentTime.day());
  display.println();

  display.setTextSize(2);

  int displayHour = currentTime.hour();
  if (!is24HourFormat) {
    displayHour = displayHour % 12;
    displayHour = displayHour ? displayHour : 12;
  }
  printDigits(displayHour);
  display.print(':');
  printDigits(currentTime.minute());

  display.setTextSize(1);
  display.print(':');
  printDigits(currentTime.second());
  display.println();
  if (!is24HourFormat) {
    display.setCursor(60, 17);
    display.print(currentTime.hour() < 12 ? " AM" : " PM");
  }
  display.println();

  displayDayOfWeek(dayOfTheWeek);

  display.print(temperature, 1);
  display.print((char)247);  // Degree symbol
  display.println("C");

  drawTemperatureIcon(temperature, 60, 25);

  display.display();

  delay(50);
}

// Date/time editing screen
void displayEditTime() {
  display.clearDisplay();

  display.setCursor(0, 1);
  display.print("Editing: ");
  switch (editField) {
    case 1: display.print("Year"); break;
    case 2: display.print("Month"); break;
    case 3: display.print("Day"); break;
    case 4: display.print("Hour"); break;
    case 5: display.print("Min"); break;
    case 6: display.print("Sec"); break;
  }

  display.setTextSize(1);
  display.println();
  display.print("D: ");
  display.print(editTime.year());
  display.print("/");
  display.print(editTime.month());
  display.print("/");
  display.print(editTime.day());

  display.println();
  display.print("T: ");
  printDigits(editTime.hour());
  display.print(":");
  printDigits(editTime.minute());
  display.print(":");
  printDigits(editTime.second());
}

// Alarm editing screen
void displayAlarmTime() {
  display.clearDisplay();

  display.setCursor(0, 1);
  display.print("Alarm: ");
  switch (editFieldAlarm) {
    case 1: display.print("Year"); break;
    case 2: display.print("Month"); break;
    case 3: display.print("Day"); break;
    case 4: display.print("Hour"); break;
    case 5: display.print("Min"); break;
    case 6: display.print("Sec"); break;
  }

  display.setTextSize(1);
  display.println();
  display.print("D: ");
  display.print(alarmTime.year());
  display.print("/");
  display.print(alarmTime.month());
  display.print("/");
  display.print(alarmTime.day());

  display.println();
  display.print("T: ");
  printDigits(alarmTime.hour());
  display.print(":");
  printDigits(alarmTime.minute());
  display.print(":");
  printDigits(alarmTime.second());
}

// Print a number with a leading zero if it has a single digit
void printDigits(int digits) {
  if (digits < 10) {
    display.print("0");
  }
  display.print(digits, DEC);
}

// Bit-banged square wave on the buzzer: half-period in microseconds, duration in milliseconds
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(buzzerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(buzzerPin, LOW);
    delayMicroseconds(tone);
  }
}

// Alarm sound: 10 beeps of 200 ms separated by 200 ms pauses
void soundAlarm() {
  for (int i = 0; i < 10; i++)
  {
    playTone(2000, 200);
    delay(200);
  }
}

// Campfire icon at 15 degrees C and above, snowflake below
void drawTemperatureIcon(float temp, int x, int y) {
  if (temp >= 15) {
    display.drawBitmap(x, y, fire_bitmap, 21, 20, BLACK);
  } else {
    display.drawBitmap(x, y, snow_bitmap, 20, 20, BLACK);
  }
}
