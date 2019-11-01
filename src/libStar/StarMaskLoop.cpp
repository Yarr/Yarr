#include "StarMaskLoop.h"
#include "StarMask_CalEn.h"

StarMaskLoop::StarMaskLoop() : LoopActionBase() {
    min = 0;
    max = 16;
    step = 1;
    m_cur = 0;
    m_nMaskedStripsPerGroup=0;
    m_nEnabledStripsPerGroup=0;
    m_EnabledMaskedShift=0;
    loopType = typeid(this);
    verbose=false;
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
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;

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
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    while(g_tx->isCmdEmpty() == 0);
}

void StarMaskLoop::execPart1() {
  if (verbose) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::cout << " ---> Mask Stage " << m_cur << std::endl;
  }

  g_stat->set(this, m_cur);
}

void StarMaskLoop::printMask(const uint32_t chans[8]) {
  std::cout << "write mask: [";
  for (int j=0; j<8; j++)
    std::cout << "  0x" << std::hex << std::setfill('0') << std::setw(8)<< chans[j] <<std::dec;
  std::cout << "]" << std::endl;
}

void StarMaskLoop::applyMask(StarChips* fe, const uint32_t masks[8], const uint32_t enables[8]) {
  if (verbose) {
    std::cout << "Apply masks:" << std::endl;
    printMask(masks);

    std::string row1, row2;
    for (unsigned int ireg=0; ireg<8; ireg++)
      for (unsigned int i=0;i<32;i++)
	if ((i%4)<2)
	  row1 += (((masks[ireg]>>i) & 0x1) ? "1" : "0");
	else
	  row2 += (((masks[ireg]>>i) & 0x1) ? "1" : "0");
    std::cout << "2nd row: " << row2.c_str() << std::endl;
    std::cout << "1sr row: " << row1.c_str() << std::endl;

    
    std::cout << "Enable channels:" << std::endl;
    printMask(enables);
  }
  
  for( int iChip = 1; iChip < fe->m_nABC+1; ++iChip){ //exclude iChip=0 which is the Hcc
    //Looping over MaskInput registers
    int index=0;
    for (int j=16; j<24; j++){
      //    				 std::cout << "write mask: "<< i << "  0x" << std::hex << std::setfill('0') << std::setw(8)<< Star_masks[m_cur][index] <<std::dec<< std::endl;
      fe->setAndWriteABCRegister(j, ~masks[index], iChip);  // strip 0's mask starts from reg 16,
      index++;
    }
    //Looping over CAL ENABLE registers
    index=0;
    for (int j=104; j<112; j++){
      //    				 std::cout << "write cal: "<< i  << " 0x" << std::hex << std::setfill('0') << std::setw(8)<< Star_calEn[m_cur][index] <<std::dec<< std::endl;
      fe->setAndWriteABCRegister(j, enables[index], iChip);  // strip 0's mask starts from reg 104,.
      index++;
    }
  }//end of loop over ABCs
}

void StarMaskLoop::applyEncodedMask(StarChips* fe, unsigned int curStep) {
  applyMask(fe, star_masks[curStep], star_calEn[curStep]);
}

void StarMaskLoop::execPart2() {
    if (verbose)
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    m_cur += step;
    if (!((int)m_cur < max))
      m_done = true;
    else {
      // Shift Enable mask by step size

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
    verbose = config["verbose"];
    m_nMaskedStripsPerGroup = config["nMaskedStripsPerGroup"];
    m_nEnabledStripsPerGroup = config["nEnabledStripsPerGroup"];
    m_EnabledMaskedShift = config["EnabledMaskedShift"];
    if (verbose) std::cout << "Loaded StarMaskLoop configuration with nMaskedStripsPerGroup=" << m_nMaskedStripsPerGroup << ", nEnabledStripsPerGroup=" << m_nEnabledStripsPerGroup << ", shifted by " << m_EnabledMaskedShift << " strips, min=" << min << ", max=" << max << ", step=" << step << std::endl;
}
