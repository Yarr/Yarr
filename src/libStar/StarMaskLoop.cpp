#include "StarMaskLoop.h"

#include <iomanip>

#include "logging.h"

namespace {
    auto logger = logging::make_log("StarMaskLoop");
}

StarMaskLoop::StarMaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 16;
    step = 1;
    m_cur = 0;

    m_doNmask = false;

    m_nMaskedStripsPerGroup=0;
    m_nEnabledStripsPerGroup=0;
    m_EnabledMaskedShift=0;
    m_onlyMask = false;
    loopType = typeid(this);
}


void StarMaskLoop::initMasks() {
  m_maskedChannelsRing.reset();

  while (!m_maskedChannelsRing.full()) {
    for (unsigned int i=0; i<m_nMaskedStripsPerGroup; i++) m_maskedChannelsRing.fill(true);
    if (!m_maskedChannelsRing.full()) {
      for (unsigned int i=0; i<(max-m_nMaskedStripsPerGroup); i++) m_maskedChannelsRing.fill(false);
    }
  }

  m_enabledChannelsRing.reset();

  for (unsigned int pos=0; pos<m_EnabledMaskedShift; pos++) m_enabledChannelsRing.fill(false);
  while (!m_enabledChannelsRing.full()) {
    for (unsigned int i=0; i<m_nEnabledStripsPerGroup; i++) m_enabledChannelsRing.fill(true);
    if (!m_enabledChannelsRing.full()) {
      for (unsigned int i=0; i<(max-m_nEnabledStripsPerGroup); i++) m_enabledChannelsRing.fill(false);
    }
  }

  SPDLOG_LOGGER_DEBUG(logger, "ChannelRings:");
  if (logger->should_log(spdlog::level::debug)) {
    if(!m_onlyMask) {
      m_enabledChannelsRing.printRing();
    }
    m_maskedChannelsRing.printRing();
  }
}

void StarMaskLoop::setupNmask(unsigned int count) {
  m_maskedChannelsRing.reset();
  m_enabledChannelsRing.reset();

  // Mask reads out as 1 if false is inserted...
  for(unsigned int pos=0; pos<count; pos++) {
    m_maskedChannelsRing.fill(true);
    m_enabledChannelsRing.fill(true);
  }

  for(unsigned int pos=count; pos<256; pos++) {
    m_maskedChannelsRing.fill(false);
    m_enabledChannelsRing.fill(false);
  }
  assert(m_maskedChannelsRing.full());
  assert(m_enabledChannelsRing.full());

  SPDLOG_LOGGER_DEBUG(logger, "ChannelRings:");
  if (logger->should_log(spdlog::level::debug)) {
    if(!m_onlyMask) {
      m_enabledChannelsRing.printRing();
    }
    m_maskedChannelsRing.printRing();
  }
}

void StarMaskLoop::init() {
    SPDLOG_LOGGER_DEBUG(logger, "Init");

    m_done = false;
    m_cur = min;
    if (m_doNmask) {
      // setupNmask called later
    } else if (m_nEnabledStripsPerGroup != 0) {
      initMasks();
    }

    while(g_tx->isCmdEmpty() == 0);
}

void StarMaskLoop::end() {
    SPDLOG_LOGGER_DEBUG(logger, "End");
    while(g_tx->isCmdEmpty() == 0);
}

void StarMaskLoop::execPart1() {
  SPDLOG_LOGGER_DEBUG(logger, "-> Mask stage {}", m_cur);

  g_stat->set(this, m_cur);

  int offset = m_cur;
  if (m_doNmask) {
    setupNmask(m_cur);
    offset = 0;
  } else {
    offset = (256-m_cur)%256;
  }

  // FIXME: Global writes used, but loop over FE to be sure of tx mask
  if(keeper->getNumOfEntries() == 0) {
    logger->warn("No ABCs defined to write masks to!\n");
  }

  for (unsigned id=0; id<keeper->getNumOfEntries(); id++) {
    FrontEnd *fe = keeper->getEntry(id).fe;
    if (!fe->isActive()) {continue;}

    auto masks = m_maskedChannelsRing.readMask(offset);
    auto enables = m_enabledChannelsRing.readCalEnable(offset);
    applyMask(static_cast<StarChips*>(fe), masks, enables);
  }

  while(g_tx->isCmdEmpty() == 0) {
    // This might no longer be useful
    logger->trace("Waiting for cmd to empty");
  }
}

void StarMaskLoop::printMask(MaskType chans, std::ostream &os) const {
  os << "write mask: [";
  for (int j=0; j<8; j++)
    os << "  0x" << std::hex << std::setfill('0') << std::setw(8)<< chans[j] <<std::dec;
  os << "]\n";
}

