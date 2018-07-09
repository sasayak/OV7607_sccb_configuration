// Author: Hendrik Borras

#include "sccb.h"
#include "l_driver.h"

// pin for the xclock of the camera
const byte CLOCKOUT = 9;


/* Functions */

/* UI functions */

char getCommand()
{
  char c = '\0';
  if (Serial.available())
  {
    c = Serial.read();
  }
  return c;
}

void print_reg_and_dat(uint8_t reg, uint8_t dat){
  Serial.print("\tReg: 0x");
  Serial.print(reg, HEX);
  Serial.print(" <= HEX:");
  Serial.print(dat, HEX);
  Serial.print(" = BIN:");
  Serial.println(dat, BIN);
}

void print_help(bool reduced=true){
  if (reduced){
    Serial.println("h: show the help screen");
  } else {
    Serial.println("--- Help screen");
    Serial.println("\th: show the help screen");
    Serial.println("\ta: read all registers");
    Serial.println("\tr: read one specific register");
    Serial.println("\tw: write one specific register");
    Serial.println("\tF: write the configuration for the FFFilters project (VGA, YUV422, PCKLDivider=0)");
    Serial.println("\tU: write the configuration from the arduino uno example (qvga and yuv422)");
    Serial.println("\tL: set registers according to standard values of the Linux driver (still needs a resolution and color space)");
    Serial.println("\t1: set resolution to VGA");
    Serial.println("\t2: set resolution to QVGA");
    Serial.println("\t3: set resolution to QQVGA");
    Serial.println("\t6: set color space to YUV422");
    Serial.println("\t7: set color space to RGB565");
    Serial.println("\t8: set color space to BAYER_RGB");
    Serial.println("\tp: toggle 'PCKL running while HREF is LOW'");
    Serial.println("\td: set PCKLDivider (register 0x11). The output clock (PCKL) will be: PCKL = XCKL / PCKLDivider");
    Serial.println("\tx: reset/clear all registers");
  }
}

byte get_hex_from_serial(){
  char hex_chars[2];
  // wait for the user to type something, then read it
  while(Serial.available() < 2){
    delay(1);
  }
  for(int i = 0; i<=2; i++){
    hex_chars[i] = Serial.read();
  }
  // empty the rest of the serial q
  while(Serial.available()) {Serial.read();}
  unsigned long number = strtoul( hex_chars, nullptr, 16);
  byte reg_to_read = byte(number);
  return reg_to_read;
}

bool check_if_reg_is_valid(byte reg_to_read){
  if(reg_to_read > 0xC9){
    Serial.print(reg_to_read, HEX);
    Serial.println(" is too large, maximum register is C9.");
    return false;
  }
  if(!check_read_allowance(reg_to_read)){
    Serial.print("\tNo read access to 0x");
    Serial.println(reg_to_read, HEX);
    return false;
  }
  return true;
}

/* I2C helpers */
const byte forbidden_addresses[] = {0x29, 0x35, 0x36, 0x49, 0x4A, 0x4D, 0x4E, 0x59, 0x60, 0x61, 0x78, 0x79, 0x8A, 0x8B, 0x8D, 0x8F, 0x90, 0x91, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0xA1, 0xA3, 0xB0, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};
const byte fb_max = 43;
bool check_read_allowance(byte reg){
  for (int i = 0; i < fb_max; i++) {
    if (forbidden_addresses[i] == reg) {
      return false;
    }
  }
  return true;
}

void reset_registers(){
  write_register(0x12, 0x80);
  delay(300);
}


/* Standard arduino setup and loop routines */

void setup() {
  Serial.begin(115200);
  while (!Serial);
  init_sccb();  
  
  // set up 8 MHz timer on CLOCKOUT (OC1A)
  pinMode (CLOCKOUT, OUTPUT); 
  // set up Timer 1
  TCCR1A = bit (COM1A0);  // toggle OC1A on Compare Match
  TCCR1B = bit (WGM12) | bit (CS10);   // CTC, no prescaling
  OCR1A =  0;       // output every cycle

  // wait a bit for the camera to catch up to the clock
  delay(400);

  Serial.println("--- Checking Manufacturer ID of device...");
  uint8_t MIDH = 0x1C;
  uint8_t MIDH_expected = 0x7F;
  uint8_t MIDL = 0x1D;
  uint8_t MIDL_expected = 0xA2;
  Serial.println("Expected output:");
  print_reg_and_dat(MIDH, MIDH_expected);
  print_reg_and_dat(MIDL, MIDL_expected);
  Serial.println("Got:");
  print_reg_and_dat(MIDH, read_register(MIDH));
  print_reg_and_dat(MIDL, read_register(MIDL));
  if((MIDH_expected != read_register(MIDH)) or (MIDL_expected != read_register(MIDL))){
    Serial.println("--- WARNING: Manufacturer ID missmatch! The device may not work as expected!");
  } else {
    Serial.println("--- ID's match! Continuing...");
  }
  
  print_help(false);  
}


