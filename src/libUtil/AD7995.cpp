#include "AD7995.h"
#include <cmath>

AD7995::AD7995(SpecCom *arg_spec) : PeriphialI2C(arg_spec) {
    dev_addr = 0x28;
    this->init();
    ch_cnt = 0;
    for (unsigned int i=0; i<4; i++)
        ch_value[i] = 0;
    //setupRead(dev_addr);
}

AD7995::~AD7995() {
    sendNack();
}

void AD7995::setActiveChannels(bool ch1, bool ch2, bool ch3, bool ch4) {
    uint32_t val = 0x0;
    val |= (ch1&0x1)<<4;
    val |= (ch2&0x1)<<5;
    val |= (ch3&0x1)<<6;
    val |= (ch4&0x1)<<7;
    
    ch_cnt = 0;
    if (ch1)
        ch_cnt++;
    if (ch2)
        ch_cnt++;
    if (ch3)
        ch_cnt++;
    if (ch4)
        ch_cnt++;
    
    setupWrite(dev_addr);
    writeData(val & 0xFF);
}

double convert(double adc) {
    double T25 = 298.15;
    double B = 3435;
    double adc_max = 4096;
    double R = ((1.0/(adc/adc_max))-1)*10e3;
    double R0 = 10.0e3;
    //std::cout << "R/R0: " << (1.0/(adc/adc_max))-1 << std::endl; 
    return (1.0/((1.0/T25)+((1.0/B)*(log(R/R0)))))-273.15;
}

void AD7995::read() {
   setupRead(dev_addr);
   uint32_t value = 0;
   unsigned ch = 99;
   unsigned adc = 0;
   uint32_t tmp = 0;
  
   // Read all active channels
   for (unsigned i=0; i<ch_cnt; i++) {
       value = 0;
       tmp = 0;
       readData(&tmp);
       value |= (0xFF&tmp) << 8;
       readData(&tmp);
       value |= tmp & 0xFF;
       ch = (value >> 12) & 0x03;
       (void) ch;
       adc = value & 0x0FFF;
       ch_value[i] = convert(adc);
   }
   sendNack();
}

double AD7995::getValue(unsigned arg_ch) {
    if (arg_ch >= 4) {
        std::cerr << __PRETTY_FUNCTION__ << " : Channel not in range!" << std::endl;
        return 0;
    }
    return ch_value[arg_ch];
}


   
