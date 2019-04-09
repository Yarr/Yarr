#include "SimpleNetioTxCore.h"
#include "NetioTools.h"

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace std;
using namespace netio;

SimpleNetioTxCore::SimpleNetioTxCore(){
  m_enableMask = 0;
  m_trigEnabled = false;
  m_trigWordLength = 4;
  m_trigCnt = 0;
  m_trigFreq = 1;
  m_elinkSize = 4;
  m_padding = true;
  m_flip = true;
  m_verbose = false;
  m_debug = false;
  m_trigAbort = false;

  m_felixhost = "felix";
  m_felixport = 12340;

  m_context = new context("posix");
  m_socket = new low_latency_send_socket(m_context);
}

SimpleNetioTxCore::~SimpleNetioTxCore(){
  m_trigProc.join();
  if(m_socket->is_open()) m_socket->disconnect();
  delete m_socket;
  delete m_context;
}

void SimpleNetioTxCore::connect(){
  if(!m_socket->is_open()){
    try{
      m_socket->connect(netio::endpoint(m_felixhost,m_felixport));
      cout << "Connected to " << m_felixhost << ":" << m_felixport << endl;
    }catch(...){
      cout << "Cannot connect to " << m_felixhost << ":" << m_felixport << endl;
    }
  }
}

void SimpleNetioTxCore::enableChannel(uint64_t chn){
  tag elink = chn*2;
  m_elinks[elink]=chn;
  if(m_verbose) cout << "Enable TX elink: 0x" << hex << elink << dec << endl;
}

void SimpleNetioTxCore::disableChannel(uint64_t chn){
  tag elink = chn*2;
  m_elinks.erase(elink);
  //if(m_verbose) cout << "Disable TX elink: 0x" << hex << elink << dec << endl;
}

void SimpleNetioTxCore::setCmdEnable(uint32_t mask) {
  for(int chan=0; chan<32; chan++) {
    if((1<<chan) & mask) {
      enableChannel(chan);
    } else {
      disableChannel(chan);
    }
  }
}

uint32_t SimpleNetioTxCore::getCmdEnable() {
  uint32_t mask = 0;
  for(auto it=m_elinks.begin();it!=m_elinks.end();it++) {
    auto link = it->second;
    mask |= 1<<link;
  }
  return mask;
}

void SimpleNetioTxCore::writeFifo(uint32_t value){
  //auto start = std::chrono::high_resolution_clock::now();
  if(m_verbose){
    cout << "Add to FIFO: " << hex << setfill('0') << setw(8) << value << dec << endl;
  }
  if(m_elinkSize==1){
    for(int32_t b=3;b>=0;b--){
      m_fifo.push_back((value>>b*8)&0xFF);
    }
  }else if(m_elinkSize==4){
    for(int32_t b=3;b>=0;b--){
      for(int32_t i=0;i<4;i++){
        uint32_t val=(value>>((b+1)*8-i*2-2))&0x3;
        if     (val==0){m_fifo.push_back(0x00);}
        else if(val==1){m_fifo.push_back(0x0F);}
        else if(val==2){m_fifo.push_back(0xF0);}
        else if(val==3){m_fifo.push_back(0xFF);}
      }
    }
  }
  if(m_debug){printFifo();}
  //auto stop = std::chrono::high_resolution_clock::now();
  //auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(stop-start);
  //std::cout << "FIFO write in " << milliseconds.count() << "ms\n";
}

void SimpleNetioTxCore::releaseFifo(){
  releaseFifo(false);
}

void SimpleNetioTxCore::releaseFifo(bool trigChns){

  //try to connect
  connect();

  //auto start = std::chrono::high_resolution_clock::now();
  if(m_verbose) cout << "release FIFO:" << endl;

  if(m_fifo.size()==0){
    cout << "Roland FIFO is zero" << endl;
    m_fifo.clear();
    m_size.clear();
    m_data.clear();
  }

  //Padding to the first bit
  if(m_padding==true){
    uint32_t i0=0;
    for(uint32_t i=1; i<m_fifo.size(); i++){
      if(m_fifo[i]!=0){i0=i-1;break;}
    }
    for(uint32_t i=1; i<m_fifo.size(); i++){
      m_fifo[i]=m_fifo[i+i0];
    }
    if(m_debug){cout << "Remove first " << i0 << " characters" << endl; printFifo();}
    for(uint32_t i=0; i<i0; i++){
      m_fifo.pop_back();
    }
    if(m_debug){cout << "Pop back" << endl; printFifo();}
  }
  if(m_flip==true){
    if(m_fifo.size()%2==1){
      m_fifo.push_back(0);
    }
    for(uint32_t i=1; i<m_fifo.size(); i++){
      uint32_t tmp = m_fifo[i];
      m_fifo[i] = ((tmp&0xF0) >> 4) | ((tmp&0x0F)<<4);
    }
    if(m_debug){cout << "Flip half bytes: " << endl;printFifo();}
  }

  //create the message for NetIO
  std::map<netio::tag,uint32_t>::iterator it;

  if(trigChns){
    for(it=m_trigChns.begin();it!=m_trigChns.end();it++){
      m_headers[it->first].elinkid=it->first;
      m_headers[it->first].length=m_fifo.size();
      m_data.push_back((uint8_t*)&(m_headers[it->first]));
      m_size.push_back(sizeof(felix::base::ToFELIXHeader));
      m_data.push_back((uint8_t*)&m_fifo[0]);
      m_size.push_back(m_fifo.size());
      //for (uint32_t i=0; i<m_fifo.size(); i++){
      //  m_data.push_back((uint8_t*)&m_fifo[i]);
      //  m_size.push_back(1);
      //}
    }
  }else{
    for(it=m_elinks.begin();it!=m_elinks.end();it++){
      m_headers[it->first].elinkid=it->first;
      m_headers[it->first].length=m_fifo.size();
      m_data.push_back((uint8_t*)&(m_headers[it->first]));
      m_size.push_back(sizeof(felix::base::ToFELIXHeader));
      m_data.push_back((uint8_t*)&m_fifo[0]);
      m_size.push_back(m_fifo.size());	
      //for (uint32_t i=0; i<m_fifo.size(); i++){
      //m_data.push_back((uint8_t*)&m_fifo[i]);
      //m_size.push_back(1);
      //}
    }
  }
  message msg(m_data,m_size);
  //Send through the socket
  m_socket->send(msg);

  m_fifo.clear();
  m_size.clear();
  m_data.clear();
}