bool PCKLon = false;

void loop() {

  switch(getCommand()){
    case 'a':
      {
        Serial.println("--- Printing the contents of all registers...");
        for(byte reg_to_read = 0x00; reg_to_read <= 0xC9; reg_to_read++){
          if(check_if_reg_is_valid(reg_to_read)){
            print_reg_and_dat(reg_to_read, read_register(reg_to_read));
          }
        }
      }
      print_help();
      break;
    case 'r':
      {
        Serial.println("--- Reading one register...");
        // empty the line before we do anything else
        while(Serial.available()) {Serial.read();}
        Serial.println("Please type the HEX number of the register in question (from 00 to C9):");
        byte reg_to_read = get_hex_from_serial();
        if(check_if_reg_is_valid(reg_to_read)){
          print_reg_and_dat(reg_to_read, read_register(reg_to_read));
        }
      }
      break;
    case 'x':
      reset_registers();
      Serial.println("--- Resetting/Clearing all registers complete");
      break;
    case 'w':
      {
        Serial.println("--- Writing one register...");
        // empty the line before we do anything else
        while(Serial.available()) {Serial.read();}
        Serial.println("Please type the HEX number of the register in question (from 00 to C9):");
        byte reg_to_write = get_hex_from_serial();
        if(check_if_reg_is_valid(reg_to_write)){
          // First display the rigisters current content
          Serial.print("Current state of the register: ");
          print_reg_and_dat(reg_to_write, read_register(reg_to_write));
          // empty the line before we do anything else
          while(Serial.available()) {Serial.read();}
          Serial.println("Please type the HEX value that should be written:");
          byte val_to_write = get_hex_from_serial();
          Serial.print("Writing 0x");
          Serial.print(val_to_write, HEX);
          Serial.print(" to register number 0x");
          Serial.println(reg_to_write, HEX);
          // write the new content
          write_register(reg_to_write, val_to_write);
          // check if the write was successfull
          Serial.print("Current state of the register: ");
          delay(300);
          byte reg_val_after_write = read_register(reg_to_write);
          print_reg_and_dat(reg_to_write, reg_val_after_write);
          if (reg_val_after_write != val_to_write){
            Serial.println("--- WARNING: Values written and read do not match.");
          }
        } else{
          Serial.println("--- Aborting write!");
        }
      }
      break;
    case 'U':
      {
        Serial.println("--- Configuring the camera to arduino uno example (qvga and yuv422)");
        init_l_driver();
        Serial.println("--- Finished configuration");
      }
      break;
    case 'h':
      print_help(false);
      break;
    case 'p':
      PCKLon = !PCKLon;
      setPCKLonHREF(PCKLon);
      Serial.print("--- Set 'PCKL running while HREF is LOW' to: ");
      Serial.println(PCKLon);
      break;
    case 'd':
      {
      Serial.println("--- Setting PCKLDivider...");
      Serial.println("Please type value for PCKLDivider in HEX (from 00 to 1F):");
      byte val_to_write = get_hex_from_serial();
      val_to_write = setPCKLDivider(val_to_write);
      Serial.print("--- Set PCKLDivider to: ");
      Serial.println(val_to_write, HEX);
      }
      break;
    case 'L':
      camInit();
      Serial.println("--- Setup of linux standard configuration complete");
      break;
    case '1':
      setRes(VGA);
      Serial.println("--- Setup of VGA resolution complete");
      break;
    case '2':
      setRes(QVGA);
      Serial.println("--- Setup of QVGA resolution complete");
      break;
    case '3':
      setRes(QQVGA);
      Serial.println("--- Setup of QQVGA resolution complete");
      break;
    case '6':
      setColorSpace(YUV422);
      Serial.println("--- Setup of YUV422 color space complete");
      break;
    case '7':
      setColorSpace(RGB565);
      Serial.println("--- Setup of RGB565 color space complete");
      break;
    case '8':
      setColorSpace(BAYER_RGB);
      Serial.println("--- Setup of BAYER_RGB color space complete");
      break;
    case 'F':
      Serial.println("--- Configuring the camera for the FFFilters project");
      camInit();
      setRes(VGA);
      setColorSpace(YUV422);
      setPCKLonHREF(true);
      setPCKLDivider(0);
      Serial.println("--- Finished configuration");
    
  }
  delay(10);
  
}

