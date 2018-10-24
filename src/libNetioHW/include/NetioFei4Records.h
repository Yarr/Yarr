#ifndef NETIOFEI4RECORDS_H
#define NETIOFEI4RECORDS_H

#include <iostream>
#include <iomanip>
#include <bitset>
#include <utility>

//Convert into 32 bit chunks.
// [23:0] payload (24bit FEI4 record)
// [25:24] type (0 = fei4, 1 = trigger tag)
// [31:26] channel (0-63)
union YARR_RECORD {
  struct{
    uint8_t payload1 : 8;
    uint8_t payload2 : 8;
    uint8_t payload3 : 8;
    uint8_t type     : 1;
    uint8_t channel  : 7;
  } inner;
  void print(){
    std::cout << "YARR_RECORD: "
              << " channel: " << std::hex << inner.channel << std::dec
              << " type: " << std::hex << inner.type << std::dec << std::endl;
  }
  uint32_t allfields;
  YARR_RECORD(uint32_t data):allfields(data){}
};

union FEI4_RECORD { // FEI4 24bit Record Word
  struct{
    uint8_t field1 : 8;
    uint8_t field2 : 8;
    uint16_t rest  : 16;
  } inner;
  void print(){
    std::cout << "FEI4_RECORD: "
              << " field1: " << std::hex << inner.field1 << std::dec
              << " field2: " << std::hex << inner.field2 << std::dec
              << " rest: "   << std::hex << inner.rest << std::dec << std::endl;
  }
  uint32_t allfields;
  FEI4_RECORD(uint32_t data):allfields(data){}
};


union FEI4_DH { // FEI4 Data Header word.
  struct {
    uint8_t field1 : 8;
    uint8_t flag   : 1;
    uint8_t lv1id  : 5;
    uint16_t bcid  : 10;
  } inner;
  void print(){
    std::cout << " -> field1: " << unsigned(inner.field1);
    std::cout << " flag: " << bool(inner.flag);
    std::cout << " lv1id: " << unsigned(inner.lv1id);
    std::cout << " bcid: " << unsigned(inner.bcid) << std::endl;
  }
  void printHex(){
    std::cout << " -> field1: " << std::hex << static_cast<uint32_t>(inner.field1) << std::dec;
    std::cout << " flag: " << std::hex << static_cast<uint32_t>(inner.flag) << std::dec;
    std::cout << " lv1id: " << std::hex << static_cast<uint32_t>(inner.lv1id) << std::dec;
    std::cout << " bcid: " << std::hex << static_cast<uint32_t>(inner.bcid) << std::dec << std::endl;
  }
  uint32_t allfields;
  FEI4_DH(uint32_t data):allfields(data){}
};


union FEI4_DR_CR{ // FEI4 Data Record's Row and Column field.
  struct{
    uint8_t column : 7;
    uint16_t row   : 9;
  } inner;
  void print(){
    std::cout << " -> column: " << unsigned(inner.column);
    std::cout << " row: " << unsigned(inner.row) << std::endl;
  }
  void printHex(){
    std::bitset<16> bscr(allfields);
    std::cout << " -> as bitset: " << bscr << std::endl;
    std::bitset<16> bsCOL = bscr >> 9; // Shift right ROW[8:0], to cut ROW from LSB.
    std::cout << " -> as bitcol: " << bsCOL << " COLval: " << (unsigned short)bsCOL.to_ulong() << std::endl;
    std::bitset<16> bsROW = bscr << 7 >> 7; // Shift left COL[6:0], then again to the right to cut COL from MSB.
    std::cout << " -> as bitrow: " << bsROW << " ROWval: " << (unsigned short)bsROW.to_ulong() << std::endl;
  }
  std::pair<unsigned, unsigned> getColAndRow(){
    std::bitset<16> bscr(allfields);
    std::bitset<16> bsCOL = bscr >> 9; // Shift right ROW[8:0], to cut ROW from LSB.
    std::bitset<16> bsROW = bscr << 7 >> 7; // Shift left COL[6:0], then again to the right to cut COL from MSB.
    return std::make_pair<unsigned, unsigned>( (unsigned short)bsCOL.to_ulong(), (unsigned short)bsROW.to_ulong());
  }
  uint16_t allfields;
  FEI4_DR_CR(uint16_t data):allfields(data){}
};

