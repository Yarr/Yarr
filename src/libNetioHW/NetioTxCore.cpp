#include "NetioTxCore.h"

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <chrono>

#include "logging.h"

using namespace std;
using namespace netio;

namespace {
auto nlog = logging::make_log("NetioHW::TxCore");
}

NetioTxCore::NetioTxCore()
{
  //m_enableMask = 0;
  m_trigEnabled = false;
  m_trigWordLength = 4;
  m_trigCnt = 0;
  m_trigFreq = 1;

  //FEI4-related settings
  m_extend = 4;
  m_padding = false;
  m_flip = false;
  m_manchester = false;
  m_felixhost = "localhost";
  m_felixport = 12340;

  m_context = new context("posix");
  m_socket = new low_latency_send_socket(m_context);
  m_pixFwTrigger = false;
  m_bufferSize = 0;

}

NetioTxCore::~NetioTxCore(){
  if(m_trigProc.joinable()) m_trigProc.join();
  if(m_socket->is_open()) m_socket->disconnect();
  delete m_socket;
  delete m_context;
}

void NetioTxCore::connect(){
  if(!m_socket->is_open()){
    try{
      m_socket->connect(netio::endpoint(m_felixhost,m_felixport));
      nlog->info("Connected to {}:{}", m_felixhost, m_felixport);
    }catch(...){
      nlog->info("Cannot connect to {}:{}", m_felixhost, m_felixport);
    }
  }
}

void NetioTxCore::enableChannel(uint32_t elink){
  nlog->debug("Enable TX elink: 0x{:x}", elink);
  m_elinks[elink]=true;
}

void NetioTxCore::disableChannel(uint32_t elink){
  nlog->debug("Disable TX elink: 0x{:x}", elink);

  m_elinks[elink]=false;

  return;
}

void NetioTxCore::disableAllChannels() {
    for (const auto it: m_elinks) {
        m_elinks[it.first] = false;
        nlog->debug("disabling channel: {}", it.first);
    }
}

// Activate single channel
void NetioTxCore::setCmdEnable(uint32_t elink) {
    this->disableAllChannels();
    this->enableChannel(elink);
    //m_elinks[elink]=true;
}

void NetioTxCore::disableCmd() {
    this->disableAllChannels();
}

// Broadcast to multiple channels
void NetioTxCore::setCmdEnable(std::vector<uint32_t> channels) {
    this->disableAllChannels();
    for (uint32_t channel : channels) {
        this->enableChannel(channel);
    }
    return;
}

uint32_t NetioTxCore::getCmdEnable() {
  return 0;
}

void NetioTxCore::writeFifo(uint32_t value){

  nlog->trace("NetioTxCore::writeFifo val={:08x}", value);

  for(const auto it : m_elinks)
    //check if the e-link is active, and if so write on correspoding FIFO
    if(it.second) {
      nlog->trace("it.first: {}, it.second: {}",
                  it.first, it.second);
      writeFifo(it.first,value);
    }
}

void NetioTxCore::writeFifo(uint32_t elink, uint32_t value){

  nlog->trace("NetioTxCore::writeFifo elink={} val=0x{:08x}", elink, value);

  if(m_elinks[elink] == false) {
        nlog->warn("WARNING: The e-link is disabled! Can not write fifo");
	return;
  }

  writeFifo(&m_fifo[elink],value);
}

void NetioTxCore::writeFifo(vector<uint8_t> *fifo, uint32_t value) const{

  if(m_extend==4){
    for(int32_t b=3;b>=0;b--){
      for(int32_t i=0;i<4;i++){
        uint32_t val=(value>>((b+1)*8-i*2-2))&0x3;
        if     (val==0){fifo->push_back(0x00);}
        else if(val==1){fifo->push_back(0x0F);}
        else if(val==2){fifo->push_back(0xF0);}
        else if(val==3){fifo->push_back(0xFF);}
      }
    }
  }else{
    for(int32_t b=3;b>=0;b--){
      fifo->push_back((value>>b*8)&0xFF);
    }
  }
}

