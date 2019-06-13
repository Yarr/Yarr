#include "NetioRxCore.h"
#include "NetioTools.h"

#include <iomanip>
#include "felixbase/client.hpp"

using namespace std;
using namespace netio;

NetioRxCore::NetioRxCore()
  : m_nioh("posix", "localhost", 12340, 12345, 50000000, true)
{
  m_t0 = std::chrono::steady_clock::now();
  string cntx = "posix";
  m_felixhost = "localhost";
  m_felixport = 12345;
  m_bytesReceived = 0;
  m_context = new context(cntx);
  m_cont = true;
  m_verbose = false;
  rxDataCount = 0; // initialize to zero data received 

  m_statistics = thread([&](){
    while (m_cont) {
      std::lock_guard<std::mutex> lock(m_mutex);
      chrono::steady_clock::time_point t1 = chrono::steady_clock::now();
      double seconds = chrono::duration_cast<chrono::milliseconds>(t1-m_t0).count();
      m_rate = m_bytesReceived/seconds;
      if(seconds>3){
        m_bytesReceived = 0;
        m_t0=chrono::steady_clock::now();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Note: is this needed; can it be shortened?
    }
  });

  m_verbose = false;
}

NetioRxCore::~NetioRxCore(){
  m_nioh.stopChecking();
  m_cont=false;
  map<uint64_t,bool>::iterator it;
  for(it=m_elinks.begin();it!=m_elinks.end();it++){
    cout << "Unsubscribe elink: 0x" << hex << it->first << dec << endl;
    m_nioh.delChannel(it->first);
    //FIXME: //m_socket->unsubscribe(it->first, endpoint(m_felixhost,m_felixport));
    //m_sockets[it->first]->unsubscribe(it->first, endpoint(m_felixhost,m_felixport));
    //m_queues[it->first]->~MonitoredQueue();
    //delete m_sockets[it->first];
  }
  m_statistics.join();
}

void NetioRxCore::enableChannel(uint64_t elink){
  if(m_verbose) cout << "Enable RX elink: 0x" << hex << elink << dec << endl;
  if(m_elinks.find(elink)==m_elinks.end()){
    m_nioh.addChannel(elink);
  }
  m_elinks[elink]=true;
}

void NetioRxCore::disableChannel(uint64_t elink){
  if(m_verbose) cout << "Disable RX elink: 0x" << hex << elink << dec << endl;
  //m_nioh.stopChecking();
  //m_elinks[elink]=false;
  // We don't disable channels...
}

void NetioRxCore::setRxEnable(uint32_t val) {
  for(int chan=0; chan<200; chan++) {
    //if((1<<chan) & val) {
    if (chan == 128) {
      enableChannel(chan);
    } else {
      disableChannel(chan);
    }
  }
}

void NetioRxCore::maskRxEnable(uint32_t val, uint32_t mask) {
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

void NetioRxCore::flushBuffer(){
    // This function is intended to remove junk data received after
    // an ECR command is sent
    m_nioh.setFlushBuffer(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    m_nioh.setFlushBuffer(false);

    // seems to need more time in the beginning 
    static int loop_count = 0;
    if (loop_count < 2){
        std::this_thread::sleep_for(std::chrono::seconds(4));
        ++loop_count;
    }

}

RawData* NetioRxCore::readData(){
  // Loop over all links looking for data
  // Return the first one we find (slow?)

  //map<uint64_t,bool>::iterator it;
  //for(it=m_elinks.begin();it!=m_elinks.end();it++){    // For every channel's queue:
    //if(!it->second) continue;
    //uint64_t elink=it->first;

    if(m_verbose) cout << "NetioRxCore::readData()"<<endl; //  elink number " << elink << endl;

    std::unique_ptr<RawData> rdp = m_nioh.rawData.popData();
    if(rdp != NULL){
	auto buffer = rdp.get()->buf;
	auto address = rdp.get()->adr;
	auto words = rdp.get()->words;

	RawData* new_rdp = new RawData(address, buffer, words);

        if(m_fetype == "rd53a"){
            //TODO:fix this in firmware; the header needs to be in buffer[0]
            uint32_t temp;
            temp = buffer[0];
            buffer[0] = buffer[1];
            buffer[1] = temp;

            //TODO: Fix this  in firmware too
            if( (buffer[words-2]>>16) == 0x0 ) //To deal with the E-frames fr$
                buffer[words-2] = 0xFFFF;

            //TODO: Fix this  in firmware too
            if( (buffer[words-1]>>16) == 0x0 ) //To deal with the E-frames fr$
                buffer[words-1] = 0xFFFF;
        }
        ++rxDataCount;
	return new_rdp;

  }
  return NULL;
}

uint32_t NetioRxCore::getDataRate(){
  return m_rate;
}

uint32_t NetioRxCore::getCurCount(){
  std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
  return m_nioh.getDataCount() - rxDataCount;
}

bool NetioRxCore::isBridgeEmpty(){ // True, if queues are stable.
  return m_nioh.isAllStable();
}

void NetioRxCore::toFileJson(json &j) {
  j["NetIO"]["host"] = m_felixhost;
  j["NetIO"]["rxport"] = m_felixport;
}

void NetioRxCore::fromFileJson(json &j) {
  m_felixhost = j["NetIO"]["host"];
  m_felixport = j["NetIO"]["rxport"];
  m_fetype = j["NetIO"]["fetype"];
  m_nioh.setFeType(m_fetype);
  m_nioh.setFelixHost(m_felixhost);
  m_nioh.setFelixRXPort(m_felixport);
}

