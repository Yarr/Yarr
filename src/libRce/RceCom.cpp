#include "RceCom.h"
#include <iostream>

enum channeldefs {ADCREADOUT=29, TDCREADOUT=30, PGPACK=31};


RceCom::RceCom() {
  pgp= Rce::PGPmaster::instance();  
  pgp->setReceiver((Rce::Receiver*)this);
  m_counter=0;
}

RceCom::~RceCom() {
}
bool RceCom::isEmpty() {
  return txfifo.empty();
}
void RceCom::write32(uint32_t value) {
  txfifo.push_back(value);
}

uint32_t RceCom::read32(){
  return 0;
}
// PGP handler
void RceCom::receive(Rce::PgpData *pgpdata){
  int link=pgpdata->header[2];
  int size=pgpdata->payloadSize;
  if (link==(uint32_t)PGPACK)return; //handshake from serialization command
  unsigned int count=0;
  uint32_t word=0;
  uint8_t* data;

  data=(uint8_t*) pgpdata->payload;
  /* convert 24bit record stream to 32 bit words*/
 
  m_lock.lock();
  for(unsigned int i=0;i<size*sizeof(uint32_t);i++)  {
    word|=data[i];
    count++;
    if(count==3) {
      if(word!=0) rxfifo.push(word);
      count=0;
      word=0;
    } 
    word<<=8;
  }
  m_lock.unlock();
  m_counter++;
}


uint32_t RceCom::getCurSize()
{
  m_lock.lock();
  uint32_t size= rxfifo.size();
  m_lock.unlock();
  return size;
}


uint32_t RceCom::readBlock32(uint32_t *buf, uint32_t length) {
  m_lock.lock();
  for(unsigned int i=0;i<length;i++) {
    buf[i]=rxfifo.front();
    rxfifo.pop();
  }
  m_lock.unlock();
  return length;
  
}



void RceCom::releaseFifo() {
  if(txfifo.size()>0) { 
    pgp->blockWrite(txfifo.data(),txfifo.size(),true,false);
    txfifo.clear();
  }
  
} // Add some padding


