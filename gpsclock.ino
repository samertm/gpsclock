#include <LiquidCrystal.h>
#include <TimeLib.h>
#include "TinyGPS++.h"
#include "SoftwareSerial.h"

// GPS from left to right:
//  PPS, RXD, TXD, GND, VCC

// GPS: TXD=2, RXD=3
SoftwareSerial serial_connection(2, 3);
TinyGPSPlus gps;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

static bool kDebug = false;

static const int kTimeNotSet = 0;
static const int kTimeNeedsSync = 1;
static const int kTimeDateInvalid = 2;
static const int kTimeValid = 3;

static const unsigned long kSyncIntervalMS = 2 * 60 * 1000UL;

static int displayTimeStatus = kTimeNotSet;

static unsigned long prevMillis = 0;

void helloWorldLCD() {
  lcd.begin(16, 2);
  lcd.print("Starting...");
}

void printPaddedTimeVal(int timeVal) {
  if (timeVal < 10) {
    lcd.print("0");
    lcd.print(timeVal);
  } else {
    lcd.print(timeVal);
  }
}

// Sets the clock and returns the display time status.
int syncGPSTime() {
  tmElements_t tm;
  if (gps.time.value() == 0 || gps.time.age() > 2000) {
    if (kDebug) {
      Serial.print("\nBAD gps.time.value or gps.time.age: ");
      Serial.println(gps.time.value());
      Serial.println(gps.time.age());
    }
    return kTimeNeedsSync;
  }

  int status = kTimeValid;
  int set_year;
  int set_month;
  int set_day;
  if (gps.date.year() > 2000) {
    set_year = gps.date.year();
    set_month = gps.date.month();
    set_day = gps.date.day();
  } else {
    // Date from gps is incorrect, use whatever the current date is
    // (could be the epoch if it's never been set).
    time_t t = now();
    set_year = year(t);
    set_month = month(t);
    set_day = day(t);

    status = kTimeDateInvalid;
  }

  setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
          set_day, set_month, set_year);
  return status;
}

time_t prevDisplay = 0;

void displayTime() {
  if (displayTimeStatus == kTimeNotSet) {
    return;
  }

  time_t t = now();
  if (t != prevDisplay) {
    prevDisplay = t;

    // Print the time.
    lcd.setCursor(0, 0);
    printPaddedTimeVal(hour(t));
    lcd.print(":");
    printPaddedTimeVal(minute(t));
    lcd.print(":");
    printPaddedTimeVal(second(t));
    lcd.print(" UTC");

    lcd.setCursor(0, 1);
    if (displayTimeStatus == kTimeDateInvalid) {
      // Remove the printed date.
      lcd.print("          ");
    } else {
      // Print the date.
      printPaddedTimeVal(day(t));
      lcd.print("-");
      printPaddedTimeVal(month(t));
      lcd.print("-");
      lcd.print(year(t));
    }
  }

  lcd.setCursor(14, 1);
  if (displayTimeStatus == kTimeNeedsSync) {
    lcd.print("N");
  } else {
    lcd.print(" ");
  }

}

// Sets the time and updates displayTimeStatus.
void setTime() {
  if (!gps.time.isUpdated() || !gps.date.isUpdated()) {
    return;
  }

  if (displayTimeStatus == kTimeValid &&
      (millis() - prevMillis) < kSyncIntervalMS) {
    return;
  }


  Serial.println("Syncing GPS time.");
  displayTimeStatus = syncGPSTime();
  if (displayTimeStatus == kTimeNeedsSync) {
    return;
  }

  // status == kTimeValid || kTimeDateInvalid
  prevMillis = millis();
}

void debugTime() {
  Serial.print("\n");
  Serial.print(gps.time.hour());
  Serial.print(gps.time.minute());
  Serial.print(gps.time.second());
  Serial.print("\n");
}

void setup() {
  // put your setup code here, to run once:
  helloWorldLCD();


  Serial.begin(9600);
  serial_connection.begin(9600);
  Serial.println("GPS Start");
}

void loop() {
  // Premature optimization: have two versions of this loop, one for
  // debug and one for prod, so there isn't an `if` statement in the
  // middle of a hot loop.
  if (true || kDebug) {
    while(serial_connection.available()) {
      char c = serial_connection.read();
      Serial.write(c);
      gps.encode(c);
    }
  } else {
    while(serial_connection.available()) {
      gps.encode(serial_connection.read());
    }
  }

  setTime();
  displayTime();
}
