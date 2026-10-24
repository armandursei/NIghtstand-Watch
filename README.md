# Nightstand Watch

An Arduino bedside clock with a Nokia 5110 graphic display, a battery-backed DS3231 real-time clock, a buzzer alarm and room temperature readout. It can be controlled with three push buttons or wirelessly with an IR remote.

## Features

- Date and time on an 84x48 graphic LCD, with large digits for hours and minutes
- 12/24-hour format toggle (AM/PM indicator in 12-hour mode)
- Day of the week
- Room temperature from the DS3231's on-chip sensor, with a campfire icon at 15 °C and above and a snowflake icon below
- On-device date/time setting, saved to the RTC
- Buzzer alarm with a full date/time setting (year to seconds)
- Every action available from both the buttons and an NEC IR remote

## Hardware

| Component | Notes |
|---|---|
| Arduino Uno or compatible board | |
| Nokia 5110 LCD (PCD8544) | 84x48 px, driven over software SPI |
| DS3231 RTC module | I2C; also provides the temperature reading |
| IR receiver + NEC remote | Keys 1, 2, 3, Up, Down are used |
| Passive buzzer | Driven with a bit-banged square wave |
| 3 push buttons | Wired to GND, internal pull-ups enabled |
| Analog humidity sensor (optional) | Sampled on A1, not shown on the display yet |

### Pin mapping

| Arduino pin | Connected to |
|---|---|
| D2 | Buzzer |
| D3 | LCD RST |
| D4 | LCD CE (CS) |
| D5 | LCD DC |
| D6 | LCD DIN |
| D7 | LCD CLK |
| D8 | Button 1 |
| D9 | Button 2 |
| D10 | Button 3 |
| D11 | IR receiver output |
| A1 | Humidity sensor output |
| SDA / SCL | DS3231 |

## How it works

The main loop reads the time and temperature from the DS3231, checks the buttons and the IR receiver, redraws the screen and fires the alarm when the alarm time is reached.

The clock has three screens:

- **Normal view**: date (`YYYY/MM/DD`), time, day of the week, temperature and icon.
- **Time editing**: lets you change year, month, day, hour, minute and second one field at a time. The day wraps around according to the month length (leap years included). Leaving the last field writes the new time to the RTC.
- **Alarm setting**: same field-by-field editor for the alarm. Opening it arms the alarm, set by default to one minute from now. When the alarm time is reached the buzzer beeps 10 times and the alarm disarms.

### Controls

| Screen | Button 1 / IR `1` | Button 2 | Button 3 |
|---|---|---|---|
| Normal view | Start editing the time | Toggle 12/24 h (IR `2`) | Set and arm the alarm (IR `3`) |
| Time editing | Next field (save after seconds) | Increment (IR `Up`) | Decrement (IR `Down`) |
| Alarm setting | Next field (exit after seconds) | Increment (IR `Up`) | Decrement (IR `Down`) |

Every received IR code is printed on the serial monitor (9600 baud), which makes it easy to adapt the key codes at the top of the sketch to a different remote.

## Required libraries

Install these from the Arduino IDE Library Manager (`SPI` comes with the board core):

| `#include` | Library Manager name |
|---|---|
| `Adafruit_GFX.h` | Adafruit GFX Library |
| `Adafruit_PCD8544.h` | Adafruit PCD8544 Nokia 5110 LCD library |
| `RTClib.h` | RTClib (Adafruit) |
| `IRremote.h` | IRremote (version 3.x or newer, uses the `decodedIRData` API) |

## Getting started

### Arduino IDE

1. Open `NightstandWatch/NightstandWatch.ino`.
2. Install the libraries listed above (Tools > Manage Libraries).
3. Select your board and port, then click Upload.
4. Set the current date and time with button 1 (or IR key `1`).

### arduino-cli

```bash
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit GFX Library" "Adafruit PCD8544 Nokia 5110 LCD library" "RTClib" "IRremote"
# Example for an Arduino Uno; adjust the FQBN and port for your board
arduino-cli compile --fqbn arduino:avr:uno NightstandWatch
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno NightstandWatch
```

## Project structure

```
.
├── NightstandWatch/
│   └── NightstandWatch.ino   # Complete sketch
└── README.md
```

## Author

George-Armand Ursei - [GitHub](https://github.com/armandursei)