void NetioTxCore::prepareFifo(vector<uint8_t> *fifo) const{

  if(m_padding==true){
    nlog->trace("Padding");
    uint32_t i0=0;
    nlog->trace("Find the first byte");
    for(uint32_t i=1; i<fifo->size(); i++){
      if(fifo->at(i)!=0){i0=i-1;break;}
    }
    nlog->trace("Copy the array forward");
    for(uint32_t i=0; i<fifo->size()-i0; i++){
      fifo->at(i)=fifo->at(i+i0);
    }
    nlog->trace("Remove first {} characters", i0);
    for(uint32_t i=0; i<i0; i++){
      fifo->pop_back();
    }
    nlog->trace("Pop back");
  }

  if(m_flip==true){
    nlog->trace("Flipping");
    if(fifo->size()%2==1){
      fifo->push_back(0);
    }
    for(uint32_t i=0; i<fifo->size(); i++){//was i=1
      uint32_t tmp = fifo->at(i);
      fifo->at(i) = ((tmp&0xF0) >> 4) | ((tmp&0x0F)<<4);
    }
  }

  if(m_manchester==true){
    nlog->trace("Manchester");
    bool clk=true;
    fifo->insert(fifo->begin(),2,0x0); //16 extra leading zeroes
    for(uint32_t i=0; i<fifo->size(); i++){
      uint32_t tmp = fifo->at(i);
      fifo->at(i) = (tmp&0xCC && clk) | (tmp&0x33 && !clk);
    }
  }
}

void NetioTxCore::sendFifo(){

  //try to connect
  connect();

  nlog->trace("NetioTxCore::sendFifo");

  for(const auto it : m_elinks)
    //check if e-link is active and, if so,
    // prepare FIFO for sending and flush if appropriate
    if(it.second){
        auto elink = it.first;
        auto &this_fifo = m_fifo[elink];
    	prepareFifo(&this_fifo);
        nlog->trace("FIFO[{}][{}]: ", elink, this_fifo.size()-1);
        for(uint32_t i=1; i<this_fifo.size(); i++){
          nlog->trace("{:02x}", this_fifo[i]&0xFF);
        }

    	m_headers[elink].elinkid=elink;
    	m_headers[elink].length=this_fifo.size();
    	m_data.push_back((uint8_t*)&(m_headers[elink]));
    	m_size.push_back(sizeof(felix::base::ToFELIXHeader));
    	m_data.push_back((uint8_t*)&(this_fifo[0]));
    	m_size.push_back(this_fifo.size());	
    }

  message msg(m_data,m_size);
  //Send through the socket
  m_socket->send(msg);

  for(const auto it : m_elinks){
    if(it.second==false) continue;
    m_fifo[it.first].clear();
  }

  m_size.clear();
  m_data.clear();

  return;
}

void NetioTxCore::releaseFifo(){ 

  nlog->trace("NetioTxCore::releaseFifo"); 
  
  int buffer_size = 0;

  for(const auto it : m_elinks){
    //check if e-link is active and, if so,
    // prepare FIFO for flushing if conditions are met.
    if(it.second){
      auto elink = it.first;
      auto &this_fifo = m_fifo[elink];
      buffer_size += this_fifo.size(); //total size of buffer from all active elinks
    }
  }

  if(buffer_size>m_bufferSize){
    sendFifo();
  }

  return;

}   

void NetioTxCore::trigger(){

  //try to connect
  connect();

  map<tag,felix::base::ToFELIXHeader> headers;
  vector<const uint8_t*> data;
  vector<size_t> size;

  nlog->trace("NetioTxCore::trigger");

  //create the message for NetIO
  for(const auto it : m_elinks)
    //loop over active e-links
    if(it.second){
    	//prepareFifo(&m_trigFifo[it.first]);
    	headers[it.first].elinkid=it.first;
    	headers[it.first].length=m_trigFifo[it.first].size();
    	data.push_back((uint8_t*)&(headers[it.first]));
    	size.push_back(sizeof(felix::base::ToFELIXHeader));
    	data.push_back((uint8_t*)&m_trigFifo[it.first][0]);
    	size.push_back(m_trigFifo[it.first].size());	
    }

  message msg(data,size);

  //Send through the socket
  m_socket->send(msg);

  //the trigger fifo is emptied somewhere else
  //for(it=m_elinks.begin();it!=m_elinks.end();it++){
    //if(it.second)
    	//m_trigFifo[it.first].clear();
  //}

  size.clear();
  data.clear();

}

