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

bool DS3232RTC::available() {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0x05);  // sends 05h - month register
  Wire.endTransmission();
  Wire.requestFrom(DS3232_I2C_ADDRESS, 1);
  if (Wire.available()) {
    Wire.read();
    return true;
  }
  return false;
}
  
/**
 *
 */
time_t DS3232RTC::get() {
  tmElements_t tm;
  read(tm);
  return makeTime(tm);
}

/**
 *
 */
void DS3232RTC::set(time_t t) {
  tmElements_t tm;
  breakTime(t, tm);
  write(tm); 
}

/**
 *
 */
void DS3232RTC::read( tmElements_t &tm ) { 
  uint8_t TimeDate[7];
  uint8_t century = 0;

  readN(DS323X_TIME_REGS, TimeDate, 7);

  tm.Second = bcd2dec(TimeDate[0] & 0x7F);
  tm.Minute = bcd2dec(TimeDate[1] & 0x7F);
  if ((TimeDate[2] & 0x40) != 0)
  {
    // Convert 12-hour format to 24-hour format
    tm.Hour = bcd2dec(TimeDate[2] & 0x1F);
    if((TimeDate[2] & 0x20) != 0) tm.Hour += 12;
  } else {
    tm.Hour = bcd2dec(TimeDate[2] & 0x3F);
  }
  tm.Wday = bcd2dec(TimeDate[3] & 0x07);
  tm.Day = bcd2dec(TimeDate[4] & 0x3F);
  tm.Month = bcd2dec(TimeDate[5] & 0x1F);
  tm.Year = bcd2dec(TimeDate[6]);
  century = (TimeDate[5] & 0x80);
  if (century != 0) tm.Year += 100;
  tm.Year = y2kYearToTm(tm.Year);
}

inline void DS3232RTC::populateTimeElements( tmElements_t &tm, uint8_t TimeDate[] )
{
	TimeDate[0] = dec2bcd(tm.Second);
	TimeDate[1] = dec2bcd(tm.Minute);
	TimeDate[2] = dec2bcd(tm.Hour);
}

inline void DS3232RTC::populateDateElements( tmElements_t &tm, uint8_t TimeDate[] )
{
	uint8_t y;

	if( tm.Wday == 0 || tm.Wday > 7)
	{
		tmElements_t tm2;
		breakTime( makeTime(tm), tm2 );  // Calculate Wday by converting to Unix time and back
		tm.Wday = tm2.Wday;
	}
	TimeDate[3] = tm.Wday;
	TimeDate[4] = dec2bcd(tm.Day);
	TimeDate[5] = dec2bcd(tm.Month);
	y = tmYearToY2k(tm.Year);
	if (y > 99)
	{
		TimeDate[5] |= 0x80; // century flag
		y -= 100;
	}
	TimeDate[6] = dec2bcd(y);
} 

/**
 *
 */
void DS3232RTC::writeTime(tmElements_t &tm) {
  uint8_t TimeDate[7];

  TimeDate[0] = dec2bcd(tm.Second);
  TimeDate[1] = dec2bcd(tm.Minute);
  TimeDate[2] = dec2bcd(tm.Hour);

  writeN(DS323X_TIME_REGS, TimeDate, 3);
}

/**
 *
 */
void DS3232RTC::writeDate(tmElements_t &tm)
{
  uint8_t y;
  uint8_t TimeDate[7];

  if( tm.Wday == 0 || tm.Wday > 7)
  {
    tmElements_t tm2;
    breakTime( makeTime(tm), tm2 );  // Calculate Wday by converting to Unix time and back
    tm.Wday = tm2.Wday;
  }
  TimeDate[3] = tm.Wday;
  TimeDate[4] = dec2bcd(tm.Day);
  TimeDate[5] = dec2bcd(tm.Month);
  y = tmYearToY2k(tm.Year);
  if (y > 99)
  {
    TimeDate[5] |= 0x80; // century flag
    y -= 100;
  }
  TimeDate[6] = dec2bcd(y);

  writeN(DS323X_DATE_REGS, TimeDate + 3 * sizeof(uint8_t), 4);
}

/**
 *
 */
void DS3232RTC::write(tmElements_t &tm) {
  uint8_t TimeDate[7];

  populateTimeElements(tm, TimeDate);
  populateDateElements(tm, TimeDate);

  writeN(DS323X_TIME_REGS, TimeDate, 7);
}

/**
 *
 */