void StarMaskLoop::applyMask(StarChips* fe, MaskType masks, MaskType enables) {
    SPDLOG_LOGGER_DEBUG(logger, "Apply masks");
    if (logger->should_log(spdlog::level::debug)) {
      std::stringstream oss;
      oss << "Apply masks:" << std::endl;
      printMask(masks, oss);

      std::string row1, row2;
      for (unsigned int ireg=0; ireg<8; ireg++)
        for (unsigned int i=0;i<32;i++)
          if (i%2==0)
            row1 += (((masks[ireg]>>i) & 0x1) ? "1" : "0");
          else
            row2 += (((masks[ireg]>>i) & 0x1) ? "1" : "0");
      oss << "2nd row: " << row2.c_str() << "\n";
      oss << "1sr row: " << row1.c_str() << "\n";

      if(!m_onlyMask) {
        oss << "Enable channels:\n";
        printMask(enables, oss);
        row1=""; row2="";
        for (unsigned int ireg=0; ireg<8; ireg++)
          for (unsigned int i=0;i<32;i++)
            if ((i%4)<2)
              row1 += (((enables[ireg]>>i) & 0x1) ? "1" : "0");
            else
              row2 += (((enables[ireg]>>i) & 0x1) ? "1" : "0");
        oss << "2nd row: " << row2.c_str() << "\n";
        oss << "1sr row: " << row1.c_str() << "\n";
      }

      logger->debug("{}", oss.str());
    } // End debug

  auto writeReg = [&](auto words) {
    g_tx->writeFifo((words[0] << 16) + words[1]);
    g_tx->writeFifo((words[2] << 16) + words[3]);
    g_tx->writeFifo((words[4] << 16) + words[5]);
    g_tx->writeFifo((words[6] << 16) + words[7]);
    g_tx->writeFifo((words[8] << 16) + LCB::IDLE);

    // Could have this once for all regs though...
    g_tx->releaseFifo();
  };

  // NB currently using broadcast writes.
  // I the future this could be expanded to allow an overriding
  // mask for each front-end, but that requires a more complex
  // configuration architecture.

  //Looping over MaskInput registers
  for (int j=ABCStarRegister::MaskInput(0), index = 0;
       j<=ABCStarRegister::MaskInput(7);
       j++, index++) {
    logger->trace("write mask: {} 0x{:08x}", index, masks[index]);
    writeReg(fe->write_abc_register(j, masks[index], 0xf));
  }

  if(!m_onlyMask) {
    //Looping over CAL ENABLE registers
    for (int j=ABCStarRegister::CalReg(0), index = 0;
         j<=ABCStarRegister::CalReg(7);
         j++, index++) {
      logger->trace("write cal: {} 0x{:08x}", j, enables[index]);
      writeReg(fe->write_abc_register(j, enables[index], 0xf));
    }
  }
}

void StarMaskLoop::execPart2() {
    SPDLOG_LOGGER_DEBUG(logger, " End {} -> {}", m_cur, m_cur + step);
    m_cur += step;
    if (!((int)m_cur < max)) {
      m_done = true;
    }
}


void StarMaskLoop::writeConfig(json &config) {
    config["min"] = min;
    config["max"] = max;
    config["step"] = step;
    config["nMaskedStripsPerGroup"] = m_nMaskedStripsPerGroup;
    config["nEnabledStripsPerGroup"] = m_nEnabledStripsPerGroup;
    config["EnabledMaskedShift"] = m_EnabledMaskedShift;

    config["maskOnly"] = m_onlyMask;
    config["doNmask"] = m_doNmask;
}

void StarMaskLoop::loadConfig(const json &config) {
    min = config["min"];
    max = config["max"];
    step = config["step"];
    m_nMaskedStripsPerGroup = config["nMaskedStripsPerGroup"];
    m_nEnabledStripsPerGroup = config["nEnabledStripsPerGroup"];
    m_EnabledMaskedShift = config["EnabledMaskedShift"];

    if(m_nEnabledStripsPerGroup == 0) {
      // Default version (how it was before ChannelRing introduced)
      // Assume range is 0-127, then 1 puts one hit in each block
      m_nMaskedStripsPerGroup = 1;
      m_nEnabledStripsPerGroup = 1;
      m_EnabledMaskedShift = 0;
    }

    if(config.contains("parameter") && config["parameter"]) {
      m_style = LOOP_STYLE_PARAMETER;
    }

    if(config.contains("maskOnly")) {
      m_onlyMask = config["maskOnly"];
    }

    if(config.contains("doNmask")) {
      m_doNmask = config["doNmask"];
    }

    logger->debug("Loaded StarMaskLoop configuration with nMaskedStripsPerGroup={}, nEnabledStripsPerGroup={}, shifted by  strips, min={}, max={}, step={}",
                  m_nMaskedStripsPerGroup, m_nEnabledStripsPerGroup,
                  m_EnabledMaskedShift, min, max, step);

    if(256 % max) {
      // Problem is that a loop will enable a channel more than once
      logger->warn("Group of {} channels not supported (remainder from 256 is non-zero)", max);
    }
}