bool SimpleNetioTxCore::isCmdEmpty(){
  return (m_fifo.size()==0);
}

void SimpleNetioTxCore::setTrigEnable(uint32_t value){
  if(value == 0) {
	m_trigProc.join();
  } else {
	m_trigEnabled = true;
	switch (m_trigCfg) {
	case INT_TIME:
	case EXT_TRIGGER:
	  m_trigProc = std::thread(&SimpleNetioTxCore::doTriggerTime, this);
	  break;
	case INT_COUNT:
	  m_trigProc = std::thread(&SimpleNetioTxCore::doTriggerCnt, this);
	  break;
	default:
	  // Should not occur, else stuck
	  break;
	}
  }
}

uint32_t SimpleNetioTxCore::getTrigEnable(){
  return m_trigEnabled;
}

void SimpleNetioTxCore::maskTrigEnable(uint32_t value, uint32_t mask) {
  for(int chn=0; chn<32; chn++) {
    if(!((1<<chn) & mask)) continue;

    bool enable = (1<<chn) & value;
    tag elink = chn*2;
    if(enable) m_trigChns[elink]=chn;
    else m_trigChns.erase(elink);
  }
}

void SimpleNetioTxCore::toggleTrigAbort(){
  m_trigAbort = true;
}

bool SimpleNetioTxCore::isTrigDone(){
  if (!m_trigEnabled && m_fifo.size()==0){ return true; }
  return false;
}

void SimpleNetioTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg){
  m_trigCfg = cfg;
}

void SimpleNetioTxCore::setTrigFreq(double freq){
  m_trigFreq=freq;
}

void SimpleNetioTxCore::setTrigCnt(uint32_t count){
  m_trigCnt = count;
}

void SimpleNetioTxCore::setTrigTime(double time){
  m_trigTime = time;
}

void SimpleNetioTxCore::setTrigWordLength(uint32_t length){
  m_trigWordLength=length;
}

void SimpleNetioTxCore::setTrigWord(uint32_t *word, uint32_t size){
  m_trigWords.clear();
  for(uint32_t i=0;i<size;i++){m_trigWords.push_back(word[i]);}
}

void SimpleNetioTxCore::setTriggerLogicMask(uint32_t mask){
  //Nothing to do yet
}

void SimpleNetioTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode){
  //Nothing to do yet
}

void SimpleNetioTxCore::resetTriggerLogic(){
  //Nothing to do yet
}

uint32_t SimpleNetioTxCore::getTrigInCount(){
  return 0;
}

void SimpleNetioTxCore::doTriggerCnt() {
  for(uint32_t i=0; i<m_trigCnt; i++){
    if(m_trigAbort) break;
    std::lock_guard<std::mutex> lk(m_mutex);
    //cout << "Trigger:" << i << endl;
    for(int32_t j=m_trigWords.size()-1; j>=0;j--){
      writeFifo(m_trigWords[j]);
    }
    writeFifo(0x0);
    releaseFifo(true);
    //std::this_thread::sleep_for(std::chrono::microseconds((int)(10))); // Frequency in Hz
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq))); // Frequency in Hz
  }
  m_trigAbort = false;
  m_trigEnabled = false;
  if(m_verbose) cout << "finished trigger count" << endl;
}

void SimpleNetioTxCore::doTriggerTime() {
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point cur = std::chrono::steady_clock::now();
  while (std::chrono::duration_cast<std::chrono::seconds>(cur-start).count() < m_trigTime) {
    if(m_trigAbort) break;
    std::lock_guard<std::mutex> lk(m_mutex);
    for(int32_t j=m_trigWords.size()-1; j>=0;j--){
      writeFifo(m_trigWords[j]);
    }
    writeFifo(0x0);
    releaseFifo(true);
    //std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq))); // Frequency in Hz
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1000/m_trigFreq))); // Frequency in kHz
    cur = std::chrono::steady_clock::now();
  }
  m_trigAbort = false;
  m_trigEnabled = false;
  if(m_verbose) cout << "finished trigger time" << endl;
}

void SimpleNetioTxCore::printFifo(){
  cout << "FIFO: [" << m_fifo.size()-1 << "] " << endl << hex;
  for(uint32_t i=1; i<m_fifo.size(); i++){
    cout << setfill('0') << setw(2) << (uint32_t)(m_fifo[i]&0xFF);
  }
  cout << dec << endl;
}

void SimpleNetioTxCore::toFileJson(json &j) {
  j["NetIO"]["host"] = m_felixhost;
  j["NetIO"]["txport"] = m_felixport;
}

void SimpleNetioTxCore::fromFileJson(json &j){
   m_felixhost = j["NetIO"]["host"];
   m_felixport = j["NetIO"]["txport"];
}