union FEI4_DR { // FEI4 Data Record
  struct {
    //uint16_t colAndRow : 16;
    uint8_t tot1       : 4;
    uint8_t tot2       : 4;
    uint32_t colAndRow : 16;
  } inner;
  uint16_t swapColAndRow(){
    uint16_t swappedCR = ((inner.colAndRow & 0xff) << 8) | ((inner.colAndRow & 0xff00) >> 8);
    std::cout << " -> swap COL and ROW: " << std::hex << static_cast<uint16_t>(swappedCR) << std::dec << std::endl;
    inner.colAndRow = swappedCR;
    return inner.colAndRow;
  }
  uint16_t getColAndRow(){
    return inner.colAndRow;
  }
  void print(){
    std::cout << " -> column and row on a uint16_t: " << unsigned(inner.colAndRow);
    std::cout << " tot1: " << unsigned(inner.tot1);
    std::cout << " tot2: " << unsigned(inner.tot2) << std::endl;
  }
  void printHex(){
    std::cout << " -> column and row on a uint16_t: " << std::hex << static_cast<uint32_t>(inner.colAndRow) << std::dec;
    std::cout << " tot1: " << std::hex << static_cast<uint32_t>(inner.tot1) << std::dec;
    std::cout << " tot2: " << std::hex << static_cast<uint32_t>(inner.tot2) << std::dec << std::endl;
  }
  std::pair<unsigned, unsigned> getTots(){
    return std::make_pair<unsigned, unsigned>(unsigned(inner.tot1), unsigned(inner.tot2));
  }
  uint32_t allfields;
  FEI4_DR(uint32_t data):allfields(data){}
};


union FEI4_AR { // FEI4 Address Record
  struct {
    uint8_t field1   : 8;
    uint8_t type     : 1;
    uint16_t address : 16;
  } inner;
  void print(){
    std::cout << " -> field1: " << unsigned(inner.field1);
    std::cout << " typeFlag: " << unsigned(inner.type);
    std::cout << " address: " << unsigned(inner.address) << std::endl;
  }
  void printHex(){
    std::cout << " -> field1: " << std::hex << static_cast<uint32_t>(inner.field1) << std::dec;
    std::cout << " typeFlag: " << std::hex << static_cast<uint32_t>(inner.type) << std::dec;
    std::cout << " address: " << std::hex << static_cast<uint32_t>(inner.address) << std::dec << std::endl;
  }
  uint32_t allfields;
  FEI4_AR(uint32_t data):allfields(data){}
};


union FEI4_VR { // FEI4 Value Record
  struct {
    uint8_t field1 : 8;
    uint32_t value : 16;
  } inner;
  void print(){
    std::cout << " -> field1: " << unsigned(inner.field1);
    std::cout << " value: " << unsigned(inner.value) << std::endl;
  }
  void printHex(){
    std::cout << " -> field1: " << std::hex << static_cast<uint32_t>(inner.field1) << std::dec;
    std::cout << " value: " << std::hex << static_cast<uint32_t>(inner.value) << std::dec << std::endl;
  }
  uint32_t allfields;
  FEI4_VR(uint32_t data):allfields(data){}
};


union FEI4_SR_CN{
  struct {
    uint8_t code : 8;
    uint16_t num : 10;
  } inner;
  void printBits(){
    std::cout << " -> as hex: " << std::hex << static_cast<uint32_t>(allfields) << std::dec << std::endl;
    std::bitset<16> bscn(allfields);
    std::cout << " -> as bitset: " << bscn << std::endl;
    std::bitset<16> bsCOD = bscn >> 10; // Shift right NUMBER[9:0], to cut NUMBER from LSB.
    std::cout << " -> as bitcol: " << bsCOD << " CODval: " << (unsigned short)bsCOD.to_ulong() << std::endl;
    std::bitset<16> bsNUM = bscn << 6 >> 6; // Shift left CODE[5:0], then again to the right to cut CODE from MSB.
    std::cout << " -> as bitrow: " << bsNUM << " NUMval: " << (unsigned short)bsNUM.to_ulong() << std::endl;
  }
  uint16_t allfields;
  FEI4_SR_CN(uint16_t data):allfields(data){}
};

union FEI4_SR {
  struct {
    uint8_t  field1  : 8;
    uint32_t codeNum : 16; // use uint32_t!
  } inner;
  uint16_t swapCodeNum(){
    uint16_t swappedCN = ((inner.codeNum & 0xff) << 8) | ((inner.codeNum & 0xff00) >> 8);
    inner.codeNum = swappedCN;
    return inner.codeNum;
  }
  uint16_t getCodeNum(){
    return inner.codeNum;
  }
  void print(){
    std::cout << " -> field1: " << unsigned(inner.field1)
              << " code: " << unsigned(inner.codeNum) << std::endl;
  }
  void printHex() {
    std::cout << " -> : field1: " << std::hex << static_cast<uint32_t>(inner.field1) << std::dec
              << " codeNum: " << std::hex << static_cast<uint32_t>(inner.codeNum) << std::dec << std::endl;
  }
  uint32_t allfields;
  FEI4_SR(uint32_t data):allfields(data){}
};


struct WIB_FRAME {
  uint64_t field0 : 64;
  uint64_t field1 : 64;
  uint64_t field2 : 64;
  uint64_t field3 : 64;
  uint64_t field4 : 64;
  uint64_t field5 : 64;
  uint64_t field6 : 64;
  uint64_t field7 : 64;
};


#endif //  NETIOFEI4RECORDS_H

