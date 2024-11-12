#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <RTClib.h>
#include <IRremote.h>

// LCD, RTC, and button definitions
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);
RTC_DS3231 rtc;

// int minValue = 200; // Valoarea citită într-un mediu foarte uscat (exemplu)
// int maxValue = 800;// Valoarea citită într-un mediu foarte umed (exemplu)


const int buzzer = 2;       // Buzzer pin
const int button1Pin = 8;   // Button 1 for activating/cycling through editing fields
const int button2Pin = 9;   // Button 2 to increment the current field
const int button3Pin = 10;  // Button 3 to decrement the current field


int sensorValue = 0;      // Variabila pentru a stoca valoarea citită de pe senzor
int humidity;

IRrecv irrecv(11);
decode_results results;

bool editingMode = false;
bool settingAlarm = false;
bool ringAlarm = false;
int editField = 0;  // Tracks which field is currently being edited
int editFieldAlarm = 0;
bool is24HourFormat = true;



DateTime currentTime;
int dayOfTheWeek;
DateTime editTime;
DateTime alarmTime;

float temperature;


const char dayNames[7][10] PROGMEM = {
  "Duminica", "Luni", "Marti", "Miercuri", "Joi", "Vineri", "Sambata"
};

// Custom bitmap for sun icon (9x9 pixels)
/*static const unsigned char PROGMEM sun_bitmap[] = {
  0b00000000, 0b01111100, 0b00000000,
  0b00000001, 0b11111111, 0b00000000,
  0b00000111, 0b11111111, 0b10000000,
  0b00011111, 0b11111111, 0b11000000,
  0b00111111, 0b11111111, 0b11100000,
  0b00111111, 0b11111111, 0b11100000,
  0b01111111, 0b11111111, 0b11110000,
  0b01111111, 0b11111111, 0b11110000,
  0b11111111, 0b11111111, 0b11111000,
  0b11111111, 0b11111111, 0b11111000,
  0b11111111, 0b11111111, 0b11111000,
  0b11111111, 0b11111111, 0b11111000,
  0b01111111, 0b11111111, 0b11110000,
  0b01111111, 0b11111111, 0b11110000,
  0b00111111, 0b11111111, 0b11100000,
  0b00111111, 0b11111111, 0b11100000,
  0b00011111, 0b11111111, 0b11000000,
  0b00000111, 0b11111111, 0b10000000,
  0b00000001, 0b11111111, 0b00000000,
  0b00000000, 0b01111100, 0b00000000
};*/

static const unsigned char PROGMEM sun_bitmap[] = {
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


// Custom bitmap for snowflake icon (8x8 pixels)
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

  //rtc.adjust(DateTime(2024, 11, 10, 15, 30, 0));


  pinMode(buzzer, OUTPUT);
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


  if (irrecv.decode()) {
    Serial.print("Codul butonului: 0x");
    Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
    if (irrecv.decodedIRData.decodedRawData == 0xBA45FF00)
      ir1 = true;
    if (irrecv.decodedIRData.decodedRawData == 0xB946FF00)
      ir2 = true;
    if (irrecv.decodedIRData.decodedRawData == 0xB847FF00)
      ir3 = true;
    if (irrecv.decodedIRData.decodedRawData == 0xE718FF00)
      irUp = true;
    if (irrecv.decodedIRData.decodedRawData == 0xAD52FF00)
      irDown = true;
    delay(100);
    irrecv.resume();
  }

  currentTime = rtc.now();

  dayOfTheWeek = currentTime.dayOfTheWeek();
  //Serial.println(dayOfTheWeek);


  temperature = rtc.getTemperature();

  if(currentTime.second()%5==0){
  //sensorValue = digitalRead(13);
  sensorValue = analogRead(1);
  humidity = map(sensorValue, 0, 1023, 0, 40);
  //humidity = constrain(humidity, 0, 100); // Constrânge între 0 și 100
  }

  handleButtons(ir1, ir2, ir3,irUp, irDown);

  display.clearDisplay();
  if (!editingMode && !settingAlarm)
    displayDateTime();  // Display the date and time, or the editing interface
  else if (editingMode)
    displayEditTime();
  else if (settingAlarm)
    displayAlarmTime();

  if (ringAlarm && currentTime >= alarmTime) {
    buz();  // Sound the buzzer if alarm time has been reached
    ringAlarm = false;
  }

  display.display();
  delay(60);
}