void DS3232RTC::readAlarm(uint8_t alarm, alarmMode_t &mode, tmElements_t &tm) {
  uint8_t data[4];
  uint8_t flags;
  uint8_t addr, offset, length;

  memset(&tm, 0, sizeof(tmElements_t));
  mode = alarmModeUnknown;
  if ((alarm > 2) || (alarm < 1)) return;
  if (alarm == 1)
  {
    addr = DS323X_ALARM1_REGS;
    offset = 0;
    length = 4;
  } else {
    addr = DS323X_ALARM2_REGS;
    offset = 1;
    length = 3;
  }

  data[0] = 0;
  readN(addr, data + offset * sizeof(uint8_t), length);

  flags = ((data[0] & 0x80) >> 7) | ((data[1] & 0x80) >> 6) |
    ((data[2] & 0x80) >> 5) | ((data[3] & 0x80) >> 4);
  if (flags == 0) flags = ((data[3] & 0x40) >> 2);
  switch (flags) {
    case 0x04: mode = alarmModePerSecond; break;  // X1111
    case 0x0E: mode = (alarm == 1) ? alarmModeSecondsMatch : alarmModePerMinute; break;  // X1110
    case 0x0C: mode = alarmModeMinutesMatch; break;  // X1100
    case 0x08: mode = alarmModeHoursMatch; break;  // X1000
    case 0x00: mode = alarmModeDateMatch; break;  // 00000
    case 0x10: mode = alarmModeDayMatch; break;  // 10000
  }

  if (alarm == 1) tm.Second = bcd2dec(data[0] & 0x7F);
  tm.Minute = bcd2dec(data[1] & 0x7F);
  if ((data[2] & 0x40) != 0) {
    // 12 hour format with bit 5 set as PM
    tm.Hour = bcd2dec(data[2] & 0x1F);
    if ((data[2] & 0x20) != 0) tm.Hour += 12;
  } else {
    // 24 hour format
    tm.Hour = bcd2dec(data[2] & 0x3F);
  }
  if ((data[3] & 0x40) == 0) {
    // Alarm holds Date (of Month)
    tm.Day = bcd2dec(data[3] & 0x3F);
  } else {
    // Alarm holds Day (of Week)
    tm.Wday = bcd2dec(data[3] & 0x07);
  }

  // TODO : Not too sure about this.
  /*
    If the alarm is set to trigger every Nth of the month
    (or every 1-7 week day), but the date/day are 0 then
    what?  The spec is not clear about alarm off conditions.
    My assumption is that it would not trigger is date/day
    set to 0, so I've created a Alarm-Off mode.
  */
  if ((mode == alarmModeDateMatch) && (tm.Day == 0)) {
    mode = alarmModeOff;
  } else if ((mode == alarmModeDayMatch) && (tm.Wday == 0)) {
    mode = alarmModeOff;
  }
}

/**
 *
 */
void DS3232RTC::writeAlarm(uint8_t alarm, alarmMode_t mode, tmElements_t tm) {
  uint8_t data[4];
  uint8_t addr, offset, length;

  switch (mode) {
    case alarmModePerSecond:
      data[0] = 0x80;
      data[1] = 0x80;
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModePerMinute:
      data[0] = 0x00;
      data[1] = 0x80;
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModeSecondsMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x80;
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModeMinutesMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x80;
      data[3] = 0x80;
      break;
    case alarmModeHoursMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x00 | dec2bcd(tm.Hour);
      data[3] = 0x80;
      break;
    case alarmModeDateMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x00 | dec2bcd(tm.Hour);
      data[3] = 0x00 | dec2bcd(tm.Day);
      break;
    case alarmModeDayMatch:
      data[0] = 0x00 | dec2bcd(tm.Second);
      data[1] = 0x00 | dec2bcd(tm.Minute);
      data[2] = 0x00 | dec2bcd(tm.Hour);
      data[3] = 0x40 | dec2bcd(tm.Wday);
      break;
    case alarmModeOff:
      data[0] = 0x00;
      data[1] = 0x00;
      data[2] = 0x00;
      data[3] = 0x00;
      break;
    default: return;
  }

  if (alarm == 1)
  {
    addr = DS323X_ALARM1_REGS;
    offset = 0;
    length = 4;
  } else {
    addr = DS323X_ALARM2_REGS;
    offset = 1;
    length = 3;
  }

  writeN( addr, data + offset * sizeof(uint8_t), length);
}

/**
 * Enable or disable the Oscillator in battery-backup mode, always on when powered by Vcc
 */
void DS3232RTC::setBBOscillator(bool enable) {
  // Bit7 is NOT EOSC, i.e. 0=started, 1=stopped when on battery power
  uint8_t value = readControlRegister();  // sends 0Eh - Control register
  if (enable) {
    value &= ~(DS323X_EOSC);
  } else {
    value |= DS323X_EOSC;
  }
  writeControlRegister(value);
}

/**
 * Enable or disable the Sqare Wave in battery-backup mode
 */
void DS3232RTC::setBBSqareWave(bool enable) {
  uint8_t value = readControlRegister();
  if (enable) {
    value |= DS323X_BBSQW;
  } else {
    value &= ~(DS323X_BBSQW);
  }
  writeControlRegister(value);
}

/**
 *  Set the SQI pin to either a square wave generator or an alarm interupt
 */
void DS3232RTC::setSQIMode(sqiMode_t mode) {
  uint8_t value = readControlRegister() & 0xE0;  // sends 0Eh - Control register
  switch (mode) {
    case sqiModeNone: value |= DS323X_INTCN; break;
    case sqiMode1Hz: value |= DS323X_RS_1HZ;  break;
    case sqiMode1024Hz: value |= DS323X_RS_1024HZ; break;
    case sqiMode4096Hz: value |= DS323X_RS_4096HZ; break;
    case sqiMode8192Hz: value |= DS323X_RS_8192HZ; break;
    case sqiModeAlarm1: value |= (DS323X_INTCN | DS323X_A1IE); break;
    case sqiModeAlarm2: value |= (DS323X_INTCN | DS323X_A2IE); break;
    case sqiModeAlarmBoth: value |= (DS323X_INTCN | DS323X_A1IE | DS323X_A2IE); break;
  }
  writeControlRegister(value);
}

