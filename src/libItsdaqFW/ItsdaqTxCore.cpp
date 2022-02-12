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
  m_trigWordLength = 0;
  m_trigEnabled = false;
  m_trigCnt = 0;
  m_trigFreq = 1;
}

ItsdaqTxCore::~ItsdaqTxCore(){
  if(m_trigProc.joinable()) m_trigProc.join();
}

// Activate single channel
void ItsdaqTxCore::setCmdEnable(uint32_t elink) {
  // FW Default is all enabled, leave this for now
  logger->trace("setCmdEnable: Multiple tx not yet implemented");
}

void ItsdaqTxCore::disableCmd() {
  logger->trace("disableCmd: Multiple tx not yet implemented");
}

// Broadcast to multiple channels
void ItsdaqTxCore::setCmdEnable(std::vector<uint32_t> channels) {
  logger->trace("setCmdEnable: Multiple tx not yet implemented");
}

uint32_t ItsdaqTxCore::getCmdEnable() {
  return 0;
}

void ItsdaqTxCore::buildSequenceWord(std::vector<uint16_t> &buffer,
                                     uint32_t LCB, uint32_t L1R3) {
  for(int b=0; b<16; b++) {
    unsigned int bit = 30-(b*2);
    uint16_t seq_word = 0;
    if(LCB & (1<<(bit+1)))
      seq_word |= 2;
    if(LCB & (1<<(bit+0)))
      seq_word |= 1;

    if(L1R3 & (1<<(bit+1)))
      seq_word |= 8;
    if(L1R3 & (1<<(bit+0)))
      seq_word |= 4;

    buffer.push_back(seq_word);
  }
}

void ItsdaqTxCore::writeFifo(uint32_t bit_data) {
  // Two bits in each sequence word
  logger->debug("WriteFifo: {:08x}", bit_data);

  // Just output idle
  buildSequenceWord(m_buffer, bit_data, 0x78557855);
}

void ItsdaqTxCore::releaseFifo(){
  // Translate from to 16bit buffer...
  m_h.SendOpcode((uint16_t)OPCODE::RAWSEQ, (uint16_t*)&m_buffer[0], (uint16_t)m_buffer.size());
  m_buffer.clear();
}

void ItsdaqTxCore::trigger(){
  logger->trace("Send trigger pattern");

  m_h.SendOpcode((uint16_t)OPCODE::RAWSEQ,
                 (uint16_t*)&m_trigBuffer[0], (uint16_t)m_trigBuffer.size());
}

bool ItsdaqTxCore::isCmdEmpty() {
  return m_buffer.empty();
}

void ItsdaqTxCore::setTrigEnable(uint32_t value){
  if(m_trigProc.joinable()) {
    m_trigProc.join();
  }
  if(value == 0) {
    m_trigEnabled = false;
  } else {
    m_trigEnabled = true;
    switch (m_trigCfg) {
    case INT_TIME:
    case EXT_TRIGGER:
      logger->debug("Starting trigger by time ({} seconds)", m_trigTime);
      m_trigProc = std::thread(&ItsdaqTxCore::doTriggerTime, this);
      break;
    case INT_COUNT:
      logger->debug("Starting trigger by count ({} triggers)", m_trigCnt);
      m_trigProc = std::thread(&ItsdaqTxCore::doTriggerCnt, this);
      break;
    default:
      // Should not occur, else stuck
      logger->error("No config for trigger, aborting loop");
      m_trigEnabled = false;
      break;
    }
  }
}

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
  if (!m_trigEnabled) {
    return true;
  }
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

void ItsdaqTxCore::buildTriggerSequence() {
  size_t trigLen = m_trigWordLength;
  auto words32 = std::min(trigLen, m_trigWords.size());

  m_trigBuffer.clear();

  for(int i=words32-1; i >= 0; i--) { //Need to send last word first
          buildSequenceWord(m_trigBuffer, m_trigWords[i], 0x78557855);
  }
}

void ItsdaqTxCore::setTrigWordLength(uint32_t length){
  m_trigWordLength=length;

  buildTriggerSequence();
}

void ItsdaqTxCore::setTrigWord(uint32_t *word, uint32_t size){
  m_trigWords.clear();
  for(uint32_t i=0;i<size;i++){m_trigWords.push_back(word[i]);}

  buildTriggerSequence();
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

void ItsdaqTxCore::doTriggerCnt() {
  uint32_t trigs=0;
  for(uint32_t i=0; i<m_trigCnt; i++) {
    if(m_trigEnabled==false) break;
    trigs++;
    logger->trace("Send trigger count {}", trigs);
    trigger();
    std::this_thread::sleep_for(std::chrono::microseconds((int)(1e6/m_trigFreq))); // Frequency in Hz
  }
  m_trigEnabled = false;
  logger->debug("Finished trigger count {}/{}", trigs, m_trigCnt);
}

void ItsdaqTxCore::doTriggerTime() {
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
  logger->debug("Finished trigger time {} with {} triggers",
                m_trigTime, trigs);
}

void ItsdaqTxCore::writeConfig(json &j)  {
}

void ItsdaqTxCore::loadConfig(const json &j){
  logger->debug("ItsdaqTxCore: No json config to load");

  // Does anything need changing if UDP socket updated?
  // init();
}
