#include "StarFelixTriggerLoop.h"
#include "LCBUtils.h"
#include "LCBFwUtils.h"

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarFwTriggerLoop");
}

StarFelixTriggerLoop::StarFelixTriggerLoop() : LoopActionBase(LOOP_STYLE_TRIGGER) {
  setTrigCnt(50); // Maximum number of triggers to send
  min = 0;
  max = 0;
  step = 1;
  loopType = typeid(this);
}

void StarFelixTriggerLoop::init() {
  m_done = false;
  logger->debug("init");

  // Get the list of enabled channels for sending LCB commands
  std::vector<uint32_t> lcb_elinks = keeper->getTxMaskUnique();

  // Channels for writing trickle memory
  std::vector<uint32_t> trickle_elinks;

  // Channels for sending LCB configuration
  std::vector<uint32_t> config_elinks;

  for (uint32_t el : lcb_elinks) {
    // FELIX Phase 2 FW spec (v1.007) Table 8.30 
    trickle_elinks.push_back( el+1 );
    config_elinks.push_back( el-1 );
  }

  logger->debug("Write trigger sequence to the trickle memory");
  LCB_FELIX::write_trickle_memory(
    *g_tx,
    config_elinks,
    trickle_elinks,
    makeTrickleSequence()
    );

  setTrigWord();

  logger->debug("Configure TxCore");
  g_tx->setTrigWord(&m_trigWord[0], m_trigWordLength);
  g_tx->setTrigWordLength(m_trigWordLength);

  // The TrigFreq here determines the interval between TRICKLE_TRIGGER_PULSE
  // Hard code to 1 kHz for now. Need to check this.
  g_tx->setTrigFreq(1000);

  // The TrigCnt is the number of time to loop over the trickle memory
  uint32_t nPulse = getTrigCnt() / m_nTrigsTrickle;
  // m_nTrigsTrickle is determined in makeTrickleSequence()

  // The actual number of triggers is equal to or greater than what is requested
  if (getTrigCnt() % m_nTrigsTrickle) { 
    nPulse += 1;
  }

  g_tx->setTrigCnt(nPulse);

  unsigned nTotalTrigs = nPulse * m_nTrigsTrickle;

  if (nTotalTrigs != getTrigCnt()) {
    logger->warn("The actual number of triggers sent is set to {}", nTotalTrigs);

    // Update the trigger counts to what is actually sent for analysis later
    setTrigCnt(nTotalTrigs);
  }

  g_tx->setTrigTime(m_trigTime);

  if (getTrigCnt() > 0) {
    g_tx->setTrigConfig(INT_COUNT);
  } else {
    g_tx->setTrigConfig(INT_TIME);
  }

  // Enable the LCB configuration elinks for setting TRICKLE_TRIGGER_PULSE
  g_tx->setCmdEnable(config_elinks);

  while(!g_tx->isCmdEmpty());
}

void StarFelixTriggerLoop::execPart1() {
  logger->debug("execPart1");
  // Enable Trigger
  g_tx->setTrigEnable(0x1);
}

void StarFelixTriggerLoop::execPart2() {
  logger->debug("execPart2");
  while(!g_tx->isTrigDone());
  // Disable Trigger
  g_tx->setTrigEnable(0x0);
  m_done = true;
}

void StarFelixTriggerLoop::end() {
  logger->debug("end");

  // Go back to general state of FE, do something here (if needed)
  g_tx->setCmdEnable(keeper->getTxMask());

  while(!g_tx->isCmdEmpty());
}

void StarFelixTriggerLoop::setTrigWord() {
  // The LCB trigger commands are stored in the trickle memory
  // What will be sent from TxCore is the TRICKLE_TRIGGER_PULSE
  m_trigWord[0] = LCB_FELIX::config_command(LCB_FELIX::TRICKLE_TRIGGER_PULSE, 1);
  m_trigWordLength = 1;
}

void StarFelixTriggerLoop::writeConfig(json &config) {
  config["trig_count"] = getTrigCnt();
  config["trig_frequency"] = m_trigFreq;
  config["trig_time"] = m_trigTime;
  config["l0_latency"] = m_trigDelay;
  config["noInject"] = m_noInject;
  config["digital"] = m_digital;
}

void StarFelixTriggerLoop::loadConfig(const json &config) {

  if (config.contains("trig_count"))
    setTrigCnt(config["trig_count"]);

  if (config.contains("trig_frequency"))
    m_trigFreq = config["trig_frequency"];

  if (config.contains("trig_time"))
    m_trigTime = config["trig_time"];

  if (config.contains("l0_latency"))
    m_trigDelay = config["l0_latency"];

  if (config.contains("noInject"))
    m_noInject = config["noInject"];

  if (config.contains("digital"))
    m_digital = config["digital"];

  logger->info("Configured trigger loop: trig_count: {} trig_frequency: {} l0_delay: {}", getTrigCnt(), m_trigFreq, m_trigDelay);
}

