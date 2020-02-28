#include "StarMaskLoop.h"

#include <iomanip>

#include "StarMask_CalEn.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("StarMaskLoop");
}

StarMaskLoop::StarMaskLoop() : LoopActionBase(LOOP_STYLE_MASK) {
    min = 0;
    max = 16;
    step = 1;
    m_cur = 0;
    m_nMaskedStripsPerGroup=0;
    m_nEnabledStripsPerGroup=0;
    m_EnabledMaskedShift=0;
    loopType = typeid(this);
}


void StarMaskLoop::initMasks() {
  int pos=0;
  while (pos<256) {
    for (unsigned int i=0; i<m_nMaskedStripsPerGroup; i++) m_maskedChannelsRing.fill(0x1, 1);
    pos += m_nMaskedStripsPerGroup;
    if (pos<256){
      for (unsigned int i=0; i<(max-m_nMaskedStripsPerGroup); i++) m_maskedChannelsRing.fill(0x0, 1);
      pos += max-m_nMaskedStripsPerGroup;
    }
  }
  for (pos=0; pos<m_EnabledMaskedShift; pos++) m_enabledChannelsRing.fill(0x0,1);
  while (pos<256) {
    for (unsigned int i=0; i<m_nEnabledStripsPerGroup; i++) m_enabledChannelsRing.fill(0x1, 1);
    pos += m_nEnabledStripsPerGroup;
    if (pos<256) {
      for (unsigned int i=0; i<(max-m_nEnabledStripsPerGroup); i++) m_enabledChannelsRing.fill(0x0, 1);
      pos += max-m_nEnabledStripsPerGroup;
    }
  }
}

void StarMaskLoop::init() {
    SPDLOG_LOGGER_DEBUG(logger, "Init");

    m_done = false;
    m_cur = min;
    if (m_nEnabledStripsPerGroup) initMasks();
    
    for ( FrontEnd* fe : keeper->feList ) {
    	if (!fe->isActive()) {continue;}
	if (!m_nEnabledStripsPerGroup)
	  applyEncodedMask(static_cast<StarChips*>(fe), m_cur);
	else {
	  m_maskedChannelsRing.pos=m_cur;
	  m_enabledChannelsRing.pos=m_cur;
	  const uint32_t * masks = m_maskedChannelsRing.readMask();
	  const uint32_t * enables = m_enabledChannelsRing.readCalEnable();
	  applyMask(static_cast<StarChips*>(fe), masks, enables);
	}
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
}

void StarMaskLoop::printMask(const uint32_t chans[8], std::ostream &os) {
  os << "write mask: [";
  for (int j=0; j<8; j++)
    os << "  0x" << std::hex << std::setfill('0') << std::setw(8)<< chans[j] <<std::dec;
  os << "]\n";
}

void StarMaskLoop::applyMask(StarChips* fe, const uint32_t masks[8], const uint32_t enables[8]) {
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
      logger->debug("{}", oss.str());
    } // End debug

  auto num_abc = fe->numABCs();

  auto writeReg = [&](auto words) {
    g_tx->writeFifo((words[0] << 16) + words[1]);
    g_tx->writeFifo((words[2] << 16) + words[3]);
    g_tx->writeFifo((words[4] << 16) + words[5]);
    g_tx->writeFifo((words[6] << 16) + words[7]);
    g_tx->writeFifo((words[8] << 16) + LCB::IDLE);

    // Could have this once for all regs though...
    g_tx->releaseFifo();
  };

  for( int iChip = 1; iChip < num_abc+1; ++iChip){ //exclude iChip=0 which is the Hcc
    //Looping over MaskInput registers
    int index=0;
    for (int j=ABCStarRegister::MaskInput(0); j<=ABCStarRegister::MaskInput(7); j++){
      logger->trace("write mask: {} {} 0x{:08x}", iChip, index, masks[index]);
      writeReg(fe->write_abc_register(j, ~masks[index], 0xf));  // strip 0's mask starts from reg 16,
      index++;
    }
    //Looping over CAL ENABLE registers
    index=0;
    for (int j=ABCStarRegister::CalREG0; j<=ABCStarRegister::CalREG7; j++){
      logger->trace("write cal: {} {} 0x{:08x}", iChip, j, enables[index]);
      writeReg(fe->write_abc_register(j, enables[index], 0xf));  // strip 0's mask starts from reg 104,.
      index++;
    }
  }//end of loop over ABCs
}

void StarMaskLoop::applyEncodedMask(StarChips* fe, unsigned int curStep) {
  applyMask(fe, star_masks[curStep], star_calEn[curStep]);
}

void StarMaskLoop::execPart2() {
    SPDLOG_LOGGER_DEBUG(logger, " End {} -> {}", m_cur, m_cur + step);
    m_cur += step;
    if (!((int)m_cur < max))
      m_done = true;
    else {
      // Shift Enable mask by step size

      if(keeper->feList.empty()) {
        logger->warn("No ABCs defined to write masks to!\n");
      }

      // FIXME: Global writes used, but loop over FE to be sure of tx mask
      for ( FrontEnd* fe : keeper->feList ) {
	if (!fe->isActive()) {continue;}
	
	if (!m_nEnabledStripsPerGroup)
	  applyEncodedMask(static_cast<StarChips*>(fe), m_cur);
	else {
	  m_maskedChannelsRing.pos=m_cur;
	  m_enabledChannelsRing.pos=m_cur;
	  const uint32_t * masks = m_maskedChannelsRing.readMask();
	  const uint32_t * enables = m_enabledChannelsRing.readCalEnable();
	  applyMask(static_cast<StarChips*>(fe), masks, enables);
	}
      }

      while(g_tx->isCmdEmpty() == 0){std::cout << __PRETTY_FUNCTION__ << "\033[1;32m " << "cmd not empty: " << "\033[0m"<<  std::endl;};
    }
}


void StarMaskLoop::writeConfig(json &config) {
    config["min"] = min;
    config["max"] = max;
    config["step"] = step;
    config["nMaskedStripsPerGroup"] = m_nMaskedStripsPerGroup;
    config["nEnabledStripsPerGroup"] = m_nEnabledStripsPerGroup;
    config["EnabledMaskedShift"] = m_EnabledMaskedShift;
}

void StarMaskLoop::loadConfig(json &config) {
    min = config["min"];
    max = config["max"];
    step = config["step"];
    m_nMaskedStripsPerGroup = config["nMaskedStripsPerGroup"];
    m_nEnabledStripsPerGroup = config["nEnabledStripsPerGroup"];
    m_EnabledMaskedShift = config["EnabledMaskedShift"];
    logger->debug("Loaded StarMaskLoop configuration with nMaskedStripsPerGroup={}, nEnabledStripsPerGroup={}, shifted by  strips, min={}, max={}, step={}",
                  m_nMaskedStripsPerGroup, m_nEnabledStripsPerGroup,
                  m_EnabledMaskedShift, min, max, step);
}
