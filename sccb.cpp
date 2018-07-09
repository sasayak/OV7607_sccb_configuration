#include "sccb.h"

/* I2C access functions */

void init_sccb(){
  if (USE_LOW_LEVEL_I2C){
    //set up twi for 100khz
    TWSR &= ~3;//disable prescaler for TWI
    TWBR = 72;//set to 100khz
  } else {
    Wire.begin();
  }
}

byte read_register(uint8_t reg){
  if (USE_LOW_LEVEL_I2C){
    return rdReg_ll(reg);
  } else {
    return rdReg_hl(reg);
  }
  
}

void write_register(uint8_t reg, uint8_t dat){
  if (USE_LOW_LEVEL_I2C){
    return wrReg_ll(reg, dat);
  } else {
    return wrReg_hl(reg, dat);
  }
}

void write_register_list(const struct regval_list reglist[]){
  uint8_t reg_addr, reg_val;
  const struct regval_list *next = reglist;
  while ((reg_addr != 0xff) | (reg_val != 0xff)){
    reg_addr = pgm_read_byte(&next->reg_num);
    reg_val = pgm_read_byte(&next->value);
    write_register(reg_addr, reg_val);
    next++;
  }
}

/* I2C high level functions */

void wrReg_hl(uint8_t reg, uint8_t dat){
  //send start condition
  Wire.beginTransmission(camAddr_WR >> 1);
  Wire.write(reg);
  Wire.write(dat);
  Wire.endTransmission();
  delay(1);
}

uint8_t rdReg_hl(uint8_t reg){
  uint8_t dat = 0;
  Wire.beginTransmission(camAddr_WR >> 1);
  Wire.write(reg);
  Wire.endTransmission();
  delay(1);
  Wire.requestFrom(camAddr_WR >> 1, 1);
  if(Wire.available()) {
    dat = Wire.read();
  }
  delay(1);
  return dat;
}


/* I2C low level functions */

void error_led(void){
  DDRB |= 32;//make sure led is output
  while (1){//wait for reset
    PORTB ^= 32;// toggle led
    _delay_ms(100);
  }
}

void twiStart(void){
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);//send start
  while (!(TWCR & (1 << TWINT)));//wait for start to be transmitted
  if ((TWSR & 0xF8) != TW_START)
    error_led();
}

void twiWriteByte(uint8_t DATA, uint8_t type){
  TWDR = DATA;
  TWCR = _BV(TWINT) | _BV(TWEN);
  while (!(TWCR & (1 << TWINT))) {}
  if ((TWSR & 0xF8) != type)
    error_led();
}

void twiAddr(uint8_t addr, uint8_t typeTWI){
  TWDR = addr;//send address
  TWCR = _BV(TWINT) | _BV(TWEN);    /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0); /* wait for transmission */
  if ((TWSR & 0xF8) != typeTWI)
    error_led();
}

void wrReg_ll(uint8_t reg, uint8_t dat){
  //send start condition
  twiStart();
  twiAddr(camAddr_WR, TW_MT_SLA_ACK);
  twiWriteByte(reg, TW_MT_DATA_ACK);
  twiWriteByte(dat, TW_MT_DATA_ACK);
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);//send stop
  _delay_ms(1);
}

static uint8_t twiRd(uint8_t nack){
  if (nack){
    TWCR = _BV(TWINT) | _BV(TWEN);
    while ((TWCR & _BV(TWINT)) == 0); /* wait for transmission */
    if ((TWSR & 0xF8) != TW_MR_DATA_NACK)
      error_led();
    return TWDR;
  }
  else{
    TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
    while ((TWCR & _BV(TWINT)) == 0); /* wait for transmission */
    if ((TWSR & 0xF8) != TW_MR_DATA_ACK)
      error_led();
    return TWDR;
  }
}

uint8_t rdReg_ll(uint8_t reg){
  uint8_t dat;
  twiStart();
  twiAddr(camAddr_WR, TW_MT_SLA_ACK);
  twiWriteByte(reg, TW_MT_DATA_ACK);
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);//send stop
  _delay_ms(1);
  twiStart();
  twiAddr(camAddr_RD, TW_MR_SLA_ACK);
  dat = twiRd(1);
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);//send stop
  _delay_ms(1);
  return dat;
}
