#ifndef __ROGUE_COM_H__
#define __ROGUE_COM_H__

#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <rogue/Logging.h>
#include <rogue/protocols/udp/Core.h>
#include <rogue/protocols/udp/Client.h>
#include <rogue/protocols/rssi/Client.h>
#include <rogue/protocols/rssi/Transport.h>
#include <rogue/protocols/rssi/Application.h>
#include <rogue/protocols/packetizer/CoreV2.h>
#include <rogue/protocols/packetizer/Transport.h>
#include <rogue/protocols/packetizer/Application.h>
#include <rogue/protocols/srp/SrpV3.h>
#include <rogue/interfaces/memory/Master.h>
#include <rogue/interfaces/memory/Constants.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/interfaces/stream/Buffer.h>
#include <rogue/hardware/axi/AxiStreamDma.h>

#include <fstream>
class RogueCom;
class RogueSender:  public rogue::interfaces::stream::Master {
 public:
 RogueSender(uint32_t port) : 
  tx_bytes(0),tx_max_pkt(0),tx_pkts(0),tx_min_pkt(0xffffffff),
  _port(port) {}
  void send(uint8_t *data, uint32_t size);
  uint32_t tx_bytes;
  uint32_t tx_max_pkt;
  uint32_t tx_pkts;
  uint32_t tx_min_pkt;
 private:
  uint32_t _port;
};

class RogueReceiver :  public rogue::interfaces::stream::Slave {
 public:
 RogueReceiver(uint32_t port,std::shared_ptr<RogueCom> com) :
  rx_bytes(0),rx_max_pkt(0),
    rx_pkts(0),rx_min_pkt(0xffffffff),
    _port(port),  _com(com) {}  
  void acceptFrame ( std::shared_ptr<rogue::interfaces::stream::Frame> frame);  
  uint32_t rx_bytes;
  uint32_t rx_max_pkt;
  uint32_t rx_pkts;
  uint32_t rx_min_pkt;
 private:
  uint32_t _port;
  std::shared_ptr<RogueCom> _com;
};


class RogueCom  {
 public:
 
  static std::shared_ptr<RogueCom> getInstance() {
    if(!instance) {
      instance=std::make_shared<RogueCom>();
    }
    return instance;
  }
  void connect(const std::string &conn,uint32_t port);
  virtual uint32_t getCurSize() ;
  virtual bool isEmpty();
  virtual uint32_t read32();
  virtual uint32_t readBlock32(uint32_t *buf, uint32_t length);
  virtual void write32(uint32_t);
  virtual void releaseFifo();	
  void enableLane(uint32_t mask);
  void enableDebugStream(bool enable);
  void reset() {
    writeRegister(rxPhyMon+0x808,1);
  }

  void enableTrig() {
     writeRegister(trigEmu+0x0,1);
  }

  bool trigBusy() {
    uint32_t temp;
     readRegister(trigEmu+0x14,temp);
    return ((temp&0x1)==0x1);

  }

  uint32_t readTLUtrigword() {
    uint32_t temp;
     readRegister(trigTLU+0x4,temp);
    return temp;

  }
  void setTrigEmu(uint32_t *trigWords,uint32_t length,uint32_t freq,uint32_t iter) {
    // fill LUT

  
    for(unsigned i=0;i<length;i++) {
      uint32_t temp=trigWords[length-1-i];
      if(bram[i] != temp) {
	writeRegister(trigLUT+i*4,temp);
      }
      bram[i]=temp;
    }
    if(length!=trigLength) writeRegister(trigEmu+0x8,length-1); // max LUT address
    if(iter!=trigIter) writeRegister(trigEmu+0xC,iter-1); // # iterations
    if(freq!=trigFreq) writeRegister(trigEmu+0x4,160000000/freq); // timer /160MHz    
    trigLength=length;
    trigIter=iter;
    trigFreq=freq;
  }
  void printStats() {
    printf("RX Stats\n");
    printf("Bytes Received:        %d\n",dataStream->rx_bytes);
    printf("Pkts Received:         %d\n",dataStream->rx_pkts);
    printf("Max Pkt Size(bytes):   %d\n",dataStream->rx_max_pkt);
    printf("Min Pkt Size(bytes):   %d\n",dataStream->rx_min_pkt);
    printf("TX Stats\n");
    printf("Bytes Sent:            %d\n",configStream->tx_bytes);
    printf("Pkts Sent:             %d\n",configStream->tx_pkts);
    printf("Max Pkt Size(bytes):   %d\n",configStream->tx_max_pkt);
    printf("Min Pkt Size(bytes):   %d\n",configStream->tx_min_pkt);

    
  }

  void setBatchSize(uint32_t sz) {
      uint32_t temp=sz&0xffff;
      writeRegister(sysReg+0x810,temp);
  }
  void setBatchTimer(uint32_t t) {
    uint32_t temp=t&0xffff;
    writeRegister(sysReg+0x80C,temp);
  }
  void queue_data(uint32_t *data,uint32_t nwords) {
    rx_lock.lock();
    for(unsigned i=0;i<nwords;i++) 
      rxfifo.push(data[i]);
    rx_lock.unlock();
  }
  uint32_t tx_size() {
    tx_lock.lock();
    uint32_t result= txfifo.size();
    tx_lock.unlock();
    return result;
  }
  void setFirmwareTrigger(bool enable) {
    firmwareTrigger=enable;
  }
  void setForceRelaseTxfifo(bool enable=true) {
	  forceRelaseTxfifo=enable;
  }
  bool getFirmwareTrigger() {return firmwareTrigger;}
 protected:
 private:	
  uint32_t m_counter;
  uint32_t rxPhyMon;
  uint32_t trigLUT;
  uint32_t trigTLU;
  uint32_t trigEmu;
  uint32_t sysReg;
  std::vector<uint32_t> txfifo;
  bool forceRelaseTxfifo;
  std::queue<uint32_t> rxfifo;
  std::mutex rx_lock;
  std::mutex tx_lock;
  rogue::interfaces::memory::MasterPtr mast;
  std::shared_ptr<RogueSender> configStream;
  std::shared_ptr<RogueReceiver> debugStream;
  std::shared_ptr<RogueReceiver> dataStream;
  rogue::hardware::axi::AxiStreamDmaPtr axisrp;
  rogue::hardware::axi::AxiStreamDmaPtr axicfg;
  rogue::hardware::axi::AxiStreamDmaPtr axidata;
  rogue::protocols::srp::SrpV3Ptr srp;
  rogue::protocols::udp::ClientPtr udp;
  rogue::protocols::rssi::ClientPtr rssi;
  rogue::protocols::packetizer::CoreV2Ptr pack;
  uint32_t trigLength;
  uint32_t trigFreq;
  uint32_t trigIter;
  uint32_t bram[1024];
  bool firmwareTrigger;
  static std::shared_ptr<RogueCom> instance;
  void writeRegister(uint32_t addr,const uint32_t &val) {
    uint32_t temp=val;
    mast->reqTransaction(addr,4,&temp,rogue::interfaces::memory::Write);
    mast->waitTransaction(0);
  }
  void readRegister(uint32_t addr,uint32_t &val) {
    uint32_t temp;
    mast->reqTransaction(addr,4,&temp,rogue::interfaces::memory::Read);
    mast->waitTransaction(0);
    val=temp;    
  }


  unsigned _port;
};

#endif
