#include "StarFelixTriggerLoop.h"
#include "LCBUtils.h"
#include "LCBFwUtils.h"
#include "AbcCfg.h"
#include "HccCfg.h"

#include "logging.h"

#include <algorithm>
#include <cmath>

namespace {
  auto logger = logging::make_log("StarFelixTriggerLoop");
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
    // FELIX Phase 2 FW spec (v1.007)
    // 8.5.9 ITK STRIPS LCB ENCODER
    // Table 8.32
    uint32_t n_link = el / 0x40;
    uint32_t n_segment = (el - n_link*0x40) / 5;
    config_elinks  .push_back( n_link*0x40 + n_segment*5 + 0 );
    trickle_elinks .push_back( n_link*0x40 + n_segment*5 + 2 );
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

  // Frequency to send trickle pulse
  g_tx->setTrigFreq(m_trickleFreq);

  // The TrigCnt of g_tx is the number of time to loop over the trickle memory
  // round up
  uint32_t nPulse = std::ceil(static_cast<float>(getTrigCnt()) / m_nTrigsTrickle);
  g_tx->setTrigCnt(nPulse);
  logger->debug("nPulse = {}", nPulse);

  // The total number of triggers that are actually sent out.
  unsigned nTotalTrigs = nPulse * m_nTrigsTrickle;

  // This is always equal to or greater than what is requested.
  // (Otherwise, have to compute and change the read address of the trickle
  // memory for the last iteration in order to send the exact number of triggers)

  if (nTotalTrigs != getTrigCnt()) {
    // Update the trigger counts to what is actually sent for analysis later
    setTrigCnt(nTotalTrigs);
    logger->warn("The actual number of triggers sent is {}", nTotalTrigs);
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
  config["useHitCount"] = m_useHitCount;
  config["trickle_frequency"] = m_trickleFreq;
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

  if (config.contains("useHitCount"))
    m_useHitCount = config["useHitCount"];

  if (config.contains("trickle_frequency"))
    m_trickleFreq = config["trickle_frequency"];

  logger->info("Configured trigger loop: trig_count: {} trig_frequency: {} l0_delay: {}", getTrigCnt(), m_trigFreq, m_trigDelay);
}

std::tuple<std::vector<uint8_t>, unsigned> StarFelixTriggerLoop::getTriggerSegment(unsigned max_trigs) {

  std::vector<uint8_t> triggers;
  unsigned nTriggers {0}; // number of triggers in triggers;

  if (max_trigs == 0)
    return {triggers, nTriggers};

  if (m_trigFreq <= 10e6) { // < 10 MHz
    // One trigger per L0A
    auto l0a = LCB_FELIX::l0a_mask(1, 0, false);

    // Number of LCB frames between each L0A
    // One frame covers 4 BCs = 100 ns
    int nFrames = (1e9 / m_trigFreq / 100);

    // nFrames-1 IDLEs, followed by an L0A
    triggers.assign(nFrames-1, LCB_FELIX::IDLE);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());

    nTriggers = 1;

    // Update to the actual trigger frequency
    // (in case m_trigFreq is not divisible by 10^7)
    m_trigFreq = 1e9 / (nFrames * 100);

    logger->info("Effective trigger rate is set to: {:.2f} kHz", m_trigFreq/1000);

  } else if (m_trigFreq < 15e6) { // 10 ~ 15 MHz
    // L0A bits: 0010 0100 1001 ...

    auto l0a = LCB_FELIX::l0a_mask(0b0010, 0, false);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());
    nTriggers += 1;

    if (max_trigs > 1) {
      l0a = LCB_FELIX::l0a_mask(0b0100, 0, false);
      triggers.insert(triggers.end(), l0a.begin(), l0a.end());
      nTriggers += 1;
    }

    if (max_trigs == 3) {
      l0a = LCB_FELIX::l0a_mask(0b1000, 0, false);
      triggers.insert(triggers.end(), l0a.begin(), l0a.end());
      nTriggers += 1;
    } else if (max_trigs > 3) {
      l0a = LCB_FELIX::l0a_mask(0b1001, 0, false);
      triggers.insert(triggers.end(), l0a.begin(), l0a.end());
      nTriggers += 2;
    }

