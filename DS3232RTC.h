#ifndef DS3232RTC_h
#define DS3232RTC_h

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Time.h>
#include <DSRTC.h>

// Based on page 11 of specs; http://www.maxim-ic.com/datasheet/index.mvp/id/4984
#define DS3232_I2C_ADDRESS 0x68

// Helpers
#define temperatureCToF(C) (C * 9 / 5 + 32)
#define temperatureFToC(F) ((F - 32) * 5 / 9)

/**
 * DS3232RTC Class
 */
class DS3232RTC: public DSRTC
{
  public:
    DS3232RTC();
  protected:
    uint8_t read1(uint8_t addr);
    void write1(uint8_t addr, uint8_t data);
    void readN(uint8_t addr, uint8_t buf[], uint8_t len);
    void writeN(uint8_t addr, uint8_t buf[], uint8_t len);
};

/**
 * DS3232SRAM Class
 */
class DS3232SRAM : public Stream
{
  public:
    DS3232SRAM();
    // more like EEPROMClass
    static uint8_t read(int addr);
    static void write(int addr, uint8_t data);

    // from Print class
    #if ARDUINO >= 100
    virtual size_t write(uint8_t data);
    virtual size_t write(const char *str);
    virtual size_t write(const uint8_t *buf, size_t size);
    #else
    virtual void write(uint8_t);
    virtual void write(const char *str);
    virtual void write(const uint8_t *buf, size_t size);
    #endif

    // from Stream class
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();  // Sets the next character position to 0.
    uint8_t seek(uint8_t pos);  // Sets the position of the next character to be extracted from the stream.
    uint8_t tell();  // Returns the position of the current character in the stream.

  private:
    bool _init;
    bool _avail;
    uint8_t _cursor;
};

extern DS3232SRAM SRAM;

#endif