bool NetioTxCore::isCmdEmpty(){

  //Check if buffer is empty.
  //Regardless, empty the FIFO in case any data is still there, but
  // return the status of the FIFO before emptying it;
  // this allows the user to know that the FIFO had something in it
  // and decide if to wait a bit or just call again immediately 
  // isCmdEmpty(), that will at that point return true.

  bool is_buffer_empty = true;
  for(const auto it : m_elinks)
    if(it.second)
      if(!m_fifo[it.first].empty()) 
	is_buffer_empty = false;

  if (not is_buffer_empty){
    sendFifo();
  }

  return is_buffer_empty;
}

void NetioTxCore::setTrigEnable(uint32_t value){
  if(value == 0) {
    if(m_trigProc.joinable()) m_trigProc.join();
  } else {
    m_trigEnabled = true;
    switch (m_trigCfg) {
    case INT_TIME:
    case EXT_TRIGGER:
      m_trigProc = std::thread(&NetioTxCore::doTriggerTime, this);
      break;
    case INT_COUNT:
      m_trigProc = std::thread(&NetioTxCore::doTriggerCnt, this);
      break;
    default:
      // Should not occur, else stuck
      break;
    }
  }
}

uint32_t NetioTxCore::getTrigEnable(){
  return m_trigEnabled;
}

void NetioTxCore::maskTrigEnable(uint32_t value, uint32_t mask) { //never used
  return;
}

void NetioTxCore::toggleTrigAbort(){
  m_trigEnabled = false;
  //Abort trigger -> Empty the CircularBuffer + ensure stable 0 size?
}

bool NetioTxCore::isTrigDone(){
  if (!m_trigEnabled && isCmdEmpty()){ return true; }
  return false;
}

void NetioTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg){
  m_trigCfg = cfg;
}

void NetioTxCore::setTrigFreq(double freq){
  m_trigFreq=freq;
}

void NetioTxCore::setTrigCnt(uint32_t count){
  m_trigCnt = count;
}

void NetioTxCore::setTrigTime(double time){
  m_trigTime = time;
  if(m_trigCfg==1)
    setTrigCnt((float)time*(float)m_trigFreq);
}

void NetioTxCore::setTrigWordLength(uint32_t length){
  m_trigWordLength=length;
}

void NetioTxCore::setTrigWord(uint32_t *word, uint32_t size){
  m_trigWords.clear();
  for(uint32_t i=0;i<size;i++){m_trigWords.push_back(word[i]);}
}

void NetioTxCore::setTriggerLogicMask(uint32_t mask){
  //Nothing to do yet
}

void NetioTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode){
  //Nothing to do yet
}

void NetioTxCore::resetTriggerLogic(){
  //Nothing to do yet
}

uint32_t NetioTxCore::getTrigInCount(){
  return 0;
}

void NetioTxCore::prepareTrigger(){
  for(auto it : m_elinks){
    m_trigFifo[it.first].clear();

    // send a sync to make sure the following commands are not interrrupted for a while
    //MT
    //For ITk pixel RM 5.0 firmware
    if(m_pixFwTrigger){ //special 16b character in the F/W = {1110, #iteration (7b), frequency(5b)
      uint32_t trigFreq_ratio = (40000000/m_trigFreq)/256; //40 Mhz/m_trigFreq(Hz) and /256 as F/W can in/decrease frequency only in multiple of 128

      if(trigFreq_ratio > 31) {cerr<<"m_trigFreq "<<m_trigFreq<<" not supported by the F/W. Supported frequency is >= 9.8 kHz"<<endl; exit(1);} //9.8 is wrong
      if(trigFreq_ratio == 0) {cerr<<"m_trigFreq "<<m_trigFreq<<" not supported by the F/W. Supported frequency is <~ 156 kHz"<<endl; exit(1);}
      if(m_trigCnt > 127)     {cerr<<"m_trigCnt "<<m_trigCnt<<" not supported by the F/W. Supported range is 1 to 127"<<endl; exit(1);}
				
      uint32_t calinj_char = 0x817e<<16 | (0xE<<12 & 0xF000) | (m_trigCnt<<5 & 0xFE0) | (trigFreq_ratio & 0x1F);
      //      printf("m_trigCnt=%d, m_trigFreq=%d, calinj_char=\%08x \n", m_trigCnt, m_trigFreq, calinj_char);
      writeFifo(&m_trigFifo[it.first],calinj_char);
    }
    else{
      if (m_feType == "rd53a")
        writeFifo(&m_trigFifo[it.first],0x817e817e);    

      for(int32_t j=m_trigWords.size()-1; j>=0;j--){
	writeFifo(&m_trigFifo[it.first],m_trigWords[j]);
      }
    }
    //writeFifo(&m_trigFifo[it.first],0x0); //Waste!
    prepareFifo(&m_trigFifo[it.first]);
  }
}

