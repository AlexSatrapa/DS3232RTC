#include <DS3232.h>

/* +----------------------------------------------------------------------+ */
/* | DS3232 Class                                                      | */ 
/* +----------------------------------------------------------------------+ */

/**
 *
 */
DS3232::DS3232() {
}

bool DS3232::available() {
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
uint8_t DS3232::read1(uint8_t addr) {
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
void DS3232::write1(uint8_t addr, uint8_t data){
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
}

void DS3232::readN(uint8_t addr, uint8_t buf[], uint8_t len)
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

void DS3232::writeN(uint8_t addr, uint8_t buf[], uint8_t len)
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