    // Update to the actual trigger frequency
    m_trigFreq = 1e9 / (3*25); // Hz
    logger->info("Effective trigger rate is set to: 13.33 MHz");

  } else if (m_trigFreq < 30e6) { // 15 ~ 30 MHz
    // L0A bits: 0101 ...
    uint8_t mask = 0b0101;
    nTriggers = 2;

    if (max_trigs < 2) {
      mask = 0b0100;
      nTriggers = 1;
    }

    auto l0a = LCB_FELIX::l0a_mask(mask, 0, false);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());

    // Update to the actual trigger frequency
    m_trigFreq = 1e9 / (2*25); // Hz
    logger->info("Effective trigger rate is set to: 20 MHz");

  } else { // > 30 MHz
    // L0A bits: 1111
    uint8_t mask = 0b1111;

    if (max_trigs == 3) {
      mask = 0b1110;
    } else if (max_trigs == 2) {
      mask = 0b1100;
    } else if (max_trigs == 1) {
      mask = 0b1000;
    }

    auto l0a = LCB_FELIX::l0a_mask(mask, 0, false);
    triggers.insert(triggers.end(), l0a.begin(), l0a.end());

    nTriggers = std::min<unsigned>(4, max_trigs);

    // Update to the actual trigger frequency
    m_trigFreq = 1e9/25; // Hz
    logger->info("Effective trigger rate is set to: 40 MHz");
  }

  return {triggers, nTriggers};
}

void StarFelixTriggerLoop::addChargeInjection(std::vector<uint8_t>& trig_segment) {
  if (m_trigFreq > 5e6) { // trigger frequency > 5 MHz
    // No gap between L0A frames. Not possible to insert pulse command.
    logger->error("Trigger frequency is too high to add charge injection command. No charge is injected.");
    // Set m_trigDelay to some invalid value
    m_trigDelay = -1;
    return;
  }

  // TODO
  // For now, assume m_trigDelay is less than the number of BCs between L0As
  // Otherwise a pulse fast command need be injected to the previous trigger segment. This has to be done elsewhere.
  if (m_trigDelay >= 1e9 /m_trigFreq / 25) { // 1 BC is 25 ns
    logger->warn("Trigger delay is greater than the interval between triggers. Cannot inject charges with the current version.");
    // Set m_trigDelay to some invalid value
    m_trigDelay = -1;
    return;
  }

  // The last three bytes should be an L0A, all other entries before that are IDLEs
  assert(trig_segment.size() >1);

  // Index of the start of L0A
  unsigned index_l0a = trig_segment.size() - 3;
  assert(trig_segment[index_l0a] == LCB_FELIX::L0A);

  // Charge injection command
  std::array<uint8_t, 2> inj;
  uint8_t bcsel = 3 - (m_trigDelay % 4);

  if (m_digital) {
    inj = LCB_FELIX::fast_command(LCB::ABC_DIGITAL_PULSE, bcsel);
  } else {
    inj = LCB_FELIX::fast_command(LCB::ABC_CAL_PULSE, bcsel);
  }

  // Number of IDLE frames between the fast command and L0A
  int nIDLE = m_trigDelay / 4;

  // Index of the IDLE frame to be replaced by the fast command
  int iInj = index_l0a - nIDLE - 1;
  assert(iInj >= 0); // due to the assumption above

  // Replace the IDLE frame
  trig_segment.erase(trig_segment.begin()+iInj);
  trig_segment.insert(trig_segment.begin()+iInj, inj.begin(), inj.end());
}


