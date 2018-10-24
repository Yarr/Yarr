#include "NetioTools.h"
#include <iostream>

using namespace std;

uint64_t NetioTools::channel2elink(uint32_t channel){
  return channel*2;
}

uint32_t NetioTools::elink2channel(uint64_t elink){
  return elink/2;
}

int32_t NetioTools::msb(uint32_t num){
  int32_t msb=-1;
  for(int32_t i=31;i>=0;i--){
	if((num>>i)&1 && msb==0){
	  msb=i+1;
	  break;
	}
  }
  return msb;
}

uint32_t NetioTools::pad(uint32_t num, uint32_t n){
  uint32_t shift=(8-n%8);
  if(shift==8) return num;
  return num<<shift;
}

uint32_t NetioTools::conv4(uint32_t num, int32_t pos){
  uint32_t rnum = 0;
  for(int32_t i=(pos+1)*8-1;i>pos*8-1;i--){
	if((num>>i)&1){
	  rnum |= (0xF << (i%8*4));
	}
  }
  return rnum;
}

uint32_t NetioTools::conv4R(uint32_t num, uint32_t pos){
  uint32_t rnum = 0;
  for(uint32_t i=pos*8; i<(pos+1)*8; i++){
	if((num & (1 << i))) rnum |= 0xF << (32-(i+1)*4);
  }
  return rnum;
}

uint32_t NetioTools::flip2(uint32_t num){
  uint32_t rnum = 0;
  for(uint32_t i=0;i<4;i++){
	rnum |= (num & (0xF0 << 8*i )) >> 4;
	rnum |= (num & (0x0F << 8*i )) << 4;
  }
  return rnum;
}

uint32_t NetioTools::flip4(uint32_t num){
  uint32_t rnum = 0;
  for(uint32_t i=0;i<4;i++){
	rnum |= ((num >> 8*i) & 0xFF) << (24-8*i);
  }
  return rnum;
}

uint32_t NetioTools::reverse(uint32_t num){
  uint32_t rnum=0;
  for(uint32_t i=0;i<32;i++){
	if((num>>i)&1){ rnum |= 1 << ((32-1)-i); }
  }
  return rnum;
}
