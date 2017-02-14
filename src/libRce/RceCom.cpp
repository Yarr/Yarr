#include "RceCom.h"



PgpModL pgpl;

RceCom::RceCom() {
  pgpl.open();
  pgp= PgpTrans::RCDImaster::instance();
  
  m_counter=0;
}

RceCom::~RceCom() {
  pgpl.close();
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
void RceCom::receive(PgpTrans::PgpData *pgpdata){
  //  int link=pgpdata->header[2];
  //std::cout<<"Link is "<<link<<std::endl;
  //std::cout<<"Payload "<<std::hex<<pgpdata->payload[0]<<std::dec<<std::endl;
  int size=pgpdata->payloadSize;
  //std::cout<<"Size "<<size<<std::endl;
  //  if (link==PGPACK)return; //handshake from serialization command
  //printf("Payloadsize %d Headersize %d\n",payloadSize,headerSize);
  uint32_t* data;
  
  data=pgpdata->payload;

  m_counter++;
  m_lock.lock();
  for(int i=0;i<size;i++) rxfifo.push(data[i]);
  m_lock.unlock();

  //m_timer.Start();
  //  m_handler->handle(link,data,size);
   //m_timer.Stop();

  //std::cout<<"Parsing done"<<std::endl;
}


uint32_t RceCom::getCurSize()
{
   
  m_lock.lock();
  return rxfifo.size();
  m_lock.unlock();
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