std::vector<uint8_t> StarFelixTriggerLoop::getHitCounterSegment() {
  std::vector<uint8_t> readHitCounts;

  for (int i=0; i<10; i++) {
    readHitCounts.push_back(LCB_FELIX::IDLE);
  }

  // Broadcast read commands for hit counter registers
  // TODO:
  // In case we wish to mask some FEs, the HCC_MASK and ABC_MASK_* of the LCB
  // configuration registers on FELIX could be set accordingly.

  // Loop over hit counter registers
  for (unsigned addr = ABCStarRegister::HitCountREG0; addr <= ABCStarRegister::HitCountREG63; addr++) {
    auto rr = LCB_FELIX::read_abc_register(addr);
    readHitCounts.insert(readHitCounts.end(), rr.begin(), rr.end());

    // Some register reads would be ignored in the firmware LCB encoder if they are sent consecutively.
    // Inserting some (10) IDLEs between the register reads seems to help.
    for (int i=0; i<10; i++) {
      readHitCounts.push_back(LCB_FELIX::IDLE);
    }
  }

  // reset hit counters
  auto hitCntRst = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_RESET, 0);
  readHitCounts.insert(readHitCounts.end(), hitCntRst.begin(), hitCntRst.end());

  // start hit counters again
  auto hitCntStart = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_START, 0);
  readHitCounts.insert(readHitCounts.end(), hitCntStart.begin(), hitCntStart.end());

  for (int i=0; i<10; i++) {
    readHitCounts.push_back(LCB_FELIX::IDLE);
  }

  return readHitCounts;
}