std::tuple<std::vector<uint8_t>, unsigned> StarFelixTriggerLoop::getTriggerSegment() {

  std::vector<uint8_t> triggers;
  unsigned nTriggers; // number of triggers in triggers;

  if (m_trigFreq < 10e6) { // 10 MHz
    // One trigger per L0A
    auto l0a = LCB_FELIX::l0a_mask(1, false);

    // Number of LCB frames between each L0A
    // One frame covers 4 BCs = 100 ns
    int nFrames = (1e9 / m_trigFreq / 100);

    // nFrames-1 IDLEs, followed by an L0A
    triggers.assign(nFrames-1, LCB_FELIX::IDLE);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());

    nTriggers = 1;

  } else if (m_trigFreq < 15e6) { // 10 ~ 15 MHz
    logger->info("Effective trigger rate is set to: 13.33 MHz");

    auto l0a_0 = LCB_FELIX::l0a_mask(0b0010, false);
    auto l0a_1 = LCB_FELIX::l0a_mask(0b0100, false);
    auto l0a_2 = LCB_FELIX::l0a_mask(0b1001, false);

    triggers.insert(triggers.end(), l0a_0.begin(), l0a_0.end());
    triggers.insert(triggers.end(), l0a_1.begin(), l0a_1.end());
    triggers.insert(triggers.end(), l0a_2.begin(), l0a_2.end());

    nTriggers = 4;

  } else if (m_trigFreq < 30e6) { // 15 ~ 30 MHz
    logger->info("Effective trigger rate is set to: 20 MHz");

    auto l0a = LCB_FELIX::l0a_mask(0b0101);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());

    nTriggers = 2;

  } else { // > 30 MHz
    logger->info("Effective trigger rate is set to: 40 MHz");

    auto l0a = LCB_FELIX::l0a_mask(0b1111);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());

    nTriggers = 4;
  }

  return {triggers, nTriggers};
}

std::vector<uint8_t> StarFelixTriggerLoop::getHitCounterSegment() {
  std::vector<uint8_t> readHitCounts;

  // Broadcast read commands for hit counter registers
  // TODO:
  // In case we wish to mask some FEs, the HCC_MASK and ABC_MASK_* of the LCB
  // configuration registers on FELIX could be set accordingly.

  // Loop over hit counter registers
  // FIXME: should not hard code the address
  for (int addr = 0x80; addr <= 0xbf; addr++) {
    auto rr = LCB_FELIX::read_abc_register(addr);
    readHitCounts.insert(readHitCounts.end(), rr.begin(), rr.end());

    // Insert some IDLEs?
    //readCnt.push_back(LCB_FELIX::IDLE);
  }

  // reset hit counters
  auto hitCntRst = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_RESET, 0);
  readHitCounts.insert(readHitCounts.end(), hitCntRst.begin(), hitCntRst.end());

  return readHitCounts;
}

std::vector<uint8_t> StarFelixTriggerLoop::makeTrickleSequence() {

  //////
  // Commands to be sent before starting to send tiggers
  std::vector<uint8_t> trickleSeq_pre;

  // Stop hit counters
  auto stopCnt = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_STOP, 0);
  trickleSeq_pre.insert(trickleSeq_pre.end(), stopCnt.begin(), stopCnt.end());

  // Reset hit counters
  auto resetCnt = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_RESET, 0);
  trickleSeq_pre.insert(trickleSeq_pre.end(), resetCnt.begin(), resetCnt.end());

  // Start hit counters
  auto startCnt = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_START, 0);
  trickleSeq_pre.insert(trickleSeq_pre.end(), startCnt.begin(), startCnt.end());

  // TODO: add some register commands as "pre-buffering"?

  //////
  // Commands to be sent after triggers are done
  std::vector<uint8_t> trickleSeq_post;

  // Stop hit counters
  trickleSeq_post.insert(trickleSeq_post.end(), stopCnt.begin(), stopCnt.end());

  //////
  // Main command sequence
  std::vector<uint8_t> trickleSeq_main;

  auto [trigger_seg, ntrigs_per_seg] = getTriggerSegment();
  // This is one trigger segment with m_trigFreq taken into account
  // The number of triggers in one segment is indicated by ntrigs_per_seg

  // TODO: add pulse
  // compute index for inserting pulse command
  //int iBC_pulse;

  // Put multiple trigger sequences together. The total number of triggers should
  // be < 256 to avoid overflow of the hit counters.
  // Use 248 since it is divisible by 4
  unsigned nTrigs = 248;
  unsigned nSegments = nTrigs / ntrigs_per_seg;
  for (unsigned s=0; s<nSegments; s++) {
    trickleSeq_main.insert(trickleSeq_main.end(), trigger_seg.begin(), trigger_seg.end());
  }

  // Read the hit counters and reset them
  auto hitcount_seg = getHitCounterSegment();
  trickleSeq_main.insert(trickleSeq_main.end(), hitcount_seg.begin(), hitcount_seg.end());

  // trickleSeq_main can be repeated to fill up the trickle memory
  int nBurst = (LCB_FELIX::TRICKLE_MEM_SIZE - trickleSeq_pre.size() - trickleSeq_post.size()) / trickleSeq_main.size();

  if (nBurst < 1) {
    logger->critical("Cannot write the trigger sequence to trickle memory: trigger frequency is too low. No trigger will be sent.");
  }

  //////
  // Total number of triggers in the trickle memory
  m_nTrigsTrickle = ntrigs_per_seg * nSegments * nBurst;

  // Put everything together
  std::vector<uint8_t> trickleSeq;
  trickleSeq.insert(trickleSeq.end(), trickleSeq_pre.begin(), trickleSeq_pre.end());

  for (unsigned i=0; i<nBurst; i++) {
    trickleSeq.insert(trickleSeq.end(), trickleSeq_main.begin(), trickleSeq_main.end());
  }

  trickleSeq.insert(trickleSeq.end(), trickleSeq_post.begin(), trickleSeq_post.end());

  return trickleSeq;
}
