/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
Modified by David Payne for use in the Scrolling Marquee
Modified to use NTP with POSIX timezone for automatic DST handling
*/

#include "TimeClient.h"
#include <coredecls.h>

static void logTimezoneAndLocalTime(const String& tz) {
  time_t now = time(nullptr);
  struct tm* localInfo = localtime(&now);
  char timeBuffer[32] = {0};

  if (localInfo != nullptr) {
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localInfo);
  } else {
    strncpy(timeBuffer, "localtime unavailable", sizeof(timeBuffer) - 1);
  }

  Serial.print("TZ applied: ");
  Serial.println(tz);
  Serial.print("Local time now: ");
  Serial.println(timeBuffer);
}

TimeClient::TimeClient(String timeZone) {
  myTimeZone = timeZone;
  // Apply POSIX timezone immediately in constructor
  setenv("TZ", myTimeZone.c_str(), 1);
  tzset();
}

void TimeClient::updateTime() {
  // Set POSIX timezone environment variable for automatic DST
  setenv("TZ", myTimeZone.c_str(), 1);
  tzset();

  // Start NTP sync with POSIX TZ so localtime() uses the selected timezone.
  configTime(myTimeZone.c_str(), "pool.ntp.org", "time.nist.gov");

  if (!timeSynced) {
    Serial.print("Waiting for NTP sync");
    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 100000 && attempts < 20) {
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      attempts++;
    }
    Serial.println();

    if (now > 100000) {
      timeSynced = true;
      Serial.println("NTP time synced successfully");
      logTimezoneAndLocalTime(myTimeZone);
    } else {
      Serial.println("NTP sync timeout - will retry later");
    }
  }
}

void TimeClient::setTimezone(String timeZone) {
  myTimeZone = timeZone;
  setenv("TZ", myTimeZone.c_str(), 1);
  tzset();
  logTimezoneAndLocalTime(myTimeZone);
  timeSynced = false;  // Force re-sync on next updateTime() call
}

String TimeClient::getHours() {
  if (!timeSynced) {
    return "--";
  }
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  int hours = timeinfo->tm_hour;
  if (hours < 10) {
    return "0" + String(hours);
  }
  return String(hours);
}

String TimeClient::getMinutes() {
  if (!timeSynced) {
    return "--";
  }
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  int minutes = timeinfo->tm_min;
  if (minutes < 10) {
    return "0" + String(minutes);
  }
  return String(minutes);
}

String TimeClient::getSeconds() {
  if (!timeSynced) {
    return "--";
  }
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  int seconds = timeinfo->tm_sec;
  if (seconds < 10) {
    return "0" + String(seconds);
  }
  return String(seconds);
}

String TimeClient::getAmPmHours() {
  int hours = getHours().toInt();
  if (hours >= 13) {
    hours = hours - 12;
  }
  if (hours == 0) {
    hours = 12;
  }
  return String(hours);
}

String TimeClient::getAmPm() {
  int hours = getHours().toInt();
  String ampmValue = "AM";
  if (hours >= 12) {
    ampmValue = "PM";
  }
  return ampmValue;
}

String TimeClient::getFormattedTime() {
  return getHours() + ":" + getMinutes() + ":" + getSeconds();
}

String TimeClient::getAmPmFormattedTime() {
  return getAmPmHours() + ":" + getMinutes() + " " + getAmPm();
}

long TimeClient::getCurrentEpoch() {
  return (long)time(nullptr);
}

long TimeClient::getCurrentEpochWithUtcOffset() {
  // With POSIX timezone, localtime() already handles the offset and DST
  // This method is kept for backward compatibility
  return getCurrentEpoch();
}
