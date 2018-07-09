#pragma once
#include <arduino.h>
#include <Wire.h>
#include <util/twi.h>
#include <util/delay.h>

// define wether the code should use the low level avr methods or the highlevel Arduino methods.
// Both will work just fine. The low level functions will run at 100kHz I2C speed and use less memory on the arduino.
// The high level functions will run at 400kHz(?) and use slightly more memory.
const bool USE_LOW_LEVEL_I2C = false;

// Camera address
#define camAddr_WR  0x42
#define camAddr_RD  0x43

struct regval_list{
  uint8_t reg_num;
  uint16_t value;
};


/* I2C access functions */
void init_sccb();
byte read_register(uint8_t reg);
void write_register(uint8_t reg, uint8_t dat);
void write_register_list(const struct regval_list reglist[]);

/* I2C high level functions */
void wrReg_hl(uint8_t reg, uint8_t dat);
uint8_t rdReg_hl(uint8_t reg);

/* I2C low level functions */
void error_led(void);
void twiStart(void);
void twiWriteByte(uint8_t DATA, uint8_t type);
void twiAddr(uint8_t addr, uint8_t typeTWI);
void wrReg_ll(uint8_t reg, uint8_t dat);
static uint8_t twiRd(uint8_t nack);
uint8_t rdReg_ll(uint8_t reg);
