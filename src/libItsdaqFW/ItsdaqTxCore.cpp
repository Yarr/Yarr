#include "ItsdaqTxCore.h"

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <chrono>

#include "ItsdaqHandler.h"

#include "logging.h"

namespace {
auto logger = logging::make_log("ItsdaqFW::TxCore");
}

enum class OPCODE : uint16_t {
  RAWSEQ = 0x0078,
};

ItsdaqTxCore::ItsdaqTxCore(ItsdaqHandler &h)
  : m_h(h)
{
#if 0
  //m_enableMask = 0;
  m_trigEnabled = false;
  m_trigWordLength = 4;
  m_trigCnt = 0;
  m_trigFreq = 1;

  m_verbose = false;
  m_debug = false;
#endif
}

ItsdaqTxCore::~ItsdaqTxCore(){
#if 0
  if(m_trigProc.joinable()) m_trigProc.join();
  if(m_socket->is_open()) m_socket->disconnect();
  delete m_socket;
  delete m_context;
#endif
}

// Activate single channel
void ItsdaqTxCore::setCmdEnable(uint32_t elink) {
    // this->disableAllChannels();
    // this->enableChannel(elink);
    // //m_elinks[elink]=true;
}

void ItsdaqTxCore::disableCmd() {
    // this->disableAllChannels();
}

// Broadcast to multiple channels
void ItsdaqTxCore::setCmdEnable(std::vector<uint32_t> channels) {
    // this->disableAllChannels();
    // for (uint32_t channel : channels) {
    //     this->enableChannel(channel);
    // }
    // return;
}

uint32_t ItsdaqTxCore::getCmdEnable() {
  return 0;
}

void ItsdaqTxCore::writeFifo(uint32_t bit_data) {
  // Two bits in each sequence word
  logger->debug("WriteFifo: {:08x}", bit_data);

  // Just output idle
  const uint32_t L1R3 = 0x78557855;

  for(int b=0; b<16; b++) {
    unsigned int bit = 30-(b*2);
    uint16_t seq_word = 0;
    if(bit_data & (1<<(bit+1)))
      seq_word |= 2;
    if(bit_data & (1<<(bit+0)))
      seq_word |= 1;

    if(L1R3 & (1<<(bit+1)))
      seq_word |= 8;
    if(L1R3 & (1<<(bit+0)))
      seq_word |= 4;

    m_buffer.push_back(seq_word);
  }
}

void ItsdaqTxCore::releaseFifo(){
  // Translate from to 16bit buffer...
  m_h.SendOpcode((uint16_t)OPCODE::RAWSEQ, (uint16_t*)&m_buffer[0], (uint16_t)m_buffer.size());
  m_buffer.clear();
}

#if 0
void ItsdaqTxCore::trigger(){
}
#endif

bool ItsdaqTxCore::isCmdEmpty() {
  return m_buffer.empty();
}

void ItsdaqTxCore::setTrigEnable(uint32_t value){
  if(value == 0) {
    // if(m_trigProc.joinable()) m_trigProc.join();
    m_trigEnabled = false;
  } else {
    m_trigEnabled = true;
  //   switch (m_trigCfg) {
  //   case INT_TIME:
  //   case EXT_TRIGGER:
  //     m_trigProc = std::thread(&ItsdaqTxCore::doTriggerTime, this);
  //     break;
  //   case INT_COUNT:
  //     m_trigProc = std::thread(&ItsdaqTxCore::doTriggerCnt, this);
  //     break;
  //   default:
  //     // Should not occur, else stuck
  //     break;
  //   }
  }

  //  m_trigEnabled = true;
}
// #endif

uint32_t ItsdaqTxCore::getTrigEnable(){
  return m_trigEnabled;
}

void ItsdaqTxCore::maskTrigEnable(uint32_t value, uint32_t mask) { //never used
  return;
}

void ItsdaqTxCore::toggleTrigAbort(){
  m_trigEnabled = false;
  //Abort trigger -> Empty the CircularBuffer + ensure stable 0 size?
}

bool ItsdaqTxCore::isTrigDone(){
  if (!m_trigEnabled && isCmdEmpty()){ return true; }
  return false;
}

void ItsdaqTxCore::setTrigConfig(enum TRIG_CONF_VALUE cfg){
  m_trigCfg = cfg;
}

void ItsdaqTxCore::setTrigFreq(double freq){
  m_trigFreq=freq;
}

void ItsdaqTxCore::setTrigCnt(uint32_t count){
  m_trigCnt = count;
}

void ItsdaqTxCore::setTrigTime(double time){
  m_trigTime = time;
}

void ItsdaqTxCore::setTrigWordLength(uint32_t length){
  m_trigWordLength=length;
}

void ItsdaqTxCore::setTrigWord(uint32_t *word, uint32_t size){
  m_trigWords.clear();
  for(uint32_t i=0;i<size;i++){m_trigWords.push_back(word[i]);}
}

void ItsdaqTxCore::setTriggerLogicMask(uint32_t mask){
  //Nothing to do yet
}

void ItsdaqTxCore::setTriggerLogicMode(enum TRIG_LOGIC_MODE_VALUE mode){
  //Nothing to do yet
}

void ItsdaqTxCore::resetTriggerLogic(){
  //Nothing to do yet
}

uint32_t ItsdaqTxCore::getTrigInCount(){
  return 0;
}

#if 0
void ItsdaqTxCore::doTriggerCnt() {
  prepareTrigger();

  uint32_t trigs=0;
  for(uint32_t i=0; i<m_trigCnt; i++) {
    if(m_trigEnabled==false) break;
    trigs++;
    trigger();
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq))); // Frequency in Hz
  }
  m_trigEnabled = false;
  if(m_verbose) cout << "finished trigger count" << endl;
  if(m_debug){
    cout << endl
         << "============> Finish trigger with n counts: " << trigs << endl
         << endl;
  }
}

void ItsdaqTxCore::doTriggerTime() {
  //PrepareTrigger
  prepareTrigger();

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point cur = start;
  uint32_t trigs=0;
  while (std::chrono::duration_cast<std::chrono::seconds>(cur-start).count() < m_trigTime) {
    if(m_trigEnabled==false) break;
    trigs++;
    trigger();
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1000/m_trigFreq))); // Frequency in kHz
    cur = std::chrono::steady_clock::now();
  }
  m_trigEnabled = false;
  if(m_verbose) cout << "finished trigger time" << endl;
  if(m_debug){
    cout << endl
         << "============> Finish trigger with n counts: " << trigs << endl
         << endl;
  }
}

void ItsdaqTxCore::printFifo(uint32_t elink){
  cout << "FIFO[" << elink << "][" << m_fifo[elink].size()-1 << "]: " << hex;
  for(uint32_t i=1; i<m_fifo[elink].size(); i++){
    std::cout << setfill('0') << setw(2) << (m_fifo[elink][i]&0xFF);
  }
  std:cout << dec << endl;
}
#endif

void ItsdaqTxCore::toFileJson(json &j)  {
}

void ItsdaqTxCore::fromFileJson(json &j){
  logger->debug("ItsdaqTxCore: No json config to load");
}
