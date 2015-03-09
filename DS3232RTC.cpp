/*
 * DS3232RTC.h - library for DS3232 RTC module from Freetronics; http://www.freetronics.com/rtc
 * This library is intended to be used with Arduino Time.h library functions; http://playground.arduino.cc/Code/Time

 (See DS3232RTC.h for notes & license)
 */

#include <Stdint.h>
#include <Wire.h>
#include <Stream.h>
#include <DS3232RTC.h>

/* +----------------------------------------------------------------------+ */
/* | DS3232RTC Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 *
 */
DS3232RTC::DS3232RTC() {
}

/**
 *
 */
uint8_t DS3232RTC::read1(uint8_t addr) {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(addr);
  Wire.endTransmission();

  Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  } else {
    return 0xFF;
  }
}

/**
 *
 */
void DS3232RTC::write1(uint8_t addr, uint8_t data){
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
}

void DS3232RTC::readN(uint8_t addr, uint8_t buf[], uint8_t len)
{
	uint8_t i;

	Wire.beginTransmission(DS3232_I2C_ADDRESS);
	Wire.write(addr);
	for (i=0; i<len; i++)
	{
		buf[i] = Wire.read();
	}
	Wire.endTransmission();
}

void DS3232RTC::writeN(uint8_t addr, uint8_t buf[], uint8_t len)
{
	uint8_t i;

	Wire.beginTransmission(DS3232_I2C_ADDRESS);
	Wire.write(addr);
	for (i=0; i<len; i++)
	{
		Wire.write(buf[i]);
	}
	Wire.endTransmission();
}


/* +----------------------------------------------------------------------+ */
/* | DS3232SRAM Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 * \brief Attaches to the DS3232 RTC module on the I2C Wire
 */
DS3232SRAM::DS3232SRAM()
  : _init(false)
  , _avail(false)
  , _cursor(0)
{
  Wire.begin();
}

/**
 *
 */
uint8_t DS3232SRAM::read(int addr) {
  if (addr > 0xEC) return 0x00;
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x14 + addr); 
  Wire.endTransmission();  
  Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  } else {
    return 0x00;
  }
}

void DS3232SRAM::write(int addr, uint8_t data) {
  if (addr > 0xEC) return;
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x14 + addr); 
  Wire.write(data);
  Wire.endTransmission();  
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(uint8_t data) {
#else
void DS3232SRAM::write(uint8_t) {
#endif
  if (available() > 0) {
    write(_cursor, data);
    _cursor++;
    #if ARDUINO >= 100
    return 1;
  } else {
    return 0;
    #endif
  }
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(const char *str) {
#else
void DS3232SRAM::write(const char *str) {
#endif
  if (available() > 0) {
    size_t i = 0;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    while (*str && ((available() + i) > 0))
      i += Wire.write(*str++);
    Wire.endTransmission();
    _cursor += i;
    #if ARDUINO >= 100
    return i;
  } else {
    return 0;
    #endif
  }
}

/**
 *
 */
#if ARDUINO >= 100
size_t DS3232SRAM::write(const uint8_t *buf, size_t size) {
#else
void DS3232SRAM::write(const uint8_t *buf, size_t size) {
#endif
  if (available() > 0) {
    size_t i = 0;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    while (size-- && ((available() + i) > 0))
      i += Wire.write(*buf++);
    Wire.endTransmission();
    _cursor += i;
    #if ARDUINO >= 100
    return i;
  } else {
    return 0;
    #endif
  }
}

/**
 *
 */
int DS3232SRAM::available() {
  if (!_init) {
    _init = true;
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x05);  // sends 05h - month register
    Wire.endTransmission();
    Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
    if (Wire.available()) {
      Wire.read();
      _avail = true;
    } else {
      _avail = false;
    }
  }

  if (_avail) {
    return 0xEC - _cursor;  // How many bytes left
  } else {
    return -1;
  }
}

/**
 * \brief Read a single byte from SRAM
 */
int DS3232SRAM::read() {
  int res = peek();
  if (res != -1) _cursor++;
  return res;
}

/**
 *
 */
int DS3232SRAM::peek() {
  if (available() > 0) {
    Wire.beginTransmission(DS3232_I2C_ADDRESS);
    Wire.write(0x14 + _cursor); 
    Wire.endTransmission();  
    Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
    if (Wire.available()) {
      return Wire.read();
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

/**
 * \brief Reset the cursor to 0
 */
void DS3232SRAM::flush() {
  _cursor = 0;
}

/**
 *
 */
uint8_t DS3232SRAM::seek(uint8_t pos) {
  if (pos <= 0xEB)
    _cursor = pos;
  return _cursor;
}

/**
 *
 */
uint8_t DS3232SRAM::tell() {
  return _cursor;
}

DS3232SRAM SRAM = DS3232SRAM();  // instantiate for use