void NetioTxCore::doTriggerCnt() {

  prepareTrigger();

  using clk = chrono::steady_clock;

  clk::time_point last_trigger = clk::now();

  const auto delta = std::chrono::nanoseconds((int64_t)(1e9/m_trigFreq));

  uint32_t trigs=0;
  if (m_trigEnabled) {
    if (m_pixFwTrigger){
      // send a single command that will start the firmware-based trigger sequence
      trigs=m_trigCnt;
      trigger();
    }
    else{ //
      for(uint32_t i=0; i<m_trigCnt; i++) {
	if(m_trigEnabled==false) break;
	trigs++;
	trigger();
	last_trigger += delta;
	std::this_thread::sleep_until(last_trigger);
      }
    }
  }
  m_trigEnabled = false;

  nlog->debug("finished trigger count");
  nlog->trace("============> Finish trigger with n counts: {}");
}

void NetioTxCore::doTriggerTime() {

  //PrepareTrigger
  prepareTrigger();

  chrono::steady_clock::time_point start = chrono::steady_clock::now();
  chrono::steady_clock::time_point cur = start;

  const auto delta = std::chrono::nanoseconds((int64_t)(1e9/m_trigFreq));

  uint32_t trigs=0;

  if (m_trigEnabled) {
    if (m_pixFwTrigger){
      trigs=m_trigCnt;
      trigger();
    }
    else{
      while (chrono::duration_cast<chrono::seconds>(cur-start).count() < m_trigTime) {
	if(m_trigEnabled==false) break;
	trigs++;
	trigger();
	cur += delta;
	std::this_thread::sleep_until(cur);
      }
    }
  }
  m_trigEnabled = false;
  nlog->debug("finished trigger time");
  nlog->trace("============> Finish trigger with n counts: {}", trigs);
}

void NetioTxCore::printFifo(uint32_t elink){
  cout << "FIFO[" << elink << "][" << m_fifo[elink].size()-1 << "]: " << hex;
  for(uint32_t i=1; i<m_fifo[elink].size(); i++){
    std::cout << setfill('0') << setw(2) << (m_fifo[elink][i]&0xFF);
  }
  std:cout << dec << endl;
}

void NetioTxCore::writeConfig(json &j)  {
  j["NetIO"]["host"] = m_felixhost;
  j["NetIO"]["txPort"] = m_felixport;
  j["NetIO"]["manchester"] = m_manchester;
  j["NetIO"]["flip"] = m_flip;
  j["NetIO"]["extend"] = (m_extend == 4);
  j["NetIO"]["bufferSize"] = m_bufferSize;
  j["NetIO"]["pixFwTrigger"] = m_pixFwTrigger;
}

void NetioTxCore::loadConfig(const json &j){
   if (j["NetIO"].contains("host")) m_felixhost  = j["NetIO"]["host"];
   if (j["NetIO"].contains("txPort")) m_felixport  = j["NetIO"]["txPort"];
   if (j["NetIO"].contains("manchester")) m_manchester = j["NetIO"]["manchester"];
   if (j["NetIO"].contains("flip")) m_flip       = j["NetIO"]["flip"];
   if (j["NetIO"].contains("extend")) m_extend     = (j["NetIO"]["extend"]?4:1);
   if (j["NetIO"].contains("feType")) m_feType     = j["NetIO"]["feType"];

   if(j["NetIO"].contains("bufferSize")){
     m_bufferSize = j["NetIO"]["bufferSize"];
     nlog->info(" bufferSize={}", m_bufferSize);
   }

   if(j["NetIO"].contains("pixFwTrigger")){
     m_pixFwTrigger = j["NetIO"]["pixFwTrigger"];
     nlog->info(" pixFwTrigger={}", m_pixFwTrigger);
   }

   nlog->info("NetioTxCore:");
   nlog->info(" manchester={}", m_manchester);
   nlog->info(" flip={}", m_flip);
   nlog->info(" extend={}", m_extend);
   nlog->info(" feType={}", m_feType);
}