/**
 * Returns 1 if the given alarm is enabled
 */

bool DS3232RTC::isAlarmInterrupt(uint8_t alarm) {
  if ((alarm > 2) || (alarm < 1)) return false;
  uint8_t value = readControlRegister() & 0x07;  // sends 0Eh - Control register
  if (alarm == 1) {
    return ((value & 0x05) == 0x05);
  } else {
    return ((value & 0x06) == 0x06);
  }
}

/**
 *  Read the control register.
 */
uint8_t DS3232RTC::readControlRegister() {
  return read1(DS323X_CONTROL_REG);
}

void DS3232RTC::writeControlRegister(uint8_t value)
{
  write1(DS323X_CONTROL_REG, value);
}

/**
 *  Read the status register.
 */
uint8_t DS3232RTC::readStatusRegister() {
  return read1(DS323X_STATUS_REG);
}

void DS3232RTC::writeStatusRegister(uint8_t value)
{
  write1(DS323X_STATUS_REG, value);
}


/**
 *
 */
bool DS3232RTC::isOscillatorStopFlag() {
  uint8_t value = readStatusRegister();
  return ((value & DS323X_OSF) != 0);
}

/**
 *
 */
void DS3232RTC::setOscillatorStopFlag(bool enable) {
  uint8_t value = readStatusRegister();
  if (enable) {
    value |= DS323X_OSF;
  } else {
    value &= ~(DS323X_OSF);
  }
  writeStatusRegister(value);
}

/**
 *
 */
void DS3232RTC::setBB33kHzOutput(bool enable) {
  uint8_t value = readStatusRegister();
  if (enable) {
    value |= DS323X_BB33KHZ;
  } else {
    value &= ~(DS323X_BB33KHZ);
  }
  writeStatusRegister(value);
}

/**
 *
 */
void DS3232RTC::setTCXORate(tempScanRate_t rate) {
  const uint8_t rateBitMask = ~(DS323X_CRATE1|DS323X_CRATE0);
  uint8_t value = readStatusRegister() & rateBitMask;  // clear rate bits
  switch (rate) {
    case tempScanRate64sec: value |= DS323X_CRATE_64; break;
    case tempScanRate128sec: value |= DS323X_CRATE_128; break;
    case tempScanRate256sec: value |= DS323X_CRATE_256; break;
    case tempScanRate512sec: value |= DS323X_CRATE_512; break;
  }
  writeStatusRegister(value);
}

/**
 *
 */
void DS3232RTC::set33kHzOutput(bool enable) {
  uint8_t value = readStatusRegister();
  if (enable) {
    value |= DS323X_EN33KHZ;
  } else {
    value &= ~(DS323X_EN33KHZ);
  }
  writeStatusRegister(value);
}

/**
 *
 */
bool DS3232RTC::isTCXOBusy() {
  uint8_t value = readStatusRegister();
  return ((value & DS323X_BSY) != 0);
}

/**
 * Returns 1 if the given alarm is flagged
 */
bool DS3232RTC::isAlarmFlag(uint8_t alarm) {
  uint8_t value = isAlarmFlag();
  return ((value & alarm) != 0);
}

/**
 *
 */
uint8_t DS3232RTC::isAlarmFlag(){
  uint8_t value = readStatusRegister();
  return (value & (DS323X_A1F | DS323X_A2F));
}

/**
 *
 */
void DS3232RTC::clearAlarmFlag(uint8_t alarm) {
	uint8_t alarm_mask, value;

	if ((alarm != 1) and (alarm != 2) and (alarm != 3)) return;
	alarm_mask = ~alarm;

	value = readStatusRegister();
	value &= alarm_mask;
	writeStatusRegister(value);
}

/**
 *
 */
void DS3232RTC::readTemperature(tpElements_t &tmp) {
  uint8_t data[2];

  readN(DS323X_TEMP_MSB, data, 2);

  tmp.Temp = data[0];
  tmp.Decimal = (data[1] >> 6) * 25;
}

/**
 * Convert Decimal to Binary Coded Decimal (BCD)
 */
uint8_t DS3232RTC::dec2bcd(uint8_t num) {
  return (num/10 * 16) + (num % 10);
}

/**
 * Convert Binary Coded Decimal (BCD) to Decimal
 */
uint8_t DS3232RTC::bcd2dec(uint8_t num) {
  return (num/16 * 10) + (num % 16);
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
  Wire.endTransmission();
  Wire.requestFrom((uint8_t) DS3232_I2C_ADDRESS, (uint8_t) len);
  if (Wire.available())
  {
    for (i=0; i<len; i++)
    {
      buf[i] = Wire.read();
    }
  }
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
