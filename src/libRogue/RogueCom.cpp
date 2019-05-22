#include <iostream>

#include <RogueCom.h>
#include <memory>
#include <unistd.h>
void RogueSender::send(uint8_t *data,uint32_t size) {
  rogue::interfaces::stream::FramePtr frame;
  rogue::interfaces::stream::FrameIterator it;
  
  // Request frame
  frame = reqFrame(size,true);
  
  // Get data write iterator
  it = frame->beginWrite();
  
  std::copy(data,data+size,it);
  
  // Set new frame size
  frame->setPayload(size);
  tx_bytes+=size;
  tx_pkts++;
  if(size<tx_min_pkt) tx_min_pkt=size;
  if(size>tx_max_pkt) tx_max_pkt=size;  
  //Send frame
  sendFrame(frame);
}

void RogueReceiver::acceptFrame ( std::shared_ptr<rogue::interfaces::stream::Frame> frame) {
  uint32_t nbytes = frame->getPayload();
         // Iterators to start and end of frame
  rogue::interfaces::stream::Frame::iterator iter = frame->beginRead();
  rogue::interfaces::stream::Frame::iterator  end = frame->endRead();
  
  //Iterate through contigous buffers
  while ( iter != end ) {
    
    //  Get contigous size
    auto size = iter.remBuffer ();
    
    // Get the data pointer from current position
    auto *src = iter.ptr ();
    uint8_t *data=(uint8_t*)src;
    if((_port==5) && ((nbytes%8)==0)) {
//	  uint32_t tlutrigword= (0x1ffff<<15) + (_com->readTLUtrigword()&0x7fff);//15bits TLU trigword
//	  //std::cout<<"tlutrigword="<<tlutrigword<<std::endl;
//      _com->queue_data(&tlutrigword,1);      

      uint32_t *p=(uint32_t*)data;
      rx_bytes+=nbytes;
      rx_pkts++;
      if(nbytes<rx_min_pkt) rx_min_pkt=nbytes;
      if(nbytes>rx_max_pkt) rx_max_pkt=nbytes;      
      _com->queue_data(p,nbytes/4);      
    }
    
    // Update destination pointer and source iterator
    iter += size;
  } 
}



void RogueCom::connect(const std::string &conn,uint32_t port) {
  _port=port;  
  for(unsigned i=0;i<1024;i++) bram[i]=0;
  trigLength=0;
  trigIter=0;
  trigFreq=0;
  if(conn.find("axis://") ==0  ) {
    std::string devname=conn;
    devname.erase(0,7);
    axisrp  =  rogue::hardware::axi::AxiStreamDma::create(devname,0,true); 
    axicfg  =  rogue::hardware::axi::AxiStreamDma::create(devname,port+1,true); 
    axidata =  rogue::hardware::axi::AxiStreamDma::create(devname,port+5,true); 
    srp = rogue::protocols::srp::SrpV3::create();
    axisrp->setSlave(srp);
    srp->setSlave(axisrp);
    mast = rogue::interfaces::memory::Master::create();
    mast->setSlave(srp);
    configStream = std::make_shared<RogueSender>(port+1);
    configStream->setSlave(axicfg);
    dataStream = std::make_shared<RogueReceiver>(port+5,this->getInstance());
    debugStream=std::make_shared<RogueReceiver>(port+1,this->getInstance());
    axidata->setSlave(dataStream);
    axicfg->setSlave(debugStream);    
  }
  else if (conn.find("ip://")== 0 ) {
    std::string ip=conn;
    ip.erase(0,5);
    udp = rogue::protocols::udp::Client::create(ip,8192,true);
    udp->setRxBufferCount(128); 
    rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());
    udp->setSlave(rssi->transport());
    rssi->transport()->setSlave(udp);

    // Packetizer, ibCrc = false, obCrc = true
    pack = rogue::protocols::packetizer::CoreV2::create(false,true,true);
    rssi->application()->setSlave(pack->transport());
    pack->transport()->setSlave(rssi->application());

    // Create an SRP master and connect it to the packetizer
    srp = rogue::protocols::srp::SrpV3::create();
    pack->application(0)->setSlave(srp);
    srp->setSlave(pack->application(0));

    // Create a memory master and connect it to the srp
    mast = rogue::interfaces::memory::Master::create();
    mast->setSlave(srp);
    configStream = std::make_shared<RogueSender>(port+1);
    configStream->setSlave(pack->application(port+1));
    dataStream= std::make_shared<RogueReceiver>(port+5,this->getInstance());
    debugStream=std::make_shared<RogueReceiver>(port+1,this->getInstance());
    pack->application(port+5)->setSlave(dataStream);
    pack->application(port+1)->setSlave(debugStream);   
    rssi->start();    
    while ( ! rssi->getOpen() ) {
      sleep(1);
      std::cout << "Establishing link ...\n";
    }
  } else {
    throw;
  }
  sysReg=0x00030000;
  rxPhyMon= 0x01000000*(port+1) + 0x00100000;    
  trigEmu=0x05000000+0x20000;
  trigLUT=0x05000000+0x10000;
  trigTLU=0x05000000+0x00000;
}

uint32_t  RogueCom::getCurSize(){
  rx_lock.lock();
  uint32_t size= rxfifo.size();
  rx_lock.unlock();
  return size;
}
bool  RogueCom::isEmpty() {
  tx_lock.lock();
  bool result=txfifo.empty();
  tx_lock.unlock();
  return result;
}
uint32_t  RogueCom::read32(){
  rx_lock.lock();
  uint32_t val=rxfifo.front();
  rxfifo.pop();
  rx_lock.unlock();
  return val;
}
uint32_t  RogueCom::readBlock32(uint32_t *buf, uint32_t length){
  rx_lock.lock();
  unsigned l=rxfifo.size();
  if(l>length) l=length;
  for(unsigned i=0;i<l;i++) {
    buf[i]=rxfifo.front();
    rxfifo.pop();    
  } 
  rx_lock.unlock();
  return l;
}
void  RogueCom::write32(uint32_t value){
  tx_lock.lock();
  txfifo.push_back(value);
  tx_lock.unlock();
}
void  RogueCom::releaseFifo(){
  tx_lock.lock();
  configStream->send((uint8_t*)txfifo.data(),sizeof(uint32_t)*txfifo.size());
  txfifo.clear();
  tx_lock.unlock();
}
void RogueCom::enableLane(uint32_t mask){
  uint32_t temp=mask&0xf;
  writeRegister(rxPhyMon+0x800,temp);
}
void RogueCom::enableDebugStream(bool enable){
  uint32_t temp=enable?1:0;
  writeRegister(rxPhyMon+0x810,temp);
}