void handleButtons(int ir1, int ir2, int ir3, int irUp, int irDown) {
  // Button 1: Enter editing mode or cycle through edit fields
  if ((digitalRead(button1Pin) == LOW || ir1) && !settingAlarm) {
    delay(70);  // Debounce delay
    if (!editingMode) {
      editingMode = true;  // Enter editing mode
      editField = 1;
      editTime = currentTime;  // Start with the year
    } else {
      editField++;
      if (editField > 6) {  // Cycle through fields, reset after seconds
        editingMode = false;
        editField = 0;
        rtc.adjust(editTime);  // Update RTC with new date/time
      }
    }
  }

  // Button 2: Increment the selected field
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

  if ((digitalRead(button2Pin) == LOW || ir2) && !editingMode && !settingAlarm) {
    delay(70);
    is24HourFormat = !is24HourFormat;
  }

  // Button 3: Decrement the selected field
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

  if ((digitalRead(button3Pin) == LOW || ir3) && !editingMode) {
    delay(70);  // Debounce delay
    if (!settingAlarm) {
      settingAlarm = true;  // Enter editing mode
      editFieldAlarm = 1;
      ringAlarm = true;
      alarmTime = DateTime(currentTime.year(), currentTime.month(), currentTime.day(), currentTime.hour(), currentTime.minute() + 1, currentTime.second());  // Start with the year
    }
  }

  if ((digitalRead(button1Pin) == LOW || ir1) && settingAlarm) {
    delay(70);  // Debounce delay

    editFieldAlarm++;
    if (editFieldAlarm > 6) {  // Cycle through fields, reset after seconds
      settingAlarm = false;
      editFieldAlarm = 0;
    }
  }
}

// Funcție pentru a verifica dacă un an este bisect
bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Funcție pentru a obține numărul de zile dintr-o lună specifică
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

void incrementField() {
  switch (editField) {
    case 1: editTime = DateTime(editTime.year() + 1, editTime.month(), editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    case 2: editTime = DateTime(editTime.year(), (editTime.month() % 12) + 1, editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    //case 3: editTime = DateTime(editTime.year(), editTime.month(), (editTime.day() % 31) + 1, editTime.hour(), editTime.minute(), editTime.second()); break;
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

void decrementField() {
  switch (editField) {
    case 1: editTime = DateTime(editTime.year() - 1, editTime.month(), editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    case 2: editTime = DateTime(editTime.year(), (editTime.month() == 1 ? 12 : editTime.month() - 1), editTime.day(), editTime.hour(), editTime.minute(), editTime.second()); break;
    //case 3: editTime = DateTime(editTime.year(), editTime.month(), (editTime.day() == 1 ? 31 : editTime.day() - 1), editTime.hour(), editTime.minute(), editTime.second()); break;
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


void incrementFieldAlarm() {
  switch (editFieldAlarm) {
    case 1: alarmTime = DateTime(alarmTime.year() + 1, alarmTime.month(), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    case 2: alarmTime = DateTime(alarmTime.year(), (alarmTime.month() % 12) + 1, alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    //case 3: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), (alarmTime.day() % 31) + 1, alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
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

void decrementFieldAlarm() {
  switch (editFieldAlarm) {
    case 1: alarmTime = DateTime(alarmTime.year() - 1, alarmTime.month(), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    case 2: alarmTime = DateTime(alarmTime.year(), (alarmTime.month() == 1 ? 12 : alarmTime.month() - 1), alarmTime.day(), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
    //case 3: alarmTime = DateTime(alarmTime.year(), alarmTime.month(), (alarmTime.day() == 1 ? 31 : alarmTime.day() - 1), alarmTime.hour(), alarmTime.minute(), alarmTime.second()); break;
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

void displayDateTime() {
  display.clearDisplay();


  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(1, 1);
  //delay(50);
  display.print(currentTime.year(), DEC);
  //delay(50);
  display.print("/");
  //delay(50);
  printDigits(currentTime.month());
  //delay(50);
  display.print('/');
  //delay(50);
  printDigits(currentTime.day());
  //delay(50);
  display.println();

  //Serial.println(dayOfTheWeek);
  
  //displayDayOfWeek(dayOfTheWeek);

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


  

  //display.print("Humid: "+sensorValue);
  //Serial.print("Valoare senzor: ");
  //Serial.println(sensorValue);
  //Serial.println(humidity);


  // Draw temperature icon
  //drawTemperatureIcon(temperature, 40, 24);

  // display.setCursor(60, 23);
  drawTemperatureIcon(temperature, 60, 25);

  display.display();

  delay(50);
}

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

void printDigits(int digits) {
  if (digits < 10) {
    display.print("0");
  }
  display.print(digits, DEC);
}

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(buzzer, HIGH);
    delayMicroseconds(tone);
    digitalWrite(buzzer, LOW);
    delayMicroseconds(tone);
  }
}

void buz() {
  for (int i = 0; i < 10; i++) 
  {
    //tone(buzzer, 2000); // 2kHz tone
    //noTone(buzzer);     // Stop tone
    //delay(100);
    playTone(2000, 200);
    delay(200);        // for 1 second
  }  // 1-second pause
}

void drawTemperatureIcon(float temp, int x, int y) {
  if (temp >= 15) {
    display.drawBitmap(x, y, sun_bitmap, 21, 20, BLACK);
  } else {
    display.drawBitmap(x, y, snow_bitmap, 20, 20, BLACK);
  }
}
