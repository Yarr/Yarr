#include "SimpleNetioRxCore.h"
#include <iomanip>
#include "felixbase/client.hpp"

using namespace std;
using namespace netio;

SimpleNetioRxCore::SimpleNetioRxCore(){
  m_t0 = std::chrono::steady_clock::now();
  m_datasize = 3;
  m_verbose = false;
  m_debug = false;
  string cntx = "posix";
  m_felixhost = "felix";
  m_felixport = 12345;
  m_bytesReceived = 0;
  m_context = new context(cntx);
  m_socket = new low_latency_subscribe_socket(m_context,[&](netio::endpoint& ep, netio::message& msg){decode(ep,msg);});

  /*m_statistics = thread([&](){
                          while (true) {
                            m_mutex.lock();
                            chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
                            double seconds = chrono::duration_cast<chrono::milliseconds>(t1-m_t0).count();
                            m_rate = m_bytesReceived/seconds;
                            if(seconds>3){
                              m_bytesReceived = 0;
                              m_t0=chrono::steady_clock::now();
                            }
                            m_mutex.unlock();
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                          }
                        });
  */

  m_bgthread = thread([&](){m_context->event_loop()->run_forever();});
}

SimpleNetioRxCore::~SimpleNetioRxCore(){
  map<tag,uint32_t>::iterator it;
  for(it=m_elinks.begin();it!=m_elinks.end();it++){
    cout << "Unsubscribe elink: 0x" << hex << it->first << dec << endl;
    m_socket->unsubscribe(it->first, endpoint(m_felixhost,m_felixport));
  }

  m_context->event_loop()->stop();
  m_bgthread.join();
  delete m_socket;
}

void SimpleNetioRxCore::decode(netio::endpoint& ep, netio::message& msg){
  std::vector<uint8_t> data = msg.data_copy();
  uint32_t offset=0;
  while(offset<data.size()){
    if(data.size()-offset<sizeof(felix::base::FromFELIXHeader)){
      cout << "SimpleNetioRxCore::decode msg too small:" << (data.size()-offset) << endl;
      break;
    }
    //Parse header
    felix::base::FromFELIXHeader header;
    memcpy(&header, (const void*)&data[offset], sizeof(header));
    offset+=sizeof(header);

    //extract channel id
    uint32_t chn=(header.elinkid>>1);
    if(header.gbtid<=1){chn+=header.gbtid*32;}
    //if(m_verbose) cout << "NetIO msg elinkid:" << header.elinkid << ", gbtid:" << header.gbtid << endl;

    //copy the data to the local buffers
    uint32_t numWords=header.length-sizeof(header);
    m_mutex.lock();
    m_bytesReceived += numWords;
    if(m_debug) cout << hex << "0x";
    for(uint32_t i=0; i<numWords; ++i){
      uint32_t v = data[offset];
      if(m_debug) cout << setw(2) << setfill('0') << v;
      m_data[chn].push(data[offset]);
      offset+=1;
    }
    if(m_debug) cout << endl;
    m_mutex.unlock();
  }
}

void SimpleNetioRxCore::enableChannel(uint64_t chn){
  tag elink = chn*2;
  if(m_elinks.find(elink)!=m_elinks.end()){return;}
  if(m_verbose) cout << "Enable RX elink: 0x" << hex << elink << dec << endl;
  try{
    cout << "Subscribe to elink: 0x" << hex << elink << dec << endl;
    m_socket->subscribe(elink, endpoint(m_felixhost,m_felixport));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    m_elinks[elink]=chn;
  }catch(...){
    cout << "Error subscribing to elink: 0x" << hex << elink << dec
         << " at " << m_felixhost << ":" << m_felixport << endl;
  }
}

void SimpleNetioRxCore::disableChannel(uint64_t chn){
  tag elink = chn*2;
  if(m_elinks.find(elink)==m_elinks.end()){return;}
  if(m_verbose) cout << "Disable RX elink: 0x" << hex << elink << dec << endl;
  try{
    cout << "Unsubscribe from elink: 0x" << hex << elink << dec << endl;
    m_socket->unsubscribe(elink, endpoint(m_felixhost,m_felixport));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }catch(...){
    cout << "Error unsubscribing to elink " << elink << " in "
         << m_felixhost << ":" << m_felixport << endl;
  }
  m_elinks.erase(elink);
}

void SimpleNetioRxCore::setRxEnable(uint32_t val) {
  for(int chan=0; chan<32; chan++) {
    if((1<<chan) & val) {
      enableChannel(chan);
    } else {
      disableChannel(chan);
    }
  }
}

void SimpleNetioRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
  for(int chan=0; chan<32; chan++) {
    if(!((1<<chan) & mask)) {
      continue;
    }

    if((1<<chan) & val) {
      enableChannel(chan);
    } else {
      disableChannel(chan);
    }
  }
}

RawData* SimpleNetioRxCore::readData(uint64_t chn){
  tag elink = chn*2;
  RawData * chnData = NULL;
  queue<uint8_t>* data = &m_data[elink];
  //uint32_t size = data->size()/m_datasize;
  float size = (float)data->size()/(float)m_datasize;
  //if(data->size()%m_datasize!=0){cout << "Problem with the data size=" << data->size() << " div=" << size << endl; }
  size=floor(size);
  if(size==0){return NULL;}
  uint32_t *buffer = new uint32_t[(int)size];
  for(uint32_t i=0; i<size;i++){
    buffer[i]=(chn<<25)&0xFF000000;
    if(m_debug) cout << "0x" << hex;
    for(uint32_t j=0; j<m_datasize;j++){
      m_mutex.lock();
      uint32_t v = data->front();
      data->pop();
      m_mutex.unlock();
      buffer[i] |= (v<<(8*(m_datasize-j-1)));
      if(m_debug) cout << setw(2) << setfill('0') << v;
    }
    if(m_debug) cout << " => 0x" << setw(6) << setfill('0') << buffer[i] << dec << endl;
  }
  chnData = new RawData(chn, buffer, size);
  return chnData;
}

RawData* SimpleNetioRxCore::readData() {
  map<tag,uint32_t>::iterator it;
  for(it=m_elinks.begin();it!=m_elinks.end();it++){
    RawData * chnData = readData(it->second);
    if(chnData)
      return chnData;
  }
  return nullptr;
}

uint32_t SimpleNetioRxCore::getDataRate(){
  return m_rate;
}

uint32_t SimpleNetioRxCore::getCurCount(){
  return 0;
}

bool SimpleNetioRxCore::isBridgeEmpty(){
  //return true;

  bool somedata=false;
  map<tag,queue<uint8_t> >::iterator it;
  for(it=m_data.begin();it!=m_data.end();it++){
    //m_mutex.lock();
    if(it->second.size()>0){somedata=true;}
    //m_mutex.unlock();
  }
  return somedata;

}

void SimpleNetioRxCore::toFileJson(json &j) {
  j["NetIO"]["host"] = m_felixhost;
  j["NetIO"]["rxport"] = m_felixport;
}

void SimpleNetioRxCore::fromFileJson(json &j) {
  m_felixhost = j["NetIO"]["host"];
  m_felixport = j["NetIO"]["rxport"];
}
