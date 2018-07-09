#include "l_driver.h"





void setColorSpace(enum COLORSPACE color){
  switch(color){
    case YUV422:
      write_register_list(yuv422_ov7670);
    break;
    case RGB565:
      write_register_list(rgb565_ov7670);
      {uint8_t temp=read_register(0x11);
      delay(1);
      write_register(0x11,temp);}//according to the Linux kernel driver rgb565 PCLK needs rewriting
    break;
    case BAYER_RGB:
      write_register_list(bayerRGB_ov7670);
    break;
  }
  delay(SETTINGS_WAIT_TIME);
}
void setRes(enum RESOLUTION res){
  switch(res){
    case VGA:
      write_register(REG_COM3,0);  // REG_COM3
      write_register_list(vga_ov7670);
    break;
    case QVGA:
      write_register(REG_COM3,4);  // REG_COM3 enable scaling
      write_register_list(qvga_ov7670);
    break;
    case QQVGA:
      write_register(REG_COM3,4);  // REG_COM3 enable scaling
      write_register_list(qqvga_ov7670);
    break;
  }
  delay(SETTINGS_WAIT_TIME);
}
void camInit(void){
  write_register(0x12, 0x80);//Reset the camera.
  delay(100);
  write_register_list(ov7670_default_regs);
  delay(SETTINGS_WAIT_TIME);
}


void setPCKLonHREF(bool PCKLon){
  if(PCKLon){
    write_register(REG_COM10, 0); //PCLK toggles on HBLANK. (Freerunning)
  }else{
    write_register(REG_COM10, 32); //PCLK does not toggle on HBLANK.
  }
  delay(SETTINGS_WAIT_TIME);
}

byte setPCKLDivider(byte divider){
  // check range
  if(divider > 0x1F){
    write_register(0x11, 0x1F);
  } else {
    write_register(0x11, divider);
  }
  delay(SETTINGS_WAIT_TIME);
  return read_register(0x11);
}

void init_l_driver(void){
  camInit();
  setPCKLonHREF(false);
  setRes(QVGA);
  setColorSpace(YUV422);
  setPCKLDivider(12);
  delay(SETTINGS_WAIT_TIME);
}
