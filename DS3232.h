/*
 * DS3232.h - library for DS3232 RTC from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time
 * Based on code by Jonathan Oxer at http://www.freetronics.com/pages/rtc-real-time-clock-module-quickstart-guide
 * Based on code by Michael Margolis at http://code.google.com/p/arduino-time
 * Based on code by Ryhs Weather at http://github.com/rweather/arduinolibs/tree/master/libraries/RTC

  Copyright (c) 2013 by the Arduino community (Author anonymous)

  This work is licensed under the Creative Commons ShareAlike 1.0 Generic (CC SA 1.0) License
  To view a copy of this license, visit http://creativecommons.org/licenses/sa/1.0 or send a
  letter to Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DS3232_h
#define DS3232_h

#include <DSRTC.h>
#include <Wire.h>

// Based on page 11 of specs; http://www.maxim-ic.com/datasheet/index.mvp/id/4984
#define DS3232_I2C_ADDRESS 0x68

// Helpers
#define temperatureCToF(C) (C * 9 / 5 + 32)
#define temperatureFToC(F) ((F - 32) * 5 / 9)

/**
 * DS3232 Class
 */
class DS3232: public DSRTC
{
  public:
    DS3232();
    bool available();
  protected:
    uint8_t read1(uint8_t addr);
    void write1(uint8_t addr, uint8_t data);
    void readN(uint8_t addr, uint8_t buf[], uint8_t len);
    void writeN(uint8_t addr, uint8_t buf[], uint8_t len);
};

#endif