std::vector<uint8_t> StarFelixTriggerLoop::makeTrickleSequence() {

  std::vector<uint8_t> trickleSeq;

  //////
  // Commands to be sent before starting to send tiggers
  std::vector<uint8_t> trickleSeq_pre;

  // Add some register commands as pre-buffering for the command decoder
  // Read HCC OPmode register
  auto hccRR = LCB_FELIX::read_hcc_register(HCCStarRegister::OPmode);
  trickleSeq_pre.insert(trickleSeq_pre.end(), hccRR.begin(), hccRR.end());

  // Stop hit counters
  auto stopCnt = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_STOP, 0);
  trickleSeq_pre.insert(trickleSeq_pre.end(), stopCnt.begin(), stopCnt.end());

  if (m_useHitCount) {
    // Reset hit counters
    auto resetCnt = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_RESET, 0);
    trickleSeq_pre.insert(trickleSeq_pre.end(), resetCnt.begin(), resetCnt.end());

    // Start hit counters
    auto startCnt = LCB_FELIX::fast_command(LCB::ABC_HIT_COUNT_START, 0);
    trickleSeq_pre.insert(trickleSeq_pre.end(), startCnt.begin(), startCnt.end());
  }

  // Add some IDLE frames before sending triggers
  for (int i=0; i<10; i++) {
    trickleSeq_pre.push_back(LCB_FELIX::IDLE);
  }

  //////
  // Commands to be sent after triggers are done
  std::vector<uint8_t> trickleSeq_post;

  // Stop hit counters
  trickleSeq_post.insert(trickleSeq_post.end(), stopCnt.begin(), stopCnt.end());

  //////
  // Main command sequence
  auto [trigger_seg, ntrig_per_seg] = getTriggerSegment();
  // This is one trigger segment with m_trigFreq taken into account
  // The number of triggers in one segment is indicated by ntrig_per_seg

  // Add charge injection if needed
  if (not m_noInject) {
    addChargeInjection(trigger_seg);
  }

  // Command segment to read and reset hit counters
  std::vector<uint8_t> hitcount_seg;
  if (m_useHitCount) {
    hitcount_seg = getHitCounterSegment();
  }

  // Put multiple trigger segments together, followed by hit counters read
  // The max number of triggers should be < 256 to avoid hit counter overflow
  // Use 250
  unsigned ntrig_per_burst = std::min<unsigned>(250, getTrigCnt());
  unsigned nSeg_per_burst = ntrig_per_burst / ntrig_per_seg;

  // The number of trigger segments per burst is also constrained by the trickle
  // memory size
  unsigned bytes_available = LCB_FELIX::TRICKLE_MEM_SIZE - trickleSeq_pre.size() - trickleSeq_post.size() - hitcount_seg.size();
  unsigned nMaxSeg_by_size = bytes_available / trigger_seg.size();

  if (nMaxSeg_by_size == 0) {
    logger->error("The trigger sequence is too long to be written into the trickle memory. The trigger frequency is too low.");
    logger->warn("No trigger will be sent");

    m_nTrigsTrickle = 0;
    m_trigFreq = -1; // set to some invalid value

    return trickleSeq; // empty vector

  } else if (nMaxSeg_by_size < 2) {
    logger->warn("Only one trigger segment is written into the trickle memory! The trigger frequency will not be accurate.");
    logger->warn("Increase the trigger frequency to write more triggers into the trickle memory.");

    m_trigFreq = -1; // set to some invalid value
  }

  if (nSeg_per_burst >= nMaxSeg_by_size) {
    // Number of trigger segment is limited by the trickle memory size
    nSeg_per_burst = nMaxSeg_by_size;

    // Also update the number of triggers per burst
    ntrig_per_burst = nSeg_per_burst * ntrig_per_seg;
  }

  // Fill the burst
  std::vector<uint8_t> trickleSeq_burst;
  for (unsigned s=0; s<nSeg_per_burst; s++) {
    trickleSeq_burst.insert(trickleSeq_burst.end(), trigger_seg.begin(), trigger_seg.end());
  }

  logger->debug("ntrig_per_burst = {}", ntrig_per_burst);
  logger->debug("ntrig_per_seg = {}", ntrig_per_seg);
  logger->debug("nSeg_per_burst = {}", nSeg_per_burst);

  // Add the remaining triggers in case ntrig_per_burst is not divisible by ntrig_per_seg
  // Only needed when trigger frequency is > 10 MHz
  if (ntrig_per_burst % ntrig_per_seg) {
    auto [trigger_last, ntrig_last] = getTriggerSegment(ntrig_per_burst % ntrig_per_seg);
    trickleSeq_burst.insert(trickleSeq_burst.end(), trigger_last.begin(), trigger_last.end());
    assert( nSeg_per_burst * ntrig_per_seg + ntrig_last == ntrig_per_burst );

    logger->debug("ntrig_last = {}", ntrig_last);
  }

  // Add hit counter read and reset commands
  trickleSeq_burst.insert(trickleSeq_burst.end(), hitcount_seg.begin(), hitcount_seg.end());

  // The trigger burst can be repeated to fill up the trickle memory
  // Max number of repetitions allowed by size
  int nBurstMax = (LCB_FELIX::TRICKLE_MEM_SIZE - trickleSeq_pre.size() - trickleSeq_post.size()) / trickleSeq_burst.size();

  if (nBurstMax < 1) {
    logger->error("No triggers are written to the trickle memory!");
  }

  // Number of trickleSeq_burst needed to send the required number of triggers
  int nBurstNeed = std::ceil(static_cast<float>(getTrigCnt()) / ntrig_per_burst);

  // The actual number of times to repeat trickleSeq_burst in the trickle memory
  int nBurst = std::min(nBurstNeed, nBurstMax);

  logger->debug("nBurstNeed = {}", nBurstNeed);
  logger->debug("nBurstMax = {}", nBurstMax);
  logger->debug("nBurst = {}", nBurst);

  // Put everything together
  trickleSeq.insert(trickleSeq.end(), trickleSeq_pre.begin(), trickleSeq_pre.end());

  for (unsigned i=0; i<nBurst; i++) {
    trickleSeq.insert(trickleSeq.end(), trickleSeq_burst.begin(), trickleSeq_burst.end());
  }

  trickleSeq.insert(trickleSeq.end(), trickleSeq_post.begin(), trickleSeq_post.end());

  //////
  // Total number of triggers in the trickle memory
  m_nTrigsTrickle = ntrig_per_burst * nBurst;

  logger->debug("m_nTrigsTrickle = {}", m_nTrigsTrickle);

  logger->debug("trickleSeq.size = {}", trickleSeq.size());
  for (auto seq : trickleSeq) {
    logger->trace("{:x}", seq);
  }

  return trickleSeq;
}
